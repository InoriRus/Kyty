#ifndef INCLUDE_KYTY_MATH_CRYPTO_H_
#define INCLUDE_KYTY_MATH_CRYPTO_H_

#include "Kyty/Core/ByteBuffer.h"
#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"

namespace Kyty::Math {

namespace AES {
enum class Mode
{
	Cbc256Pkcs7Padding,
	Cbc256ZeroPadding
};

Core::ByteBuffer Encrypt(const uint8_t* buf, uint32_t length, const uint8_t* key, const uint8_t* iv, Mode mode);
Core::ByteBuffer Encrypt(const Core::ByteBuffer& buf, const uint8_t* key, const uint8_t* iv, Mode mode);
Core::ByteBuffer EncryptStr(const String& str, const uint8_t* key, const uint8_t* iv, Mode mode);

Core::ByteBuffer Decrypt(const uint8_t* buf, uint32_t length, const uint8_t* key, const uint8_t* iv, Mode mode);
Core::ByteBuffer Decrypt(const Core::ByteBuffer& buf, const uint8_t* key, const uint8_t* iv, Mode mode);
String           DecryptStr(const uint8_t* buf, uint32_t length, const uint8_t* key, const uint8_t* iv, Mode mode);
String           DecryptStr(const Core::ByteBuffer& buf, const uint8_t* key, const uint8_t* iv, Mode mode);

} // namespace AES

namespace MD5 {
/* MD5 context. */
struct CTX
{
	uint32_t state[4];   /* state (ABCD) */
	uint32_t count[2];   /* number of bits, modulo 2^64 (lsb first) */
	uint8_t  buffer[64]; /* input buffer */
};

void Init(CTX* context);
void Update(CTX* context, const uint8_t* input, uint32_t input_len);
void Final(uint8_t digest[16], CTX* context);

Core::ByteBuffer Hash(const uint8_t* buf, uint32_t length);
Core::ByteBuffer Hash(const Core::ByteBuffer& buf);
Core::ByteBuffer Hash(const String& str);
} // namespace MD5

namespace CRC32 {
uint32_t Hash(const uint8_t* buf, uint32_t length);
uint32_t Hash(const Core::ByteBuffer& buf);
uint32_t Hash(const String& str);
} // namespace CRC32

} // namespace Kyty::Math

#endif /* INCLUDE_KYTY_MATH_CRYPTO_H_ */
