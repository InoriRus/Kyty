#include "Emulator/Graphics/Tile.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Threads.h"

#include "Emulator/Graphics/AsyncJob.h"
#include "Emulator/Profiler.h"

#if KYTY_COMPILER != KYTY_COMPILER_CLANG
#include <intrin.h>
#endif

#include <algorithm>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct Uint128
{
	uint64_t n[2];
};

struct Uint256
{
	Uint128 n[2];
};

class Tiler
{
public:
	Tiler(): m_job1(nullptr), m_job2(nullptr) /*, m_job3(nullptr), m_job4(nullptr)*/
	{
		EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());
	}
	virtual ~Tiler() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(Tiler);

	Core::Mutex m_mutex;

	AsyncJob m_job1;
	AsyncJob m_job2;
	// AsyncJob m_job3;
	// AsyncJob m_job4;
};

class Tiler32
{
public:
	uint32_t m_macro_tile_height = 0;
	uint32_t m_bank_height       = 0;
	uint32_t m_num_banks         = 0;
	uint32_t m_num_pipes         = 0;
	uint32_t m_padded_width      = 0;
	uint32_t m_padded_height     = 0;
	uint32_t m_pipe_bits         = 0;
	uint32_t m_bank_bits         = 0;

	void Init(uint32_t width, uint32_t height, bool neo)
	{
		m_macro_tile_height = (neo ? 128 : 64);
		m_bank_height       = neo ? 2 : 1;
		m_num_banks         = neo ? 8 : 16;
		m_num_pipes         = neo ? 16 : 8;
		m_padded_width      = width;
		if (height == 1080)
		{
			m_padded_height = neo ? 1152 : 1088;
		}
		if (height == 720)
		{
			m_padded_height = 768;
		}
		m_pipe_bits = neo ? 4 : 3;
		m_bank_bits = neo ? 3 : 4;
	}

	static uint32_t GetElementIndex(uint32_t x, uint32_t y)
	{
		uint32_t elem = 0;
		elem |= ((x >> 0u) & 0x1u) << 0u;
		elem |= ((x >> 1u) & 0x1u) << 1u;
		elem |= ((y >> 0u) & 0x1u) << 2u;
		elem |= ((x >> 2u) & 0x1u) << 3u;
		elem |= ((y >> 1u) & 0x1u) << 4u;
		elem |= ((y >> 2u) & 0x1u) << 5u;

		return elem;
	}

	static uint32_t GetPipeIndex(uint32_t x, uint32_t y, bool neo)
	{
		uint32_t pipe = 0;

		if (!neo)
		{
			pipe |= (((x >> 3u) ^ (y >> 3u) ^ (x >> 4u)) & 0x1u) << 0u;
			pipe |= (((x >> 4u) ^ (y >> 4u)) & 0x1u) << 1u;
			pipe |= (((x >> 5u) ^ (y >> 5u)) & 0x1u) << 2u;
		} else
		{
			pipe |= (((x >> 3u) ^ (y >> 3u) ^ (x >> 4u)) & 0x1u) << 0u;
			pipe |= (((x >> 4u) ^ (y >> 4u)) & 0x1u) << 1u;
			pipe |= (((x >> 5u) ^ (y >> 5u)) & 0x1u) << 2u;
			pipe |= (((x >> 6u) ^ (y >> 5u)) & 0x1u) << 3u;
		}

		return pipe;
	}

	static uint32_t IntLog2(uint32_t i)
	{
#if KYTY_COMPILER == KYTY_COMPILER_CLANG
		return 31 - __builtin_clz(i | 1u);
#else
		unsigned long temp;
		_BitScanReverse(&temp, i | 1u);
		return temp;
#endif
	}

	static uint32_t GetBankIndex(uint32_t x, uint32_t y, uint32_t bank_width, uint32_t bank_height, uint32_t num_banks, uint32_t num_pipes)
	{
		const uint32_t x_shift_offset = IntLog2(bank_width * num_pipes);
		const uint32_t y_shift_offset = IntLog2(bank_height);
		const uint32_t xs             = x >> x_shift_offset;
		const uint32_t ys             = y >> y_shift_offset;
		uint32_t       bank           = 0;
		switch (num_banks)
		{
			case 8:
				bank |= (((xs >> 3u) ^ (ys >> 5u)) & 0x1u) << 0u;
				bank |= (((xs >> 4u) ^ (ys >> 4u) ^ (ys >> 5u)) & 0x1u) << 1u;
				bank |= (((xs >> 5u) ^ (ys >> 3u)) & 0x1u) << 2u;
				break;
			case 16:
				bank |= (((xs >> 3u) ^ (ys >> 6u)) & 0x1u) << 0u;
				bank |= (((xs >> 4u) ^ (ys >> 5u) ^ (ys >> 6u)) & 0x1u) << 1u;
				bank |= (((xs >> 5u) ^ (ys >> 4u)) & 0x1u) << 2u;
				bank |= (((xs >> 6u) ^ (ys >> 3u)) & 0x1u) << 3u;
				break;
			default:;
		}

		return bank;
	}

	[[nodiscard]] uint64_t GetTiledOffset(uint32_t x, uint32_t y, bool neo) const
	{
		uint64_t element_index = GetElementIndex(x, y);

		uint32_t xh               = x;
		uint32_t yh               = y;
		uint64_t pipe             = GetPipeIndex(xh, yh, neo);
		uint64_t bank             = GetBankIndex(xh, yh, 1, m_bank_height, m_num_banks, m_num_pipes);
		uint32_t tile_bytes       = (8 * 8 * 32 + 7) / 8;
		uint64_t element_offset   = (element_index * 32);
		uint64_t tile_split_slice = 0;

		if (tile_bytes > 512)
		{
			tile_split_slice = element_offset / (512 * 8);
			element_offset %= (512 * 8);
			tile_bytes = 512;
		}

		uint64_t macro_tile_bytes        = (128 / 8) * (m_macro_tile_height / 8) * tile_bytes / (m_num_pipes * m_num_banks);
		uint64_t macro_tiles_per_row     = m_padded_width / 128;
		uint64_t macro_tile_row_index    = y / m_macro_tile_height;
		uint64_t macro_tile_column_index = x / 128;
		uint64_t macro_tile_index        = (macro_tile_row_index * macro_tiles_per_row) + macro_tile_column_index;
		uint64_t macro_tile_offset       = macro_tile_index * macro_tile_bytes;
		uint64_t macro_tiles_per_slice   = macro_tiles_per_row * (m_padded_height / m_macro_tile_height);
		uint64_t slice_bytes             = macro_tiles_per_slice * macro_tile_bytes;
		uint64_t slice_offset            = tile_split_slice * slice_bytes;
		uint64_t tile_row_index          = (y / 8) % m_bank_height;
		uint64_t tile_index              = tile_row_index;
		uint64_t tile_offset             = tile_index * tile_bytes;

		uint64_t tile_split_slice_rotation = ((m_num_banks / 2) + 1) * tile_split_slice;
		bank ^= tile_split_slice_rotation;
		bank &= (m_num_banks - 1);

		uint64_t total_offset = (slice_offset + macro_tile_offset + tile_offset) * 8 + element_offset;
		uint64_t bit_offset   = total_offset & 0x7u;
		total_offset /= 8;

		uint64_t pipe_interleave_offset = total_offset & 0xffu;
		uint64_t offset                 = total_offset >> 8u;
		uint64_t byte_offset =
		    pipe_interleave_offset | (pipe << (8u)) | (bank << (8u + m_pipe_bits)) | (offset << (8u + m_pipe_bits + m_bank_bits));

		return ((byte_offset << 3u) | bit_offset) / 8;
	}
};

class Tiler1d
{
public:
	uint32_t m_width            = 0;
	uint32_t m_height           = 0;
	uint32_t m_bits_per_element = 0;
	uint32_t m_tile_bytes       = 0;
	uint32_t m_tiles_per_row    = 0;

	void Init(uint32_t dfmt, uint32_t nfmt, uint32_t width, uint32_t height, uint32_t padded_width, uint32_t /*padded_height*/,
	          bool /*neo*/)
	{
		m_width  = width;
		m_height = height;

		if ((nfmt == 9 && dfmt == 10) || (nfmt == 0 && dfmt == 10))
		{
			// VK_FORMAT_R8G8B8A8_SRGB;
			m_bits_per_element = 32;
		} else if (nfmt == 9 && dfmt == 37)
		{
			// VK_FORMAT_BC3_SRGB_BLOCK;
			m_bits_per_element = 128;
			m_width            = std::max((m_width + 3) / 4, 1U);
			m_height           = std::max((m_height + 3) / 4, 1U);
		} else
		{
			EXIT("unknown format: nfmt = %u, dfmt = %u\n", nfmt, dfmt);
		}

		m_tile_bytes    = (8 * 8 * 1 * m_bits_per_element + 7) / 8;
		m_tiles_per_row = padded_width / 8;
	}

	static uint32_t GetElementIndex(uint32_t x, uint32_t y)
	{
		uint32_t elem = 0;
		elem |= ((x >> 0u) & 0x1u) << 0u;
		elem |= ((y >> 0u) & 0x1u) << 1u;
		elem |= ((x >> 1u) & 0x1u) << 2u;
		elem |= ((y >> 1u) & 0x1u) << 3u;
		elem |= ((x >> 2u) & 0x1u) << 4u;
		elem |= ((y >> 2u) & 0x1u) << 5u;
		return elem;
	}

	[[nodiscard]] uint64_t GetTiledOffset(uint32_t x, uint32_t y, bool /*neo*/) const
	{
		uint64_t element_index = GetElementIndex(x, y);

		uint64_t tile_row_index    = y / 8;
		uint64_t tile_column_index = x / 8;
		uint64_t tile_offset       = ((tile_row_index * m_tiles_per_row) + tile_column_index) * m_tile_bytes;
		uint64_t element_offset    = element_index * m_bits_per_element;
		uint64_t offset            = tile_offset * 8 + element_offset;
		return offset / 8;
	}
};

static Tiler* g_tiler = nullptr;

void TileInit()
{
	EXIT_IF(g_tiler != nullptr);

	g_tiler = new Tiler;
}

// NOLINTNEXTLINE(readability-non-const-parameter)
static void Detile32(const Tiler32* t, uint32_t width, uint32_t height, uint32_t dst_pitch, uint8_t* dst, const uint8_t* src, bool neo)
{
	EXIT_IF(g_tiler == nullptr);

	Core::LockGuard lock(g_tiler->m_mutex);

	struct DetileParams
	{
		const Tiler32* t;
		uint32_t       start_y;
		uint32_t       width;
		uint32_t       height;
		uint32_t       dst_pitch;
		uint8_t*       dst;
		const uint8_t* src;
		bool           neo;
	};

	auto func = [](void* args)
	{
		auto* p = static_cast<DetileParams*>(args);

		auto*          dst       = p->dst;
		const auto*    src       = p->src;
		const Tiler32* t         = p->t;
		uint32_t       start_y   = p->start_y;
		uint32_t       width     = p->width;
		uint32_t       height    = p->height;
		uint32_t       dst_pitch = p->dst_pitch;
		bool           neo       = p->neo;

		for (uint32_t y = start_y; y < height; y++)
		{
			uint32_t x             = 0;
			uint64_t linear_offset = y * dst_pitch * 4;

			for (; x + 1 < width; x += 2)
			{
				auto tiled_offset = t->GetTiledOffset(x, y, neo);

				*reinterpret_cast<uint64_t*>(dst + linear_offset) = *reinterpret_cast<const uint64_t*>(src + tiled_offset);
				linear_offset += 8;
			}
			if (x < width)
			{
				auto tiled_offset = t->GetTiledOffset(x, y, neo);

				*reinterpret_cast<uint32_t*>(dst + linear_offset) = *reinterpret_cast<const uint32_t*>(src + tiled_offset);
			}
		}
	};

	DetileParams p1 {t, 0, width, height / 4, dst_pitch, dst, src, neo};
	DetileParams p2 {t, p1.height, width, /*(height * 2) / 4*/ height, dst_pitch, dst, src, neo};
	// DetileParams p3 {t, p2.height, width, (height * 3) / 4, dst_pitch, dst, src, neo};
	// DetileParams p4 {t, p3.height, width, height, dst_pitch, dst, src, neo};

	g_tiler->m_job1.Execute(func, &p1);
	g_tiler->m_job2.Execute(func, &p2);
	// g_tiler->m_job3.Execute(func, &p3);
	// g_tiler->m_job4.Execute(func, &p4);

	g_tiler->m_job1.Wait();
	g_tiler->m_job2.Wait();
	// g_tiler->m_job3.Wait();
	// g_tiler->m_job4.Wait();

	//	Core::Thread t1(func, &p1);
	//	Core::Thread t2(func, &p2);
	//	Core::Thread t3(func, &p3);
	//	Core::Thread t4(func, &p4);
	//
	//	t1.Join();
	//	t2.Join();
	//	t3.Join();
	//	t4.Join();
}

static void Detile32(const Tiler1d* t, uint32_t width, uint32_t height, uint32_t dst_pitch, uint8_t* dst, const uint8_t* src, bool neo)
{
	for (uint32_t y = 0; y < height; y++)
	{
		uint32_t x             = 0;
		uint64_t linear_offset = y * dst_pitch * 4;

		for (; x + 1 < width; x += 2)
		{
			auto tiled_offset = t->GetTiledOffset(x, y, neo);

			*reinterpret_cast<uint64_t*>(dst + linear_offset) = *reinterpret_cast<const uint64_t*>(src + tiled_offset);
			linear_offset += 8;
		}
		if (x < width)
		{
			auto tiled_offset = t->GetTiledOffset(x, y, neo);

			*reinterpret_cast<uint32_t*>(dst + linear_offset) = *reinterpret_cast<const uint32_t*>(src + tiled_offset);
		}
	}
}

static void Detile128(const Tiler1d* t, uint32_t width, uint32_t height, uint32_t dst_pitch, uint8_t* dst, const uint8_t* src, bool neo)
{
	for (uint32_t y = 0; y < height; y++)
	{
		uint32_t x             = 0;
		uint64_t linear_offset = y * dst_pitch * 16;

		for (; x + 1 < width; x += 2)
		{
			auto tiled_offset = t->GetTiledOffset(x, y, neo);

			*reinterpret_cast<Uint256*>(dst + linear_offset) = *reinterpret_cast<const Uint256*>(src + tiled_offset);
			linear_offset += 32;
		}
		if (x < width)
		{
			auto tiled_offset = t->GetTiledOffset(x, y, neo);

			*reinterpret_cast<Uint128*>(dst + linear_offset) = *reinterpret_cast<const Uint128*>(src + tiled_offset);
		}
	}
}

static void Detile1d(const Tiler1d* t, uint8_t* dst, const uint8_t* src, bool neo)
{
	if (t->m_bits_per_element == 32)
	{
		Detile32(t, t->m_width, t->m_height, t->m_width, dst, src, neo);
	} else if (t->m_bits_per_element == 128)
	{
		Detile128(t, t->m_width, t->m_height, t->m_width, dst, src, neo);
	} else
	{
		EXIT("Unknown size");
	}
}

void TileConvertTiledToLinear(void* dst, const void* src, TileMode mode, uint32_t width, uint32_t height, bool neo)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(mode != TileMode::VideoOutTiled);

	Tiler32 t;
	t.Init(width, height, neo);

	Detile32(&t, width, height, width, static_cast<uint8_t*>(dst), static_cast<const uint8_t*>(src), neo);
}

void TileConvertTiledToLinear(void* dst, const void* src, TileMode mode, uint32_t dfmt, uint32_t nfmt, uint32_t width, uint32_t height,
                              uint32_t levels, bool neo)
{
	EXIT_NOT_IMPLEMENTED(mode != TileMode::TextureTiled);

	uint32_t padded_width[16]  = {0};
	uint32_t padded_height[16] = {0};
	uint32_t level_sizes[16]   = {0};

	TileGetTextureSize(dfmt, nfmt, width, height, width, levels, 13, neo, nullptr, level_sizes, padded_width, padded_height);

	uint32_t mip_width  = width;
	uint32_t mip_height = height;

	auto*       dstptr = static_cast<uint8_t*>(dst);
	const auto* srcptr = static_cast<const uint8_t*>(src);

	for (uint32_t l = 0; l < levels; l++)
	{
		Tiler1d t;
		t.Init(dfmt, nfmt, mip_width, mip_height, padded_width[l], padded_height[l], neo);

		Detile1d(&t, dstptr, srcptr, neo);

		dstptr += level_sizes[l];
		srcptr += level_sizes[l];

		if (mip_width > 1)
		{
			mip_width /= 2;
		}
		if (mip_height > 1)
		{
			mip_height /= 2;
		}
	}
}

bool TileGetDepthSize(uint32_t width, uint32_t height, uint32_t pitch, uint32_t z_format, uint32_t stencil_format, bool htile, bool neo,
                      TileSizeAlign* stencil_size, TileSizeAlign* htile_size, TileSizeAlign* depth_size)
{
	struct DepthInfo
	{
		uint32_t      width          = 0;
		uint32_t      height         = 0;
		uint32_t      z_format       = 0;
		uint32_t      stencil_format = 0;
		bool          tile           = false;
		bool          neo            = false;
		uint32_t      pitch          = 0;
		TileSizeAlign stencil        = {};
		TileSizeAlign htile          = {};
		TileSizeAlign depth          = {};
	};

	static const DepthInfo infos_base[] = {
	    {3840, 2160, 3, 0, true, false, 3840, {0, 0}, {655360, 2048}, {33423360, 32768}},
	    {3840, 2160, 3, 0, false, false, 3840, {0, 0}, {0, 0}, {33423360, 32768}},
	    {1920, 1080, 3, 0, true, false, 2048, {0, 0}, {196608, 2048}, {9437184, 32768}},
	    {1920, 1080, 3, 0, false, false, 2048, {0, 0}, {0, 0}, {9437184, 32768}},
	    {1280, 720, 3, 0, true, false, 1280, {0, 0}, {98304, 2048}, {3932160, 32768}},
	    {1280, 720, 3, 0, false, false, 1280, {0, 0}, {0, 0}, {3932160, 32768}},
	    {3840, 2160, 1, 0, true, false, 3840, {0, 0}, {655360, 2048}, {16711680, 32768}},
	    {3840, 2160, 1, 0, false, false, 3840, {0, 0}, {0, 0}, {16711680, 32768}},
	    {1920, 1080, 1, 0, true, false, 2048, {0, 0}, {196608, 2048}, {4718592, 32768}},
	    {1920, 1080, 1, 0, false, false, 2048, {0, 0}, {0, 0}, {4718592, 32768}},
	    {1280, 720, 1, 0, true, false, 1280, {0, 0}, {98304, 2048}, {1966080, 32768}},
	    {1280, 720, 1, 0, false, false, 1280, {0, 0}, {0, 0}, {1966080, 32768}},
	    {3840, 2160, 0, 1, true, false, 3840, {8355840, 32768}, {655360, 2048}, {0, 0}},
	    {3840, 2160, 0, 1, false, false, 3840, {8355840, 32768}, {0, 0}, {0, 0}},
	    {1920, 1080, 0, 1, true, false, 2048, {2359296, 32768}, {196608, 2048}, {0, 0}},
	    {1920, 1080, 0, 1, false, false, 2048, {2359296, 32768}, {0, 0}, {0, 0}},
	    {1280, 720, 0, 1, true, false, 1280, {983040, 32768}, {98304, 2048}, {0, 0}},
	    {1280, 720, 0, 1, false, false, 1280, {983040, 32768}, {0, 0}, {0, 0}},
	    {3840, 2160, 3, 1, true, false, 3840, {8355840, 32768}, {655360, 2048}, {33423360, 32768}},
	    {3840, 2160, 3, 1, false, false, 3840, {8355840, 32768}, {0, 0}, {33423360, 32768}},
	    {1920, 1080, 3, 1, true, false, 2048, {2359296, 32768}, {196608, 2048}, {9437184, 32768}},
	    {1920, 1080, 3, 1, false, false, 2048, {2359296, 32768}, {0, 0}, {9437184, 32768}},
	    {1280, 720, 3, 1, true, false, 1280, {983040, 32768}, {98304, 2048}, {3932160, 32768}},
	    {1280, 720, 3, 1, false, false, 1280, {983040, 32768}, {0, 0}, {3932160, 32768}},
	    {3840, 2160, 1, 1, true, false, 3840, {8355840, 32768}, {655360, 2048}, {16711680, 32768}},
	    {3840, 2160, 1, 1, false, false, 3840, {8355840, 32768}, {0, 0}, {16711680, 32768}},
	    {1920, 1080, 1, 1, true, false, 2048, {2359296, 32768}, {196608, 2048}, {4718592, 32768}},
	    {1920, 1080, 1, 1, false, false, 2048, {2359296, 32768}, {0, 0}, {4718592, 32768}},
	    {1280, 720, 1, 1, true, false, 1280, {983040, 32768}, {98304, 2048}, {1966080, 32768}},
	    {1280, 720, 1, 1, false, false, 1280, {983040, 32768}, {0, 0}, {1966080, 32768}},
	};

	static const DepthInfo infos_neo[] = {
	    {3840, 2160, 3, 0, true, true, 3840, {0, 0}, {655360, 4096}, {33423360, 65536}},
	    {3840, 2160, 3, 0, false, true, 3840, {0, 0}, {0, 0}, {33423360, 65536}},
	    {1920, 1080, 3, 0, true, true, 1920, {0, 0}, {196608, 4096}, {8847360, 65536}},
	    {1920, 1080, 3, 0, false, true, 1920, {0, 0}, {0, 0}, {8847360, 65536}},
	    {1920, 1080, 3, 0, true, true, 2048, {0, 0}, {196608, 4096}, {9437184, 65536}},
	    {1920, 1080, 3, 0, false, true, 2048, {0, 0}, {0, 0}, {9437184, 65536}},
	    {1280, 720, 3, 0, true, true, 1280, {0, 0}, {131072, 4096}, {3932160, 65536}},
	    {1280, 720, 3, 0, false, true, 1280, {0, 0}, {0, 0}, {3932160, 65536}},
	    {3840, 2160, 1, 0, true, true, 3840, {0, 0}, {655360, 4096}, {16711680, 65536}},
	    {3840, 2160, 1, 0, false, true, 3840, {0, 0}, {0, 0}, {16711680, 65536}},
	    {1920, 1080, 1, 0, true, true, 2048, {0, 0}, {196608, 4096}, {4718592, 65536}},
	    {1920, 1080, 1, 0, false, true, 2048, {0, 0}, {0, 0}, {4718592, 65536}},
	    {1280, 720, 1, 0, true, true, 1280, {0, 0}, {131072, 4096}, {1966080, 65536}},
	    {1280, 720, 1, 0, false, true, 1280, {0, 0}, {0, 0}, {1966080, 65536}},
	    {3840, 2160, 0, 1, true, true, 3840, {8355840, 32768}, {655360, 4096}, {0, 0}},
	    {3840, 2160, 0, 1, false, true, 3840, {8355840, 32768}, {0, 0}, {0, 0}},
	    {1920, 1080, 0, 1, true, true, 2048, {2359296, 32768}, {196608, 4096}, {0, 0}},
	    {1920, 1080, 0, 1, false, true, 2048, {2359296, 32768}, {0, 0}, {0, 0}},
	    {1280, 720, 0, 1, true, true, 1280, {983040, 32768}, {131072, 4096}, {0, 0}},
	    {1280, 720, 0, 1, false, true, 1280, {983040, 32768}, {0, 0}, {0, 0}},
	    {3840, 2160, 3, 1, true, true, 3840, {8355840, 32768}, {655360, 4096}, {33423360, 65536}},
	    {3840, 2160, 3, 1, false, true, 3840, {8355840, 32768}, {0, 0}, {33423360, 65536}},
	    {1920, 1080, 3, 1, true, true, 2048, {2359296, 32768}, {196608, 4096}, {9437184, 65536}},
	    {1920, 1080, 3, 1, false, true, 2048, {2359296, 32768}, {0, 0}, {9437184, 65536}},
	    {1280, 720, 3, 1, true, true, 1280, {983040, 32768}, {131072, 4096}, {3932160, 65536}},
	    {1280, 720, 3, 1, false, true, 1280, {983040, 32768}, {0, 0}, {3932160, 65536}},
	    {3840, 2160, 1, 1, true, true, 3840, {8355840, 32768}, {655360, 4096}, {16711680, 65536}},
	    {3840, 2160, 1, 1, false, true, 3840, {8355840, 32768}, {0, 0}, {16711680, 65536}},
	    {1920, 1080, 1, 1, true, true, 2048, {2359296, 32768}, {196608, 4096}, {4718592, 65536}},
	    {1920, 1080, 1, 1, false, true, 2048, {2359296, 32768}, {0, 0}, {4718592, 65536}},
	    {1280, 720, 1, 1, true, true, 1280, {983040, 32768}, {131072, 4096}, {1966080, 65536}},
	    {1280, 720, 1, 1, false, true, 1280, {983040, 32768}, {0, 0}, {1966080, 65536}},
	};

	EXIT_IF(depth_size == nullptr);
	EXIT_IF(htile_size == nullptr);
	EXIT_IF(stencil_size == nullptr);

	if (neo)
	{
		for (const auto& i: infos_neo)
		{
			if (i.width == width && i.height == height && i.pitch == pitch && i.tile == htile && i.z_format == z_format &&
			    i.stencil_format == stencil_format)
			{
				*depth_size   = i.depth;
				*htile_size   = i.htile;
				*stencil_size = i.stencil;
				return true;
			}
		}
	} else
	{
		for (const auto& i: infos_base)
		{
			if (i.width == width && i.height == height && i.pitch == pitch && i.tile == htile && i.z_format == z_format &&
			    i.stencil_format == stencil_format)
			{
				*depth_size   = i.depth;
				*htile_size   = i.htile;
				*stencil_size = i.stencil;
				return true;
			}
		}
	}
	*depth_size   = TileSizeAlign();
	*htile_size   = TileSizeAlign();
	*stencil_size = TileSizeAlign();
	return false;
}

void TileGetVideoOutSize(uint32_t width, uint32_t height, uint32_t pitch, bool tile, bool neo, TileSizeAlign* size)
{
	EXIT_IF(size == nullptr);

	uint32_t ret_size  = 0;
	uint32_t ret_align = 0;

	if (pitch == 3840)
	{
		if (width == 3840 && height == 2160 && tile && !neo)
		{
			ret_size  = 33423360;
			ret_align = 32768;
		}
		if (width == 3840 && height == 2160 && tile && neo)
		{
			ret_size  = 33423360;
			ret_align = 65536;
		}
		if (width == 3840 && height == 2160 && !tile && !neo)
		{
			ret_size  = 33177600;
			ret_align = 256;
		}
		if (width == 3840 && height == 2160 && !tile && neo)
		{
			ret_size  = 33177600;
			ret_align = 256;
		}
	}

	if (pitch == 1920)
	{
		if (width == 1920 && height == 1080 && tile && !neo)
		{
			ret_size  = 8355840;
			ret_align = 32768;
		}
		if (width == 1920 && height == 1080 && tile && neo)
		{
			ret_size  = 8847360;
			ret_align = 65536;
		}
		if (width == 1920 && height == 1080 && !tile && !neo)
		{
			ret_size  = 8294400;
			ret_align = 256;
		}
		if (width == 1920 && height == 1080 && !tile && neo)
		{
			ret_size  = 8294400;
			ret_align = 256;
		}
	}

	if (pitch == 1280)
	{
		if (width == 1280 && height == 720 && tile && !neo)
		{
			ret_size  = 3932160;
			ret_align = 32768;
		}
		if (width == 1280 && height == 720 && tile && neo)
		{
			ret_size  = 3932160;
			ret_align = 65536;
		}
		if (width == 1280 && height == 720 && !tile && !neo)
		{
			ret_size  = 3686400;
			ret_align = 256;
		}
		if (width == 1280 && height == 720 && !tile && neo)
		{
			ret_size  = 3686400;
			ret_align = 256;
		}
	}

	size->size  = ret_size;
	size->align = ret_align;
}

void TileGetTextureSize(uint32_t dfmt, uint32_t nfmt, uint32_t width, uint32_t height, uint32_t pitch, uint32_t levels, uint32_t tile,
                        bool neo, TileSizeAlign* total_size, uint32_t* level_sizes, uint32_t* padded_width, uint32_t* padded_height)
{
	KYTY_PROFILER_FUNCTION();

	struct Padded
	{
		uint32_t width  = 0;
		uint32_t height = 0;
	};

	struct TextureInfo
	{
		uint32_t      dfmt   = 0;
		uint32_t      nfmt   = 0;
		uint32_t      width  = 0;
		uint32_t      height = 0;
		uint32_t      pitch  = 0;
		uint32_t      levels = 0;
		uint32_t      tile   = 0;
		bool          neo    = false;
		TileSizeAlign size[16];
		Padded        padded[16];
	};

	static const TextureInfo infos[] = {
	    // clang-format off

			// kDataFormatB8G8R8A8UnormSrgb, 512, 512, 0, 0, kTileModeDisplay_LinearAligned
			{ 10, 9, 512, 512, 512, 10, 8, false, {{1048576, 256}, {262144, 256}, {65536, 256}, {16384, 256}, {4096, 256}, {1024, 256}, {512, 256}, {256, 256}, {256, 256}, {256, 256}, }, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},  } },
			{ 10, 9, 512, 512, 512, 10, 8, true, {{1048576, 256}, {262144, 256}, {65536, 256}, {16384, 256}, {4096, 256}, {1024, 256}, {512, 256}, {256, 256}, {256, 256}, {256, 256}, }, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},  } },
			// kDataFormatB8G8R8A8UnormSrgb, 512, 512, 0, 0, kTileModeThin_1dThin
			{ 10, 9, 512, 512, 512, 10, 13, false, {{1048576, 256}, {262144, 256}, {65536, 256}, {16384, 256}, {4096, 256}, {1024, 256}, {256, 256}, {256, 256}, {256, 256}, {256, 256}, }, { {512, 512}, {256, 256}, {128, 128}, {64, 64}, {32, 32}, {16, 16}, {8, 8}, {8, 8}, {8, 8}, {8, 8},  } },
			{ 10, 9, 512, 512, 512, 10, 13, true, {{1048576, 256}, {262144, 256}, {65536, 256}, {16384, 256}, {4096, 256}, {1024, 256}, {256, 256}, {256, 256}, {256, 256}, {256, 256}, }, { {512, 512}, {256, 256}, {128, 128}, {64, 64}, {32, 32}, {16, 16}, {8, 8}, {8, 8}, {8, 8}, {8, 8},  } },
			// kDataFormatBc3UnormSrgb, 512, 512, 0, 0, kTileModeThin_1dThin
			{ 37, 9, 512, 512, 512, 10, 13, false, {{262144, 256}, {65536, 256}, {16384, 256}, {4096, 256}, {1024, 256}, {1024, 256}, {1024, 256}, {1024, 256}, {1024, 256}, {1024, 256}, }, { {128, 128}, {64, 64}, {32, 32}, {16, 16}, {8, 8}, {8, 8}, {8, 8}, {8, 8}, {8, 8}, {8, 8},  } },
			{ 37, 9, 512, 512, 512, 10, 13, true, {{262144, 256}, {65536, 256}, {16384, 256}, {4096, 256}, {1024, 256}, {1024, 256}, {1024, 256}, {1024, 256}, {1024, 256}, {1024, 256}, }, { {128, 128}, {64, 64}, {32, 32}, {16, 16}, {8, 8}, {8, 8}, {8, 8}, {8, 8}, {8, 8}, {8, 8},  } },
			// kDataFormatB8G8R8A8Unorm, 512, 512, 0, 0, kTileModeThin_1dThin
			{ 10, 0, 512, 512, 512, 10, 13, false, {{1048576, 256}, {262144, 256}, {65536, 256}, {16384, 256}, {4096, 256}, {1024, 256}, {256, 256}, {256, 256}, {256, 256}, {256, 256}, }, { {512, 512}, {256, 256}, {128, 128}, {64, 64}, {32, 32}, {16, 16}, {8, 8}, {8, 8}, {8, 8}, {8, 8},  } },
			{ 10, 0, 512, 512, 512, 10, 13, true, {{1048576, 256}, {262144, 256}, {65536, 256}, {16384, 256}, {4096, 256}, {1024, 256}, {256, 256}, {256, 256}, {256, 256}, {256, 256}, }, { {512, 512}, {256, 256}, {128, 128}, {64, 64}, {32, 32}, {16, 16}, {8, 8}, {8, 8}, {8, 8}, {8, 8},  } },
			// kDataFormatB8G8R8A8Unorm, 512, 768, 0, 0, kTileModeThin_1dThin
			{ 10, 0, 512, 768, 512, 10, 13, false, {{1572864, 256}, {1048576, 256}, {131072, 256}, {32768, 256}, {8192, 256}, {2048, 256}, {512, 256}, {256, 256}, {256, 256}, {256, 256}, }, { {512, 768}, {256, 512}, {128, 256}, {64, 128}, {32, 64}, {16, 32}, {8, 16}, {8, 8}, {8, 8}, {8, 8},  } },
			{ 10, 0, 512, 768, 512, 10, 13, true, {{1572864, 256}, {1048576, 256}, {131072, 256}, {32768, 256}, {8192, 256}, {2048, 256}, {512, 256}, {256, 256}, {256, 256}, {256, 256}, }, { {512, 768}, {256, 512}, {128, 256}, {64, 128}, {32, 64}, {16, 32}, {8, 16}, {8, 8}, {8, 8}, {8, 8},  } },
			// kDataFormatB8G8R8A8Unorm, 256, 256, 0, 0, kTileModeThin_2dThin
			{ 10, 0, 256, 256, 256, 9, 14, false, {{262144, 32768}, {65536, 32768}, {16384, 32768}, {4096, 32768}, {1024, 32768}, {256, 32768}, {256, 32768}, {256, 32768}, {256, 32768}, }, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},  } },
			{ 10, 0, 256, 256, 256, 9, 14, true, {{262144, 65536}, {65536, 65536}, {16384, 65536}, {4096, 65536}, {1024, 65536}, {256, 65536}, {256, 65536}, {256, 65536}, {256, 65536}, }, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},  } },
			// kDataFormatB8G8R8A8Unorm, 1920, 1080, 1920, 1, kTileModeDisplay_2dThin
			{ 10, 0, 1920, 1080, 1920, 1, 10, false, {{8355840, 32768}, }, { {0, 0},  } },
			{ 10, 0, 1920, 1080, 1920, 1, 10, true, {{8847360, 65536}, }, { {0, 0},  } },
			// kDataFormatB8G8R8A8Unorm, 3840, 2160, 3840, 1, kTileModeDisplay_2dThin
			{ 10, 0, 3840, 2160, 3840, 1, 10, false, {{33423360, 32768}, }, { {0, 0},  } },
			{ 10, 0, 3840, 2160, 3840, 1, 10, true, {{33423360, 65536}, }, { {0, 0},  } },
			// kDataFormatR32Float, 1920, 1080, 1920, 1, kTileModeDepth_2dThin_256
			{ 4, 7, 1920, 1080, 1920, 1, 2, false, {{8355840, 32768}, }, { {0, 0},  } },
			{ 4, 7, 1920, 1080, 1920, 1, 2, true, {{8847360, 65536}, }, { {0, 0},  } },
			// kDataFormatR8Unorm, 2048, 2048, 0, 0, kTileModeDisplay_LinearAligned
			{ 1, 0, 2048, 2048, 2048, 12, 8, false, {{4194304, 256}, {1048576, 256}, {262144, 256}, {65536, 256}, {16384, 256}, {4096, 256}, {2048, 256}, {1024, 256}, {512, 256}, {256, 256}, {256, 256}, {256, 256}, }, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},  } },
			{ 1, 0, 2048, 2048, 2048, 12, 8, true, {{4194304, 256}, {1048576, 256}, {262144, 256}, {65536, 256}, {16384, 256}, {4096, 256}, {2048, 256}, {1024, 256}, {512, 256}, {256, 256}, {256, 256}, {256, 256}, }, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},  } },
			// kDataFormatB8G8R8A8Unorm, 1, 1, 64, 0, kTileModeDisplay_LinearAligned
			{ 10, 0, 1, 1, 64, 1, 8, false, {{256, 256}, }, { {0, 0},  } },
			{ 10, 0, 1, 1, 64, 1, 8, true, {{256, 256}, }, { {0, 0},  } },

	    // clang-format on
	};

	// EXIT_IF(total_size == nullptr);

	for (const auto& i: infos)
	{
		if (i.dfmt == dfmt && i.nfmt == nfmt && i.width == width && i.pitch == pitch && i.height == height && i.levels >= levels &&
		    i.tile == tile && i.neo == neo)
		{
			for (uint32_t l = 0; l < levels; l++)
			{
				if (total_size != nullptr)
				{
					total_size->size += i.size[l].size;
					total_size->align = i.size[l].align;
				}
				if (level_sizes != nullptr)
				{
					level_sizes[l] = i.size[l].size;
				}
				if (padded_width != nullptr)
				{
					padded_width[l] = i.padded[l].width;
				}
				if (padded_height != nullptr)
				{
					padded_height[l] = i.padded[l].height;
				}
			}
			return;
		}
	}

	if (tile == 8 && levels == 1 && ((dfmt == 10 && nfmt == 9) || (dfmt == 10 && nfmt == 0)))
	{
		uint32_t size = pitch * height * 4;
		if (total_size != nullptr)
		{
			total_size->size  = size;
			total_size->align = 256;
		}
		if (level_sizes != nullptr)
		{
			level_sizes[0] = size;
		}
	}
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
