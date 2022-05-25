//
// Original algorithm is from:
// 	https://sqlite.org/src/file?name=src/mem3.c
// 	SQLite source code is in the public-domain and is free to everyone to use for any purpose.

#include "Kyty/Core/MSpace.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"

namespace Kyty::Core {

static constexpr size_t MSPACE_HEADER_SIZE = 1440;

static uint64_t align(uint64_t v, uint64_t a)
{
	return (v + (a - 1)) & ~(a - 1);
}

static constexpr uint32_t MSPACE_ARRAY_SMALL = 10;
static constexpr uint32_t MSPACE_ARRAY_HASH  = 61;

struct MSpaceBlock
{
	union
	{
		struct
		{
			uint32_t prev_size;
			uint32_t size_4x;
		} hdr;
		struct
		{
			uint32_t next;
			uint32_t prev;
		} list;
	} u;
};

struct MSpaceContext
{
	uint32_t              capacity                            = 0;
	MSpaceBlock*          base                                = nullptr;
	bool                  in_callback                         = false;
	Core::Mutex*          mutex                               = nullptr;
	mspace_dbg_callback_t dbg_callback                        = nullptr;
	char                  name[32]                            = {0};
	uint32_t              index_of_key_chunk                  = 0;
	uint32_t              size_of_key_chunk                   = 0;
	uint32_t              array_small[MSPACE_ARRAY_SMALL - 1] = {};
	uint32_t              array_hash[MSPACE_ARRAY_HASH]       = {};
};

static void MSpaceInternalUnlinkFromList(MSpaceContext& ctx, uint32_t i, uint32_t* root)
{
	uint32_t next = ctx.base[i].u.list.next;
	uint32_t prev = ctx.base[i].u.list.prev;

	if (prev == 0)
	{
		*root = next;
	} else
	{
		ctx.base[prev].u.list.next = next;
	}
	if (next != 0)
	{
		ctx.base[next].u.list.prev = prev;
	}
	ctx.base[i].u.list.next = 0;
	ctx.base[i].u.list.prev = 0;
}

static void MSpaceInternalUnlink(MSpaceContext& ctx, uint32_t i)
{
	uint32_t size = ctx.base[i - 1].u.hdr.size_4x / 4;
	if (size <= MSPACE_ARRAY_SMALL)
	{
		MSpaceInternalUnlinkFromList(ctx, i, &ctx.array_small[size - 2]);
	} else
	{
		uint32_t hash = size % MSPACE_ARRAY_HASH;
		MSpaceInternalUnlinkFromList(ctx, i, &ctx.array_hash[hash]);
	}
}

static void MSpaceInternalLinkIntoList(MSpaceContext& ctx, uint32_t i, uint32_t* root)
{
	ctx.base[i].u.list.next = *root;
	ctx.base[i].u.list.prev = 0;
	if (*root != 0u)
	{
		ctx.base[*root].u.list.prev = i;
	}
	*root = i;
}

static void MSpaceInternalLink(MSpaceContext& ctx, uint32_t i)
{
	uint32_t size = ctx.base[i - 1].u.hdr.size_4x / 4;

	if (size <= MSPACE_ARRAY_SMALL)
	{
		MSpaceInternalLinkIntoList(ctx, i, &ctx.array_small[size - 2]);
	} else
	{
		uint32_t hash = size % MSPACE_ARRAY_HASH;
		MSpaceInternalLinkIntoList(ctx, i, &ctx.array_hash[hash]);
	}
}

static void MSpaceInternalEnter(MSpaceContext& ctx)
{
	if (ctx.mutex != nullptr)
	{
		ctx.mutex->Lock();
	}
}

static void MSpaceInternalLeave(MSpaceContext& ctx)
{
	if (ctx.mutex != nullptr)
	{
		ctx.mutex->Unlock();
	}
}

static void MSpaceInternalOutOfMemory(MSpaceContext& ctx, uint32_t free_bytes, uint32_t bytes)
{
	if (!ctx.in_callback)
	{
		ctx.in_callback = true;

		if (ctx.dbg_callback != nullptr)
		{
			MSpaceInternalLeave(ctx);

			ctx.dbg_callback(&ctx, free_bytes, bytes);

			MSpaceInternalEnter(ctx);
		}

		ctx.in_callback = false;
	}
}

static void* MSpaceInternalCheckout(MSpaceContext& ctx, uint32_t i, uint32_t num_blocks)
{
	uint32_t x                                   = ctx.base[i - 1].u.hdr.size_4x;
	ctx.base[i - 1].u.hdr.size_4x                = num_blocks * 4 | 1u | (x & 2u);
	ctx.base[i + num_blocks - 1].u.hdr.prev_size = num_blocks;
	ctx.base[i + num_blocks - 1].u.hdr.size_4x |= 2u;
	return &ctx.base[i];
}

static void* MSpaceInternalFromKeyBlk(MSpaceContext& ctx, uint32_t num_blocks)
{
	if (num_blocks >= ctx.size_of_key_chunk - 1)
	{
		void* p                = MSpaceInternalCheckout(ctx, ctx.index_of_key_chunk, ctx.size_of_key_chunk);
		ctx.index_of_key_chunk = 0;
		ctx.size_of_key_chunk  = 0;
		return p;
	}

	uint32_t newi = ctx.index_of_key_chunk + ctx.size_of_key_chunk - num_blocks;

	ctx.base[ctx.index_of_key_chunk + ctx.size_of_key_chunk - 1].u.hdr.prev_size = num_blocks;
	ctx.base[ctx.index_of_key_chunk + ctx.size_of_key_chunk - 1].u.hdr.size_4x |= 2u;
	ctx.base[newi - 1].u.hdr.size_4x = num_blocks * 4 + 1;
	ctx.size_of_key_chunk -= num_blocks;
	ctx.base[newi - 1].u.hdr.prev_size                 = ctx.size_of_key_chunk;
	uint32_t x                                         = ctx.base[ctx.index_of_key_chunk - 1].u.hdr.size_4x & 2u;
	ctx.base[ctx.index_of_key_chunk - 1].u.hdr.size_4x = ctx.size_of_key_chunk * 4 | x;
	return static_cast<void*>(&ctx.base[newi]);
}

static void MSpaceInternalMerge(MSpaceContext& ctx, uint32_t* root)
{
	uint32_t next = 0;

	for (uint32_t i = *root; i > 0; i = next)
	{
		next          = ctx.base[i].u.list.next;
		uint32_t size = ctx.base[i - 1].u.hdr.size_4x;
		if ((size & 2u) == 0)
		{
			MSpaceInternalUnlinkFromList(ctx, i, root);
			uint32_t prev = i - ctx.base[i - 1].u.hdr.prev_size;
			if (prev == next)
			{
				next = ctx.base[prev].u.list.next;
			}
			MSpaceInternalUnlink(ctx, prev);
			size                                      = i + size / 4 - prev;
			uint32_t x                                = ctx.base[prev - 1].u.hdr.size_4x & 2u;
			ctx.base[prev - 1].u.hdr.size_4x          = size * 4 | x;
			ctx.base[prev + size - 1].u.hdr.prev_size = size;
			MSpaceInternalLink(ctx, prev);
			i = prev;
		} else
		{
			size /= 4;
		}
		if (size > ctx.size_of_key_chunk)
		{
			ctx.index_of_key_chunk = i;
			ctx.size_of_key_chunk  = size;
		}
	}
}

static void* MSpaceInternalMallocUnsafe(MSpaceContext& ctx, uint32_t size, uint32_t report_size)
{
	uint32_t num_blocks = 0;

	if (size <= 12)
	{
		num_blocks = 2;
	} else
	{
		num_blocks = (size + 11) / 8;
	}

	if (num_blocks <= MSPACE_ARRAY_SMALL)
	{
		uint32_t i = ctx.array_small[num_blocks - 2];
		if (i > 0)
		{
			MSpaceInternalUnlinkFromList(ctx, i, &ctx.array_small[num_blocks - 2]);
			return MSpaceInternalCheckout(ctx, i, num_blocks);
		}
	} else
	{
		uint32_t hash = num_blocks % MSPACE_ARRAY_HASH;
		for (uint32_t i = ctx.array_hash[hash]; i > 0; i = ctx.base[i].u.list.next)
		{
			if (ctx.base[i - 1].u.hdr.size_4x / 4 == num_blocks)
			{
				MSpaceInternalUnlinkFromList(ctx, i, &ctx.array_hash[hash]);
				return MSpaceInternalCheckout(ctx, i, num_blocks);
			}
		}
	}

	if (ctx.size_of_key_chunk >= num_blocks)
	{
		return MSpaceInternalFromKeyBlk(ctx, num_blocks);
	}

	for (uint32_t to_free = num_blocks * 16; to_free < (ctx.capacity * 16); to_free *= 2)
	{
		MSpaceInternalOutOfMemory(ctx, to_free, report_size);
		if (ctx.index_of_key_chunk != 0u)
		{
			MSpaceInternalLink(ctx, ctx.index_of_key_chunk);
			ctx.index_of_key_chunk = 0;
			ctx.size_of_key_chunk  = 0;
		}
		for (auto& hash: ctx.array_hash)
		{
			MSpaceInternalMerge(ctx, &hash);
		}
		for (auto& small: ctx.array_small)
		{
			MSpaceInternalMerge(ctx, &small);
		}
		if (ctx.size_of_key_chunk != 0u)
		{
			MSpaceInternalUnlink(ctx, ctx.index_of_key_chunk);
			if (ctx.size_of_key_chunk >= num_blocks)
			{
				return MSpaceInternalFromKeyBlk(ctx, num_blocks);
			}
		}
	}

	return nullptr;
}

static void MSpaceInternalFreeUnsafe(MSpaceContext& ctx, void* old)
{
	auto* p = static_cast<MSpaceBlock*>(old);

	uint32_t index = p - ctx.base;
	uint32_t size  = ctx.base[index - 1].u.hdr.size_4x / 4;
	ctx.base[index - 1].u.hdr.size_4x &= ~1u;
	ctx.base[index + size - 1].u.hdr.prev_size = size;
	ctx.base[index + size - 1].u.hdr.size_4x &= ~2u;
	MSpaceInternalLink(ctx, index);

	if (ctx.index_of_key_chunk != 0u)
	{
		while ((ctx.base[ctx.index_of_key_chunk - 1].u.hdr.size_4x & 2u) == 0)
		{
			size = ctx.base[ctx.index_of_key_chunk - 1].u.hdr.prev_size;
			ctx.index_of_key_chunk -= size;
			ctx.size_of_key_chunk += size;
			MSpaceInternalUnlink(ctx, ctx.index_of_key_chunk);
			uint32_t x                                         = ctx.base[ctx.index_of_key_chunk - 1].u.hdr.size_4x & 2u;
			ctx.base[ctx.index_of_key_chunk - 1].u.hdr.size_4x = ctx.size_of_key_chunk * 4 | x;
			ctx.base[ctx.index_of_key_chunk + ctx.size_of_key_chunk - 1].u.hdr.prev_size = ctx.size_of_key_chunk;
		}
		uint32_t x = ctx.base[ctx.index_of_key_chunk - 1].u.hdr.size_4x & 2u;
		while ((ctx.base[ctx.index_of_key_chunk + ctx.size_of_key_chunk - 1].u.hdr.size_4x & 1u) == 0)
		{
			MSpaceInternalUnlink(ctx, ctx.index_of_key_chunk + ctx.size_of_key_chunk);
			ctx.size_of_key_chunk += ctx.base[ctx.index_of_key_chunk + ctx.size_of_key_chunk - 1].u.hdr.size_4x / 4;
			ctx.base[ctx.index_of_key_chunk - 1].u.hdr.size_4x                           = ctx.size_of_key_chunk * 4 | x;
			ctx.base[ctx.index_of_key_chunk + ctx.size_of_key_chunk - 1].u.hdr.prev_size = ctx.size_of_key_chunk;
		}
	}
}

static uint32_t MSpaceInternalSize(void* p)
{
	auto* block = static_cast<MSpaceBlock*>(p);
	return (block[-1].u.hdr.size_4x & ~3u) * 2 - 4;
}

static void* MSpaceInternalMalloc(MSpaceContext& ctx, uint32_t size, uint32_t report_size)
{
	MSpaceInternalEnter(ctx);
	auto* p = MSpaceInternalMallocUnsafe(ctx, size, report_size);
	MSpaceInternalLeave(ctx);
	return p;
}

static void* MSpaceInternalMalloc_align(MSpaceContext& ctx, uint32_t size, uint64_t boundary)
{
	auto addr = reinterpret_cast<uint64_t>(MSpaceInternalMalloc(ctx, size + boundary + 8, size));
	if (addr != 0)
	{
		uint64_t aligned_addr = align(addr + 8, boundary);
		auto*    buf          = reinterpret_cast<uint64_t*>(aligned_addr);
		buf[-1]               = addr;
		return buf;
	}
	return nullptr;
}

static void MSpaceInternalFree(MSpaceContext& ctx, void* prior)
{
	MSpaceInternalEnter(ctx);
	MSpaceInternalFreeUnsafe(ctx, prior);
	MSpaceInternalLeave(ctx);
}

static void MSpaceInternalFree_align(MSpaceContext& ctx, void* buf)
{
	auto*    buf64     = static_cast<uint64_t*>(buf);
	uint64_t real_addr = buf64[-1];
	MSpaceInternalFree(ctx, reinterpret_cast<void*>(real_addr));
}

static void* MSpaceInternalRealloc(MSpaceContext& ctx, void* prior, uint32_t size, uint32_t report_size)
{
	if (prior == nullptr)
	{
		return MSpaceInternalMalloc(ctx, size, report_size);
	}
	if (size <= 0)
	{
		MSpaceInternalFree(ctx, prior);
		return nullptr;
	}
	auto old = MSpaceInternalSize(prior);
	if (size <= old && size >= old - 128)
	{
		return prior;
	}
	MSpaceInternalEnter(ctx);
	auto* p = MSpaceInternalMallocUnsafe(ctx, size, report_size);
	if (p != nullptr)
	{
		if (old < size)
		{
			memcpy(p, prior, old);
		} else
		{
			memcpy(p, prior, size);
		}
		MSpaceInternalFreeUnsafe(ctx, prior);
	}
	MSpaceInternalLeave(ctx);
	return p;
}

static void* MSpaceInternalRealloc_align(MSpaceContext& ctx, void* buf, uint32_t size, uint64_t boundary)
{
	auto*    buf64     = static_cast<uint64_t*>(buf);
	uint64_t real_addr = (buf64 != nullptr ? buf64[-1] : 0);
	auto addr = reinterpret_cast<uint64_t>(MSpaceInternalRealloc(ctx, reinterpret_cast<uint64_t*>(real_addr), size + boundary + 8, size));
	if (addr != 0)
	{
		uint64_t aligned_addr = align(addr + 8, boundary);
		if (addr != real_addr)
		{
			auto* dst = reinterpret_cast<void*>(aligned_addr);
			auto* src = reinterpret_cast<void*>(addr + reinterpret_cast<uint64_t>(buf) - real_addr);
			if (dst != src)
			{
				memmove(dst, src, size);
			}
		}
		auto* buf = reinterpret_cast<uint64_t*>(aligned_addr);
		buf[-1]   = addr;
		return buf;
	}
	return nullptr;
}

static bool MSpaceInternalInit(MSpaceContext& ctx, const char* name, void* base, size_t capacity, bool thread_safe,
                               mspace_dbg_callback_t dbg_callback)
{
	if (sizeof(MSpaceBlock) != 8)
	{
		return false;
	}

	if ((capacity / sizeof(MSpaceBlock)) < 3)
	{
		return false;
	}

	ctx = MSpaceContext();

	int s = snprintf(ctx.name, sizeof(ctx.name), "%s", name);

	if (static_cast<size_t>(s) >= sizeof(ctx.name))
	{
		return false;
	}

	ctx.base     = static_cast<MSpaceBlock*>(base);
	ctx.capacity = (capacity / sizeof(MSpaceBlock)) - 2;

	ctx.size_of_key_chunk                  = ctx.capacity;
	ctx.index_of_key_chunk                 = 1;
	ctx.base[0].u.hdr.size_4x              = (ctx.size_of_key_chunk << 2u) + 2;
	ctx.base[ctx.capacity].u.hdr.prev_size = ctx.capacity;
	ctx.base[ctx.capacity].u.hdr.size_4x   = 1;

	ctx.mutex        = (thread_safe ? new Core::Mutex : nullptr);
	ctx.dbg_callback = dbg_callback;

	return true;
}

static void MSpaceInternalShutdown(MSpaceContext& ctx)
{
	delete ctx.mutex;
}

mspace_t MSpaceCreate(const char* name, void* base, size_t capacity, bool thread_safe, mspace_dbg_callback_t dbg_callback)
{
	auto addr = reinterpret_cast<uint64_t>(base);

	if ((addr & 0x7u) != 0)
	{
		return nullptr;
	}

	if ((capacity & 0x7u) != 0)
	{
		return nullptr;
	}

	if (sizeof(MSpaceContext) > MSPACE_HEADER_SIZE)
	{
		return nullptr;
	}

	if (base == nullptr || capacity < MSPACE_HEADER_SIZE)
	{
		return nullptr;
	}

	if (name == nullptr)
	{
		return nullptr;
	}

	uint64_t aligned_buf_addr = align(addr + MSPACE_HEADER_SIZE, 1);

	auto* ctx = reinterpret_cast<MSpaceContext*>(addr);
	auto* buf = reinterpret_cast<void*>(aligned_buf_addr);

	if (!MSpaceInternalInit(*ctx, name, buf, (capacity - (aligned_buf_addr - addr)), thread_safe, dbg_callback))
	{
		return nullptr;
	}

	return ctx;
}

bool MSpaceDestroy(mspace_t msp)
{
	if (msp == nullptr)
	{
		return false;
	}

	auto* ctx = static_cast<MSpaceContext*>(msp);

	MSpaceInternalShutdown(*ctx);

	return true;
}

void* MSpaceMalloc(mspace_t msp, size_t size)
{
	if (msp == nullptr)
	{
		return nullptr;
	}

	if ((size >> 32u) != 0)
	{
		return nullptr;
	}

	auto* ctx = static_cast<MSpaceContext*>(msp);

	return MSpaceInternalMalloc_align(*ctx, size, 32);
}

bool MSpaceFree(mspace_t msp, void* ptr)
{
	if (msp == nullptr)
	{
		return false;
	}

	auto* ctx = static_cast<MSpaceContext*>(msp);

	MSpaceInternalFree_align(*ctx, ptr);

	return true;
}

void* MSpaceRealloc(mspace_t msp, void* ptr, size_t size)
{
	if (msp == nullptr)
	{
		return nullptr;
	}

	if ((size >> 32u) != 0)
	{
		return nullptr;
	}

	auto* ctx = static_cast<MSpaceContext*>(msp);

	return MSpaceInternalRealloc_align(*ctx, ptr, size, 32);
}

} // namespace Kyty::Core
