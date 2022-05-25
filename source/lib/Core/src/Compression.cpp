#include "Kyty/Core/Compression.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/MemoryAlloc.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Sys/SysTimer.h"

extern "C" {
#include "LzmaDec.h"
#include "LzmaEnc.h"
}

// IWYU pragma: no_include "7zTypes.h"

#if KYTY_COMPILER == KYTY_COMPILER_MSVC
#define MINIZ_NO_STDIO
#define MINIZ_NO_MALLOC
#define MZ_ASSERT(x) ASSERT(x)
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wunused-value"
#include "miniz.c.h"
//#pragma GCC diagnostic pop
#else
#define MINIZ_NO_STDIO
#define MINIZ_NO_MALLOC
#define MZ_ASSERT(x) ASSERT(x)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#if KYTY_COMPILER == KYTY_COMPILER_MINGW
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
//#pragma GCC diagnostic ignored "-Wenum-compare"
#include "miniz.c.h"
#pragma GCC diagnostic pop
#endif

#include "zstd.h"

namespace Kyty::Core {

namespace LzmaImpl {

struct InStream
{
	ISeqInStream t {};
	Core::File   mem_file;

	void Init(const uint8_t* buf, uint32_t size);
	void Close();
};

struct OutStream
{
	ISeqOutStream    t {};
	Core::ByteBuffer buf;

	void Init(uint32_t initial_size);
};

SRes Read(void* p, void* buf, size_t* size)
{
	SRes  result = SZ_OK;
	auto* s      = static_cast<InStream*>(p);

	EXIT_IF(!p);
	EXIT_IF(!buf);
	EXIT_IF(!size);

	uint64_t s64 = *size;

	EXIT_IF((s64 >> 32u) != 0);

	uint32_t br = 0;
	s->mem_file.Read(buf, s64, &br);

	//	if (br != s64)
	//	{
	//		result = SZ_ERROR_READ;
	//	}

	*size = br;

	return result;
}

size_t Write(void* p, const void* buf, size_t size)
{
	auto* s = static_cast<OutStream*>(p);

	EXIT_IF(!p);
	EXIT_IF(!buf);
	EXIT_IF(size == 0);

	uint64_t s64 = size;

	EXIT_IF((s64 >> 32u) != 0);

	s->buf.Add(static_cast<const Byte*>(buf), s64);

	return s64;
}

SRes Progress(void* /*p*/, UInt64 /*inSize*/, UInt64 /*outSize*/)
{
	return SZ_OK;
}

void* Alloc(void* /*p*/, size_t size)
{
	return Core::mem_alloc(size);
}

void Free(void* /*p*/, void* address)
{
	Core::mem_free(address);
}

void InStream::Init(const uint8_t* buf, uint32_t size)
{
	this->t.Read = Read;
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
	this->mem_file.OpenInMem(const_cast<uint8_t*>(buf), size);
}

void InStream::Close()
{
	this->mem_file.Close();
}

void OutStream::Init(uint32_t initial_size)
{
	this->t.Write = Write;
	this->buf.Expand(initial_size);
}

static ICompressProgress g_progress_callback = {&LzmaImpl::Progress};
static ISzAlloc          g_alloc_lzma        = {&LzmaImpl::Alloc, &LzmaImpl::Free};

} // namespace LzmaImpl

namespace ZipImpl {
void* Alloc(void* /*opaque*/, size_t items, size_t size)
{
	return Core::mem_alloc(items * size);
}

void Free(void* /*opaque*/, void* address)
{
	Core::mem_free(address);
}

void* Realloc(void* /*opaque*/, void* address, size_t items, size_t size)
{
	return Core::mem_realloc(address, items * size);
}

size_t Read(void* opaque, mz_uint64 file_ofs, void* buf, size_t n)
{
	File* f = static_cast<File*>(opaque);
	f->Seek(file_ofs);
	uint32_t b = 0;
	EXIT_IF(sizeof(size_t) > 4 && (uint64_t(n) >> 32u) > 0);
	auto nn = static_cast<uint32_t>(n);
	f->Read(buf, nn, &b);
	return b;
}

size_t Write(void* opaque, mz_uint64 file_ofs, const void* buf, size_t n)
{
	File* f = static_cast<File*>(opaque);
	f->Seek(file_ofs);
	uint32_t b = 0;
	EXIT_IF(sizeof(size_t) > 4 && (uint64_t(n) >> 32u) > 0);
	auto nn = static_cast<uint32_t>(n);
	f->Write(buf, nn, &b);
	return b;
}

} // namespace ZipImpl

constexpr uint32_t HASH_LOG  = 12u;
constexpr uint32_t HASH_SIZE = (1u << HASH_LOG);
constexpr uint32_t HASH_MASK = (HASH_SIZE - 1u);

static void UPDATE_HASH(uint32_t* v, const uint8_t* p)
{
	(*v) = *(reinterpret_cast<const uint16_t*>(p));
	(*v) ^= *(reinterpret_cast<const uint16_t*>((p) + 1u)) ^ ((*v) >> (16u - HASH_LOG));
}

constexpr int32_t MAX_COPY     = 32;
constexpr int32_t MAX_LEN      = 264; /* 256 + 8 */
constexpr int32_t MAX_DISTANCE = 8192;

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static uint32_t lzf_calc_compressed_size(const void* input, uint32_t length)
{
	const auto*    ip       = static_cast<const uint8_t*>(input);
	const uint8_t* ip_limit = ip + length - MAX_COPY - 4;
	uint32_t       opl      = 0;

	const uint8_t*  htab[HASH_SIZE];
	const uint8_t** hslot = nullptr;
	uint32_t        hval  = 0;

	const uint8_t* ref  = nullptr;
	int32_t        copy = 0;
	int32_t        len  = 0;
	// int32_t distance;
	const uint8_t* anchor = nullptr;

	for (hslot = htab; hslot < htab + HASH_SIZE; hslot++)
	{
		*hslot = ip;
	}

	copy = 0;
	opl++;

	while (ip < ip_limit)
	{
		UPDATE_HASH(&hval, ip);
		hslot = htab + (hval & HASH_MASK);
		ref   = /*(uint8_t*)*/ *hslot;

		*hslot = ip;

		if ((ip == ref) || (*(reinterpret_cast<const uint16_t*>(ref)) != *(reinterpret_cast<const uint16_t*>(ip))) || (ref[2] != ip[2]) ||
		    ((ip - ref) >= MAX_DISTANCE))
		{
			ip++;
			opl++;
			copy++;
			if (copy >= MAX_COPY)
			{
				copy = 0;
				opl++;
			}
			continue;
		}

		anchor = /*(uint8_t*)*/ ip;
		len    = 3;
		ref += 3;
		ip += 3;

		if (ip < ip_limit - MAX_LEN)
		{
			while (len < MAX_LEN - 8)
			{
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				len += 8;
			}
			ip--;
		}
		len = static_cast<int32_t>(ip - anchor);

		ip = anchor + len;

		if (copy != 0)
		{
			// anchor = anchor - copy - 1;
			copy = 0;
		} else
		{
			opl--;
		}

		len -= 2;

		// distance--;

		if (len < 7)
		{
			opl++;
		} else
		{
			opl += 2;
		}

		opl += 2;

		--ip;
		UPDATE_HASH(&hval, ip);
		htab[hval & HASH_MASK] = ip;
		ip++;
	}

	ip_limit = static_cast<const uint8_t*>(input) + length;
	while (ip < ip_limit)
	{
		ip++;
		opl++;
		copy++;
		if (copy == MAX_COPY)
		{
			copy = 0;
			opl++;
		}
	}

	if (copy == 0)
	{
		opl--;
	}

	return opl;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static uint32_t lzf_compress(const void* input, uint32_t length, void* output)
{
	const auto*    ip       = static_cast<const uint8_t*>(input);
	const uint8_t* ip_limit = ip + length - MAX_COPY - 4;
	auto*          op       = static_cast<uint8_t*>(output);

	const uint8_t*  htab[HASH_SIZE];
	const uint8_t** hslot = nullptr;
	uint32_t        hval  = 0;

	const uint8_t* ref      = nullptr;
	int32_t        copy     = 0;
	int32_t        len      = 0;
	int32_t        distance = 0;
	const uint8_t* anchor   = nullptr;

	for (hslot = htab; hslot < htab + HASH_SIZE; hslot++)
	{
		*hslot = ip;
	}

	copy  = 0;
	*op++ = MAX_COPY - 1;

	while (ip < ip_limit)
	{
		UPDATE_HASH(&hval, ip);
		hslot = htab + (hval & HASH_MASK);
		ref   = /*(uint8_t*)*/ *hslot;

		*hslot = ip;

		distance = static_cast<int32_t>(ip - ref);

		if ((ip == ref) || (*(reinterpret_cast<const uint16_t*>(ref)) != *(reinterpret_cast<const uint16_t*>(ip))) || (ref[2] != ip[2]) ||
		    (distance >= MAX_DISTANCE))
		{
			*op++ = *ip++;
			copy++;
			if (copy >= MAX_COPY)
			{
				copy  = 0;
				*op++ = MAX_COPY - 1;
			}
			continue;
		}

		anchor = /*(uint8_t*)*/ ip;
		len    = 3;
		ref += 3;
		ip += 3;

		if (ip < ip_limit - MAX_LEN)
		{
			while (len < MAX_LEN - 8)
			{
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				if (*ref++ != *ip++)
				{
					break;
				}
				len += 8;
			}
			ip--;
		}
		len = static_cast<int32_t>(ip - anchor);

		ip = anchor + len;

		if (copy != 0)
		{
			// anchor = anchor - copy - 1;
			*(op - copy - 1) = copy - 1;
			copy             = 0;
		} else
		{
			op--;
		}

		len -= 2;

		distance--;

		if (len < 7)
		{
			*op++ = (static_cast<uint32_t>(len) << 5u) + (static_cast<uint32_t>(distance) >> 8u);
		} else
		{
			*op++ = (7u << 5u) + (static_cast<uint32_t>(distance) >> 8u);
			*op++ = len - 7;
		}

		*op++ = (static_cast<uint32_t>(distance) & 255u);

		*op++ = MAX_COPY - 1;

		--ip;
		UPDATE_HASH(&hval, ip);
		htab[hval & HASH_MASK] = ip;
		ip++;
	}

	ip_limit = static_cast<const uint8_t*>(input) + length;
	while (ip < ip_limit)
	{
		*op++ = *ip++;
		copy++;
		if (copy == MAX_COPY)
		{
			copy  = 0;
			*op++ = MAX_COPY - 1;
		}
	}

	if (copy != 0)
	{
		*(op - copy - 1) = copy - 1;
	} else
	{
		op--;
	}

	return op - static_cast<uint8_t*>(output);
}

static uint32_t lzf_calc_decompressed_size(const void* input, uint32_t length)
{
	const auto*    ip       = static_cast<const uint8_t*>(input);
	const uint8_t* ip_limit = ip + length - 1;
	uint32_t       opl      = 0;

	while (ip < ip_limit)
	{
		uint32_t ctrl = (*ip) + 1;
		uint32_t len  = (*ip++) >> 5u;

		if (ctrl < 33)
		{
			if (ctrl != 0u)
			{
				ip++;
				opl++;
				ctrl--;

				if (ctrl != 0u)
				{
					ip++;
					opl++;
					ctrl--;

					if (ctrl != 0u)
					{
						ip++;
						opl++;
						ctrl--;

						for (; ctrl != 0u; ctrl--)
						{
							ip++;
							opl++;
						}
					}
				}
			}
		} else
		{
			len--;

			if (len == 7 - 1)
			{
				len += *ip++;
			}

			ip++;

			opl += 3;
			if (len != 0u)
			{
				for (; len != 0u; --len)
				{
					opl++;
				}
			}
		}
	}

	return opl;
}

static uint32_t lzf_decompress(const void* input, uint32_t length, void* output, uint32_t maxout)
{
	const auto*    ip       = static_cast<const uint8_t*>(input);
	const uint8_t* ip_limit = ip + length - 1;
	auto*          op       = static_cast<uint8_t*>(output);
	uint8_t*       op_limit = op + maxout;
	uint8_t*       ref      = nullptr;

	while (ip < ip_limit)
	{
		uint32_t ctrl = (*ip) + 1;
		uint32_t ofs  = ((*ip) & 31u) << 8u;
		uint32_t len  = (*ip++) >> 5u;

		if (ctrl < 33)
		{
			if (op + ctrl > op_limit)
			{
				return 0;
			}

			if (ctrl != 0u)
			{
				*op++ = *ip++;
				ctrl--;

				if (ctrl != 0u)
				{
					*op++ = *ip++;
					ctrl--;

					if (ctrl != 0u)
					{
						*op++ = *ip++;
						ctrl--;

						for (; ctrl != 0u; ctrl--)
						{
							*op++ = *ip++;
						}
					}
				}
			}
		} else
		{
			len--;
			ref = op - ofs;
			ref--;

			if (len == 7 - 1)
			{
				len += *ip++;
			}

			ref -= *ip++;

			if (op + len + 3 > op_limit)
			{
				return 0;
			}

			if (ref < static_cast<uint8_t*>(output))
			{
				return 0;
			}

			*op++ = *ref++;
			*op++ = *ref++;
			*op++ = *ref++;
			if (len != 0u)
			{
				for (; len != 0u; --len)
				{
					*op++ = *ref++;
				}
			}
		}
	}

	return op - static_cast<uint8_t*>(output);
}

ByteBuffer CompressLzma(const uint8_t* buf, uint32_t length)
{
	CLzmaEncHandle enc = LzmaEnc_Create(&LzmaImpl::g_alloc_lzma);

	EXIT_IF(!enc);
	EXIT_IF(!buf);
	EXIT_IF(length == 0);

	CLzmaEncProps props;
	LzmaEncProps_Init(&props);

	[[maybe_unused]] SRes res = LzmaEnc_SetProps(enc, &props);

	EXIT_IF(res != SZ_OK);

	LzmaImpl::InStream  in_stream {};
	LzmaImpl::OutStream out_stream {};

	EXIT_IF(reinterpret_cast<ISeqOutStream*>(&out_stream) != &out_stream.t);
	EXIT_IF(reinterpret_cast<ISeqInStream*>(&in_stream) != &in_stream.t);

	uint8_t header[LZMA_PROPS_SIZE + 8];

	SizeT size = LZMA_PROPS_SIZE;
	res        = LzmaEnc_WriteProperties(enc, header, &size);

	EXIT_IF(res != SZ_OK || size != LZMA_PROPS_SIZE);

	uint64_t length64 = length;

	std::memcpy(header + LZMA_PROPS_SIZE, &length64, 8);

	in_stream.Init(buf, length);
	out_stream.Init(0);

	out_stream.t.Write(&out_stream.t, header, LZMA_PROPS_SIZE + 8);

	res =
	    LzmaEnc_Encode(enc, &out_stream.t, &in_stream.t, &LzmaImpl::g_progress_callback, &LzmaImpl::g_alloc_lzma, &LzmaImpl::g_alloc_lzma);

	EXIT_IF(res != SZ_OK);

	LzmaEnc_Destroy(enc, &LzmaImpl::g_alloc_lzma, &LzmaImpl::g_alloc_lzma);

	in_stream.Close();

	return out_stream.buf;
}

ByteBuffer CompressLzma(const ByteBuffer& buf)
{
	return CompressLzma(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

ByteBuffer CompressLzma(const String& str)
{
	String::Utf8 utf8 = str.utf8_str();
	return CompressLzma(reinterpret_cast<const uint8_t*>(utf8.GetDataConst()), utf8.Size());
}

constexpr size_t LZMA_IN_BUF_SIZE  = (1u << 16u);
constexpr size_t LZMA_OUT_BUF_SIZE = (1u << 16u);

static SRes Decode2(CLzmaDec* state, ISeqOutStream* out_stream, ISeqInStream* in_stream, UInt64 unpack_size)
{
	uint8_t in_buf[LZMA_IN_BUF_SIZE];
	uint8_t out_buf[LZMA_OUT_BUF_SIZE];
	size_t  in_pos  = 0;
	size_t  in_size = 0;
	size_t  out_pos = 0;
	LzmaDec_Init(state);
	for (;;)
	{
		if (in_pos == in_size)
		{
			in_size = LZMA_IN_BUF_SIZE;
			RINOK(in_stream->Read(in_stream, in_buf, &in_size));
			in_pos = 0;
		}
		{
			SRes            res           = 0;
			SizeT           in_processed  = in_size - in_pos;
			SizeT           out_processed = LZMA_OUT_BUF_SIZE - out_pos;
			ELzmaFinishMode finish_mode   = LZMA_FINISH_ANY;
			ELzmaStatus     status        = LZMA_STATUS_NOT_SPECIFIED;
			if (out_processed > unpack_size)
			{
				out_processed = static_cast<SizeT>(unpack_size);
				finish_mode   = LZMA_FINISH_END;
			}

			res = LzmaDec_DecodeToBuf(state, out_buf + out_pos, &out_processed, in_buf + in_pos, &in_processed, finish_mode, &status);
			in_pos += in_processed;
			out_pos += out_processed;
			unpack_size -= out_processed;

			if (out_stream != nullptr)
			{
				if (out_stream->Write(out_stream, out_buf, out_pos) != out_pos)
				{
					return SZ_ERROR_WRITE;
				}
			}

			out_pos = 0;

			if (res != SZ_OK || (unpack_size == 0))
			{
				return res;
			}

			if (in_processed == 0 && out_processed == 0)
			{
				return SZ_ERROR_DATA;
			}
		}
	}

	return SZ_OK;
}

ByteBuffer DecompressLzma(const uint8_t* buf, uint32_t length)
{
	EXIT_IF(!buf);
	EXIT_IF(length == 0);

	CLzmaDec dec;
	LzmaDec_Construct(&dec);

	LzmaImpl::InStream  in_stream {};
	LzmaImpl::OutStream out_stream {};

	in_stream.Init(buf, length);

	EXIT_IF(reinterpret_cast<ISeqOutStream*>(&out_stream) != &out_stream.t);
	EXIT_IF(reinterpret_cast<ISeqInStream*>(&in_stream) != &in_stream.t);

	uint8_t header[LZMA_PROPS_SIZE + 8];

	size_t size = LZMA_PROPS_SIZE + 8;
	in_stream.t.Read(&in_stream.t, header, &size);

	EXIT_IF(size != LZMA_PROPS_SIZE + 8);

	uint64_t length64 = 0;

	std::memcpy(&length64, header + LZMA_PROPS_SIZE, 8);

	EXIT_IF((length64 >> 32u) != 0);

	out_stream.Init(length64);

	[[maybe_unused]] SRes res = LzmaDec_Allocate(&dec, header, LZMA_PROPS_SIZE, &LzmaImpl::g_alloc_lzma);

	EXIT_IF(res != SZ_OK);

	res = Decode2(&dec, &out_stream.t, &in_stream.t, length64);

	EXIT_IF(res != SZ_OK);

	LzmaDec_Free(&dec, &LzmaImpl::g_alloc_lzma);

	in_stream.Close();

	return out_stream.buf;
}

ByteBuffer DecompressLzma(const ByteBuffer& buf)
{
	return DecompressLzma(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

String DecompressLzmaStr(const uint8_t* buf, uint32_t length)
{
	ByteBuffer utf8 = DecompressLzma(buf, length);
	EXIT_IF(utf8.At(utf8.Size() - 1) != (Byte)0);
	return String::FromUtf8(reinterpret_cast<const char*>(utf8.GetDataConst()));
}

String DecompressLzmaStr(const ByteBuffer& buf)
{
	return DecompressLzmaStr(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

constexpr uint32_t ZIP_OUT_BUF_SIZE = (16 * 1024);

ByteBuffer CompressZip(const uint8_t* buf, uint32_t length, ZipCompressLevel level)
{
	int       status = 0;
	mz_stream stream;
	memset(&stream, 0, sizeof(stream));

	ByteBuffer out;
	uint8_t    temp_buf[ZIP_OUT_BUF_SIZE];

	stream.next_in  = buf;
	stream.avail_in = length;
	stream.zalloc   = ZipImpl::Alloc;
	stream.zfree    = ZipImpl::Free;

	status = mz_deflateInit(&stream, level);

	EXIT_IF(status != MZ_OK);

	if (status == MZ_OK)
	{
		for (;;)
		{
			stream.next_out  = temp_buf;
			stream.avail_out = ZIP_OUT_BUF_SIZE;

			status = mz_deflate(&stream, MZ_FINISH);

			EXIT_IF(status != MZ_OK && status != MZ_STREAM_END);

			out.Add(reinterpret_cast<Byte*>(temp_buf), ZIP_OUT_BUF_SIZE - stream.avail_out);

			if (status != MZ_OK)
			{
				break;
			}
		}
	}

	EXIT_IF(stream.total_out != out.Size());

	mz_deflateEnd(&stream);

	return out;
}

ByteBuffer CompressZip(const ByteBuffer& buf, ZipCompressLevel level)
{
	return CompressZip(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size(), level);
}

ByteBuffer CompressZip(const String& str, ZipCompressLevel level)
{
	String::Utf8 utf8 = str.utf8_str();
	return CompressZip(reinterpret_cast<const uint8_t*>(utf8.GetDataConst()), utf8.Size(), level);
}

ByteBuffer DecompressZip(const uint8_t* buf, uint32_t length)
{
	int       status = 0;
	mz_stream stream;
	memset(&stream, 0, sizeof(stream));

	ByteBuffer out;
	uint8_t    temp_buf[ZIP_OUT_BUF_SIZE];

	stream.next_in  = buf;
	stream.avail_in = length;
	stream.zalloc   = ZipImpl::Alloc;
	stream.zfree    = ZipImpl::Free;

	status = mz_inflateInit(&stream);

	EXIT_IF(status != MZ_OK);

	if (status == MZ_OK)
	{
		for (;;)
		{
			stream.next_out  = temp_buf;
			stream.avail_out = ZIP_OUT_BUF_SIZE;

			status = mz_inflate(&stream, MZ_NO_FLUSH);

			EXIT_IF(status != MZ_OK && status != MZ_STREAM_END);

			out.Add(reinterpret_cast<Byte*>(temp_buf), ZIP_OUT_BUF_SIZE - stream.avail_out);

			if (status != MZ_OK)
			{
				break;
			}
		}
	}

	EXIT_IF(stream.total_out != out.Size());

	mz_deflateEnd(&stream);

	return out;
}

ByteBuffer DecompressZip(const ByteBuffer& buf)
{
	return DecompressZip(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

String DecompressZipStr(const uint8_t* buf, uint32_t length)
{
	ByteBuffer utf8 = DecompressZip(buf, length);
	EXIT_IF(utf8.At(utf8.Size() - 1) != Byte(0));
	return String::FromUtf8(reinterpret_cast<const char*>(utf8.GetDataConst()));
}

String DecompressZipStr(const ByteBuffer& buf)
{
	return DecompressZipStr(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

ByteBuffer CompressLzf(const uint8_t* buf, uint32_t length)
{
	uint32_t   size = lzf_calc_compressed_size(buf, length);
	ByteBuffer b(size * 2);
	size = lzf_compress(buf, length, b.GetData());

	KYTY_MEM_CHECK(b.GetDataConst());

	EXIT_IF(size > b.Size());

	if (size != b.Size())
	{
		b.RemoveAt(size, b.Size() - size);
	}

	return b;
}

ByteBuffer CompressLzf(const ByteBuffer& buf)
{
	return CompressLzf(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

ByteBuffer CompressLzf(const String& str)
{
	String::Utf8 utf8 = str.utf8_str();
	return CompressLzf(reinterpret_cast<const uint8_t*>(utf8.GetDataConst()), utf8.Size());
}

ByteBuffer DecompressLzf(const uint8_t* buf, uint32_t length)
{
	[[maybe_unused]] uint32_t size = lzf_calc_decompressed_size(buf, length);
	ByteBuffer                b(size);
	size = lzf_decompress(buf, length, b.GetData(), size);

	EXIT_IF(size != b.Size());

	return b;
}

ByteBuffer DecompressLzf(const ByteBuffer& buf)
{
	return DecompressLzf(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

String DecompressLzfStr(const uint8_t* buf, uint32_t length)
{
	ByteBuffer utf8 = DecompressLzf(buf, length);
	EXIT_IF(utf8.At(utf8.Size() - 1) != (Byte)0);
	return String::FromUtf8(reinterpret_cast<const char*>(utf8.GetDataConst()));
}

String DecompressLzfStr(const ByteBuffer& buf)
{
	return DecompressLzfStr(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

ByteBuffer CompressZstd(const uint8_t* buf, uint32_t length, int level)
{
	size_t dst_size = ZSTD_compressBound(length) * 2;
	auto*  dst      = new uint8_t[dst_size];
	dst_size        = ZSTD_compress(dst, dst_size, buf, length, level);
	if (ZSTD_isError(dst_size) != 0u)
	{
		EXIT("ZSTD: %s\n", ZSTD_getErrorName(dst_size));
	}
	ByteBuffer ret(dst, static_cast<uint32_t>(dst_size));
	DeleteArray(dst);
	return ret;
}

ByteBuffer CompressZstd(const ByteBuffer& buf, int level)
{
	return CompressZstd(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size(), level);
}

ByteBuffer CompressZstd(const String& str, int level)
{
	String::Utf8 utf8 = str.utf8_str();
	return CompressZstd(reinterpret_cast<const uint8_t*>(utf8.GetDataConst()), utf8.Size(), level);
}

ByteBuffer DecompressZstd(const uint8_t* buf, uint32_t length)
{
	ByteBuffer       r;
	size_t           buff_out_size = ZSTD_DStreamOutSize();
	Byte*            buff_out      = new Byte[buff_out_size];
	ZSTD_DCtx* const dctx          = ZSTD_createDCtx();
	ZSTD_inBuffer    input         = {buf, length, 0};
	while (input.pos < input.size)
	{
		ZSTD_outBuffer output = {buff_out, buff_out_size, 0};
		auto           ret    = ZSTD_decompressStream(dctx, &output, &input);
		if (ZSTD_isError(ret) != 0u)
		{
			EXIT("ZSTD: %s\n", ZSTD_getErrorName(ret));
		}
		r.Add(buff_out, static_cast<uint32_t>(output.pos));
	}
	ZSTD_freeDCtx(dctx);
	DeleteArray(buff_out);
	return r;
}

ByteBuffer DecompressZstd(const ByteBuffer& buf)
{
	return DecompressZstd(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

String DecompressZstdStr(const uint8_t* buf, uint32_t length)
{
	ByteBuffer utf8 = DecompressZstd(buf, length);
	EXIT_IF(utf8.At(utf8.Size() - 1) != (Byte)0);
	return String::FromUtf8(reinterpret_cast<const char*>(utf8.GetDataConst()));
}

String DecompressZstdStr(const ByteBuffer& buf)
{
	return DecompressZstdStr(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

struct ZipPrivate
{
	mz_zip_archive zip;
};

ZipReader::~ZipReader()
{
	Close();
}

bool ZipReader::Open(const String& file_name)
{
	Close();

	m_p = new ZipPrivate;
	memset(&m_p->zip, 0, sizeof(m_p->zip));
	m_p->zip.m_pRead    = ZipImpl::Read;
	m_p->zip.m_pWrite   = ZipImpl::Write;
	m_p->zip.m_pAlloc   = ZipImpl::Alloc;
	m_p->zip.m_pFree    = ZipImpl::Free;
	m_p->zip.m_pRealloc = ZipImpl::Realloc;

	File* f = new File;
	f->Open(file_name, File::Mode::Read);

	m_p->zip.m_pIO_opaque = f;

	if (f->IsInvalid() || (mz_zip_reader_init(&m_p->zip, f->Size(), 0) == 0))
	{
		Close();

		return false;
	}

	return true;
}

bool ZipReader::Open(const ByteBuffer& buf)
{
	Close();

	m_p = new ZipPrivate;
	memset(&m_p->zip, 0, sizeof(m_p->zip));
	m_p->zip.m_pRead    = ZipImpl::Read;
	m_p->zip.m_pWrite   = ZipImpl::Write;
	m_p->zip.m_pAlloc   = ZipImpl::Alloc;
	m_p->zip.m_pFree    = ZipImpl::Free;
	m_p->zip.m_pRealloc = ZipImpl::Realloc;

	File* f = new File;
	f->OpenInMem(const_cast<ByteBuffer&>(buf)); // NOLINT(cppcoreguidelines-pro-type-const-cast)

	m_p->zip.m_pIO_opaque = f;

	if (mz_zip_reader_init(&m_p->zip, f->Size(), 0) == 0)
	{
		Close();

		return false;
	}

	return true;
}

bool ZipReader::Open(uint8_t* mem, uint32_t size)
{
	Close();

	m_p = new ZipPrivate;
	memset(&m_p->zip, 0, sizeof(m_p->zip));
	m_p->zip.m_pRead    = ZipImpl::Read;
	m_p->zip.m_pWrite   = ZipImpl::Write;
	m_p->zip.m_pAlloc   = ZipImpl::Alloc;
	m_p->zip.m_pFree    = ZipImpl::Free;
	m_p->zip.m_pRealloc = ZipImpl::Realloc;

	File* f = new File;
	f->OpenInMem(mem, size);

	m_p->zip.m_pIO_opaque = f;

	if (mz_zip_reader_init(&m_p->zip, f->Size(), 0) == 0)
	{
		Close();

		return false;
	}

	return true;
}

void ZipReader::Close()
{
	if (m_p != nullptr)
	{
		mz_zip_reader_end(&m_p->zip);
		File* f = static_cast<File*>(m_p->zip.m_pIO_opaque);
		f->Close();
		Delete(f);
		Delete(m_p);
		m_p = nullptr;
	}
}

int ZipReader::GetNumFiles()
{
	EXIT_IF(!m_p);

	return static_cast<int>(mz_zip_reader_get_num_files(&m_p->zip));
}

bool ZipReader::GetFileStat(int file_index, ZipFileStat* o)
{
	EXIT_IF(!m_p);

	mz_zip_archive_file_stat s;

	if (mz_zip_reader_file_stat(&m_p->zip, file_index, &s) != 0)
	{
		o->m_file_index = s.m_file_index;

		SysTimeStruct at = {0};
		sys_time_t_to_system(s.m_time, at);
		o->m_time = DateTime(Date(at.Year, at.Month, at.Day), Time(at.Hour, at.Minute, at.Second, at.Milliseconds));

		o->m_crc32       = s.m_crc32;
		o->m_comp_size   = s.m_comp_size;
		o->m_uncomp_size = s.m_uncomp_size;
		o->m_filename    = String::FromUtf8(s.m_filename);
		o->m_comment     = String::FromUtf8(s.m_comment);

		return true;
	}

	return false;
}

bool ZipReader::IsFileDirectory(int file_index)
{
	EXIT_IF(!m_p);

	return mz_zip_reader_is_file_a_directory(&m_p->zip, file_index) != 0;
}

bool ZipReader::IsFileEncrypted(int file_index)
{
	EXIT_IF(!m_p);

	return mz_zip_reader_is_file_encrypted(&m_p->zip, file_index) != 0;
}

String ZipReader::GetFileName(int file_index)
{
	uint32_t   len = mz_zip_reader_get_filename(&m_p->zip, file_index, nullptr, 0);
	ByteBuffer buf(len);
	mz_zip_reader_get_filename(&m_p->zip, file_index, reinterpret_cast<char*>(buf.GetData()), len);
	EXIT_IF(buf.At(buf.Size() - 1) != (Byte)0);
	return String::FromUtf8(reinterpret_cast<const char*>(buf.GetDataConst()));
}

int ZipReader::FindFile(const String& name, const String& comment)
{
	return mz_zip_reader_locate_file(&m_p->zip, name.C_Str(), comment.C_Str(), 0);
}

ByteBuffer ZipReader::ExtractFile(int file_index)
{
	size_t size = 0;

	if (file_index < 0 || IsFileDirectory(file_index))
	{
		return {};
	}

	ZipFileStat s {};
	GetFileStat(file_index, &s);

	if (s.m_uncomp_size == 0)
	{
		return {};
	}

	void* ptr = mz_zip_reader_extract_to_heap(&m_p->zip, file_index, &size, 0);

	EXIT_IF(!ptr || !size);

	EXIT_IF(sizeof(size_t) > 4 && (uint64_t(size) >> 32u) > 0);
	auto nn = static_cast<uint32_t>(size);

	ByteBuffer buf(nn);
	std::memcpy(buf.GetData(), ptr, size);

	m_p->zip.m_pFree(nullptr, ptr);

	return buf;
}

ByteBuffer ZipReader::ExtractFile(const String& name)
{
	return ExtractFile(FindFile(name));
}

// ZipWriter::ZipWriter()
//{
//	m_p = nullptr;
//}

ZipWriter::~ZipWriter()
{
	Close();
}

bool ZipWriter::Create(const String& file_name)
{
	Close();

	m_p = new ZipPrivate;
	memset(&m_p->zip, 0, sizeof(m_p->zip));
	m_p->zip.m_pRead    = ZipImpl::Read;
	m_p->zip.m_pWrite   = ZipImpl::Write;
	m_p->zip.m_pAlloc   = ZipImpl::Alloc;
	m_p->zip.m_pFree    = ZipImpl::Free;
	m_p->zip.m_pRealloc = ZipImpl::Realloc;

	File* f = new File;
	f->Create(file_name);

	m_p->zip.m_pIO_opaque = f;

	if (f->IsInvalid() || (mz_zip_writer_init(&m_p->zip, 0) == 0))
	{
		Close();

		return false;
	}

	return true;
}

void ZipWriter::Close()
{
	if (m_p != nullptr)
	{
		mz_zip_writer_finalize_archive(&m_p->zip);
		mz_zip_writer_end(&m_p->zip);
		File* f = static_cast<File*>(m_p->zip.m_pIO_opaque);
		f->Close();
		Delete(f);
		Delete(m_p);
		m_p = nullptr;
	}
}

bool ZipWriter::AddFile(const String& file_name, const ByteBuffer& buf, ZipCompressLevel level)
{
	EXIT_IF(!m_p);

	if (m_p != nullptr)
	{
		return AddFile(file_name, reinterpret_cast<const uint8_t*>(buf.GetData()), buf.Size(), level);
	}
	return false;
}

bool ZipWriter::AddFile(const String& file_name, const uint8_t* buf, uint32_t size, ZipCompressLevel level)
{
	EXIT_IF(!m_p);
	EXIT_IF(!buf && size > 0);

	if (m_p != nullptr)
	{
		return mz_zip_writer_add_mem(&m_p->zip, file_name.C_Str(), buf, size, level) != 0;
	}
	return false;
}

bool ZipWriter::AddFileFromFile(const String& file_name, const String& from_file, ZipCompressLevel level)
{
	EXIT_IF(!m_p);

	if (m_p != nullptr)
	{
		File f;
		f.Open(from_file, File::Mode::Read);
		if (f.IsInvalid())
		{
			return false;
		}
		ByteBuffer buf = f.ReadWholeBuffer();
		f.Close();
		return AddFile(file_name, buf, level);
	}

	return false;
}

bool ZipWriter::AddFileFromReader(const String& /*file_name*/, ZipReader* from_reader, int file_index)
{
	EXIT_IF(!m_p);
	EXIT_IF(!from_reader);

	return mz_zip_writer_add_from_zip_reader(&m_p->zip, &from_reader->m_p->zip, file_index) != 0;
}

bool ZipWriter::AddDir(const String& dir_name)
{
	EXIT_IF(!m_p);

	String dir = dir_name.EndsWith(U"/") ? dir_name : dir_name + U"/";
	return AddFile(dir, nullptr, 0);
}

} // namespace Kyty::Core
