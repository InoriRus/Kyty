#ifndef INCLUDE_KYTY_CORE_COMPRESSION_H_
#define INCLUDE_KYTY_CORE_COMPRESSION_H_

#include "Kyty/Core/ByteBuffer.h"
#include "Kyty/Core/Common.h"
#include "Kyty/Core/DateTime.h"
#include "Kyty/Core/String.h"

namespace Kyty::Core {

using ZipCompressLevel  = int;
using ZstdCompressLevel = int;

constexpr ZipCompressLevel ZIP_NO_COMPRESSION   = 0;
constexpr ZipCompressLevel ZIP_BEST_SPEED       = 1;
constexpr ZipCompressLevel ZIP_DEFAULT_LEVEL    = 6;
constexpr ZipCompressLevel ZIP_BEST_COMPRESSION = 9;

constexpr ZstdCompressLevel ZSTD_BEST_SPEED       = 1;
constexpr ZstdCompressLevel ZSTD_DEFAULT_LEVEL    = 3;
constexpr ZstdCompressLevel ZSTD_BEST_COMPRESSION = 22;

ByteBuffer CompressZstd(const uint8_t* buf, uint32_t length, int level = ZSTD_DEFAULT_LEVEL);
ByteBuffer CompressZstd(const ByteBuffer& buf, int level = ZSTD_DEFAULT_LEVEL);
ByteBuffer CompressZstd(const String& str, int level = ZSTD_DEFAULT_LEVEL);

ByteBuffer DecompressZstd(const uint8_t* buf, uint32_t length);
ByteBuffer DecompressZstd(const ByteBuffer& buf);
String     DecompressZstdStr(const uint8_t* buf, uint32_t length);
String     DecompressZstdStr(const ByteBuffer& buf);

ByteBuffer CompressLzma(const uint8_t* buf, uint32_t length);
ByteBuffer CompressLzma(const ByteBuffer& buf);
ByteBuffer CompressLzma(const String& str);

ByteBuffer DecompressLzma(const uint8_t* buf, uint32_t length);
ByteBuffer DecompressLzma(const ByteBuffer& buf);
String     DecompressLzmaStr(const uint8_t* buf, uint32_t length);
String     DecompressLzmaStr(const ByteBuffer& buf);

ByteBuffer CompressZip(const uint8_t* buf, uint32_t length, ZipCompressLevel level = ZIP_DEFAULT_LEVEL);
ByteBuffer CompressZip(const ByteBuffer& buf, ZipCompressLevel level = ZIP_DEFAULT_LEVEL);
ByteBuffer CompressZip(const String& str, ZipCompressLevel level = ZIP_DEFAULT_LEVEL);

ByteBuffer DecompressZip(const uint8_t* buf, uint32_t length);
ByteBuffer DecompressZip(const ByteBuffer& buf);
String     DecompressZipStr(const uint8_t* buf, uint32_t length);
String     DecompressZipStr(const ByteBuffer& buf);

ByteBuffer CompressLzf(const uint8_t* buf, uint32_t length);
ByteBuffer CompressLzf(const ByteBuffer& buf);
ByteBuffer CompressLzf(const String& str);

ByteBuffer DecompressLzf(const uint8_t* buf, uint32_t length);
ByteBuffer DecompressLzf(const ByteBuffer& buf);
String     DecompressLzfStr(const uint8_t* buf, uint32_t length);
String     DecompressLzfStr(const ByteBuffer& buf);

struct ZipFileStat
{
	uint32_t m_file_index;
	// uint32_t m_central_dir_ofs;
	// uint16_t m_version_made_by;
	// uint16_t m_version_needed;
	// uint16_t m_bit_flag;
	// uint16_t m_method;
	DateTime m_time;
	uint32_t m_crc32;
	uint64_t m_comp_size;
	uint64_t m_uncomp_size;
	// uint16_t m_internal_attr;
	// uint32_t m_external_attr;
	// uint64_t m_local_header_ofs;
	// uint32_t m_comment_size;
	String m_filename;
	String m_comment;
};

struct ZipPrivate;

class ZipReader
{
public:
	ZipReader() = default;
	virtual ~ZipReader();

	bool Open(const String& file_name);
	bool Open(const ByteBuffer& buf);
	bool Open(uint8_t* mem, uint32_t size);

	void Close();

	// Returns the total number of files in the archive.
	int GetNumFiles();

	// Returns detailed information about an archive file entry.
	bool GetFileStat(int file_index, ZipFileStat* out_stat);

	// Determines if an archive file entry is a directory entry.
	bool IsFileDirectory(int file_index);
	bool IsFileEncrypted(int file_index);

	// Retrieves the filename of an archive file entry.
	String GetFileName(int file_index);

	// Attempts to locates a file in the archive's central directory.
	// Returns -1 if the file cannot be found.
	int FindFile(const String& name, const String& comment = U"");

	// Extracts a archive file to a memory buffer
	ByteBuffer ExtractFile(int file_index);
	ByteBuffer ExtractFile(const String& name);

	friend class ZipWriter;

	KYTY_CLASS_NO_COPY(ZipReader);

private:
	ZipPrivate* m_p = nullptr;
};

class ZipWriter
{
public:
	ZipWriter() = default;
	virtual ~ZipWriter();

	bool Create(const String& file_name);

	void Close();

	bool AddFile(const String& file_name, const ByteBuffer& buf, ZipCompressLevel level = ZIP_DEFAULT_LEVEL);
	bool AddFile(const String& file_name, const uint8_t* buf, uint32_t size, ZipCompressLevel level = ZIP_DEFAULT_LEVEL);
	bool AddFileFromFile(const String& file_name, const String& from_file, ZipCompressLevel level = ZIP_DEFAULT_LEVEL);
	bool AddFileFromReader(const String& file_name, ZipReader* from_reader, int file_index);

	bool AddDir(const String& dir_name);

	KYTY_CLASS_NO_COPY(ZipWriter);

private:
	ZipPrivate* m_p = nullptr;
};

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_COMPRESSION_H_ */
