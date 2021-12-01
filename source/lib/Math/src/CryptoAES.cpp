#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Math/Crypto.h" // IWYU pragma: associated

extern "C" {
#include "rijndael-alg-fst.h"
}

namespace Kyty::Math {

Core::ByteBuffer AES::Encrypt(const uint8_t* buf, uint32_t length, const uint8_t* key, const uint8_t* iv, Mode mode)
{
	Core::ByteBuffer out;

	EXIT_IF(!buf || !key);
	EXIT_IF(length == 0);
	EXIT_IF(mode != Mode::Cbc256Pkcs7Padding && mode != Mode::Cbc256ZeroPadding);

	uint32_t rk[4 * (MAXNR + 1)];
	uint8_t  tmp_buf[16];
	uint8_t  tmp_iv[16];

	if (iv != nullptr)
	{
		std::memcpy(tmp_iv, iv, 16);
	} else
	{
		std::memset(tmp_iv, 0, 16);
	}

	if (mode == Mode::Cbc256Pkcs7Padding || mode == Mode::Cbc256ZeroPadding)
	{
		int nr = rijndaelKeySetupEnc(rk, key, 256);

		EXIT_IF(nr != 14);

		uint32_t padding = 16 - (length % 16);

		if (mode == Mode::Cbc256ZeroPadding && padding == 16)
		{
			padding = 0;
		}

		Core::ByteBuffer o(length + padding);

		auto* out_ptr = reinterpret_cast<uint8_t*>(o.GetData());

		while (length >= 16)
		{
			for (int n = 0; n < 16; n++)
			{
				tmp_buf[n] = buf[n] ^ tmp_iv[n];
			}

			rijndaelEncrypt(rk, nr, tmp_buf, out_ptr);

			std::memcpy(tmp_iv, out_ptr, 16);

			length -= 16;
			buf += 16;
			out_ptr += 16;
		}

		if ((length + padding) != 0u)
		{
			EXIT_IF(length + padding != 16);

			for (uint32_t n = 0; n < length; n++)
			{
				tmp_buf[n] = buf[n] ^ tmp_iv[n];
			}

			if (mode == Mode::Cbc256ZeroPadding)
			{
				for (uint32_t n = length; n < 16; n++)
				{
					tmp_buf[n] = 0u ^ tmp_iv[n];
				}
			} else
			{
				for (uint32_t n = length; n < 16; n++)
				{
					tmp_buf[n] = padding ^ tmp_iv[n];
				}
			}

			rijndaelEncrypt(rk, nr, tmp_buf, out_ptr);
		}

		out = o;
	}

	return out;
}

Core::ByteBuffer AES::Encrypt(const Core::ByteBuffer& buf, const uint8_t* key, const uint8_t* iv, Mode mode)
{
	return AES::Encrypt(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size(), key, iv, mode);
}

Core::ByteBuffer AES::Decrypt(const uint8_t* buf, uint32_t length, const uint8_t* key, const uint8_t* iv, Mode mode)
{
	Core::ByteBuffer out;

	EXIT_IF(!buf || !key);
	EXIT_IF(length == 0);
	EXIT_IF(mode != Mode::Cbc256Pkcs7Padding && mode != Mode::Cbc256ZeroPadding);

	EXIT_IF((length % 16) != 0);

	uint32_t rk[4 * (MAXNR + 1)];
	uint8_t  tmp_buf[16];
	uint8_t  tmp_iv[16];

	if (iv != nullptr)
	{
		std::memcpy(tmp_iv, iv, 16);
	} else
	{
		std::memset(tmp_iv, 0, 16);
	}

	if (mode == Mode::Cbc256Pkcs7Padding || mode == Mode::Cbc256ZeroPadding)
	{
		int nr = rijndaelKeySetupDec(rk, key, 256);

		EXIT_IF(nr != 14);

		Core::ByteBuffer o(length);

		auto* out_ptr = reinterpret_cast<uint8_t*>(o.GetData());

		while (length >= 16)
		{
			rijndaelDecrypt(rk, nr, buf, tmp_buf);

			for (int n = 0; n < 16; n++)
			{
				out_ptr[n] = tmp_buf[n] ^ tmp_iv[n];
			}

			std::memcpy(tmp_iv, buf, 16);

			length -= 16;
			buf += 16;
			out_ptr += 16;
		}

		EXIT_IF(length > 0);

		if (mode == Mode::Cbc256Pkcs7Padding)
		{
			auto padding = std::to_integer<uint32_t>(o.At(o.Size() - 1));
			o.RemoveAt(o.Size() - padding, padding);
		}

		out = o;
	}

	return out;
}

Core::ByteBuffer AES::EncryptStr(const String& str, const uint8_t* key, const uint8_t* iv, Mode mode)
{
	String::Utf8 utf8 = str.utf8_str();
	return Encrypt(reinterpret_cast<const uint8_t*>(utf8.GetDataConst()), utf8.Size(), key, iv, mode);
}

Core::ByteBuffer AES::Decrypt(const Core::ByteBuffer& buf, const uint8_t* key, const uint8_t* iv, Mode mode)
{
	return AES::Decrypt(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size(), key, iv, mode);
}

String AES::DecryptStr(const uint8_t* buf, uint32_t length, const uint8_t* key, const uint8_t* iv, Mode mode)
{
	Core::ByteBuffer bin = AES::Decrypt(buf, length, key, iv, mode);
	EXIT_IF(bin.At(bin.Size() - 1) != (Core::Byte)0);
	return String::FromUtf8(reinterpret_cast<const char*>(bin.GetDataConst()));
}

String AES::DecryptStr(const Core::ByteBuffer& buf, const uint8_t* key, const uint8_t* iv, Mode mode)
{
	return AES::DecryptStr(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size(), key, iv, mode);
}

} // namespace Kyty::Math
