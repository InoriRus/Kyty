#include "Kyty/Math/Crypto.h" // IWYU pragma: associated

namespace Kyty::Math {

namespace MD5 {

constexpr uint32_t S11 = 7;
constexpr uint32_t S12 = 12;
constexpr uint32_t S13 = 17;
constexpr uint32_t S14 = 22;
constexpr uint32_t S21 = 5;
constexpr uint32_t S22 = 9;
constexpr uint32_t S23 = 14;
constexpr uint32_t S24 = 20;
constexpr uint32_t S31 = 4;
constexpr uint32_t S32 = 11;
constexpr uint32_t S33 = 16;
constexpr uint32_t S34 = 23;
constexpr uint32_t S41 = 6;
constexpr uint32_t S42 = 10;
constexpr uint32_t S43 = 15;
constexpr uint32_t S44 = 21;

/* F, G, H and I are basic MD5 functions.
 */
//#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
//#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
//#define H(x, y, z) ((x) ^ (y) ^ (z))
//#define I(x, y, z) ((y) ^ ((x) | (~z)))
static uint32_t F(uint32_t x, uint32_t y, uint32_t z)
{
	return (((x) & (y)) | ((~x) & (z)));
}
static uint32_t G(uint32_t x, uint32_t y, uint32_t z)
{
	return (((x) & (z)) | ((y) & (~z)));
}
static uint32_t H(uint32_t x, uint32_t y, uint32_t z)
{
	return ((x) ^ (y) ^ (z));
}
static uint32_t I(uint32_t x, uint32_t y, uint32_t z)
{
	return ((y) ^ ((x) | (~z)));
}

/* ROTATE_LEFT rotates x left n bits.
 */
//#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
static uint32_t ROTATE_LEFT(uint32_t x, uint32_t n)
{
	return (((x) << (n)) | ((x) >> (32u - (n))));
}

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
Rotation is separate from addition to prevent recomputation.
 */
//#define FF(a, b, c, d, x, s, ac) {
// (a) += F ((b), (c), (d)) + (x) + (uint32_t)(ac);
// (a) = ROTATE_LEFT ((a), (s));
// (a) += (b);
//  }
//#define GG(a, b, c, d, x, s, ac) {
// (a) += G ((b), (c), (d)) + (x) + (uint32_t)(ac);
// (a) = ROTATE_LEFT ((a), (s));
// (a) += (b);
//  }
//#define HH(a, b, c, d, x, s, ac) {
// (a) += H ((b), (c), (d)) + (x) + (uint32_t)(ac);
// (a) = ROTATE_LEFT ((a), (s));
// (a) += (b);
//  }
//#define II(a, b, c, d, x, s, ac) {
// (a) += I ((b), (c), (d)) + (x) + (uint32_t)(ac);
// (a) = ROTATE_LEFT ((a), (s));
// (a) += (b);
//  }

static void FF(uint32_t* a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
	(*a) += F((b), (c), (d)) + (x) + (ac);
	(*a) = ROTATE_LEFT((*a), (s));
	(*a) += (b);
}

static void GG(uint32_t* a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
	(*a) += G((b), (c), (d)) + (x) + (ac);
	(*a) = ROTATE_LEFT((*a), (s));
	(*a) += (b);
}

static void HH(uint32_t* a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
	(*a) += H((b), (c), (d)) + (x) + (ac);
	(*a) = ROTATE_LEFT((*a), (s));
	(*a) += (b);
}

static void II(uint32_t* a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
{
	(*a) += I((b), (c), (d)) + (x) + (ac);
	(*a) = ROTATE_LEFT((*a), (s));
	(*a) += (b);
}

static void Transform(uint32_t state[4], const uint8_t block[64]);
static void Encode(uint8_t* output, const uint32_t* input, uint32_t len);
static void Decode(uint32_t* output, const uint8_t* input, uint32_t len);

static uint8_t g_padding[64] = {0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void Init(CTX* context)
{
	context->count[0] = context->count[1] = 0;

	/* Load magic initialization constants.*/
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

void Update(CTX* context, const uint8_t* input, uint32_t input_len)
{
	unsigned int i       = 0;
	unsigned int index   = 0;
	unsigned int part_len = 0;

	/* Compute number of bytes mod 64 */
	index = ((context->count[0] >> 3u) & 0x3Fu);

	/* Update number of bits */
	if ((context->count[0] += (input_len << 3u)) < (input_len << 3u))
	{
		context->count[1]++;
	}
	context->count[1] += (input_len >> 29u);

	part_len = 64 - index;

	/* Transform as many times as possible.
	 */
	if (input_len >= part_len)
	{
		std::memcpy(&context->buffer[index], (input), part_len);

		Transform(context->state, context->buffer);

		for (i = part_len; i + 63 < input_len; i += 64)
		{
			Transform(context->state, &input[i]);
		}

		index = 0;
	} else
	{
		i = 0;
	}

	/* Buffer remaining input */
	std::memcpy(&context->buffer[index], (&input[i]), input_len - i);
}

void Final(uint8_t digest[16], CTX* context)
{
	unsigned char bits[8];
	unsigned int  index  = 0;
	unsigned int  pad_len = 0;

	/* Save number of bits */
	Encode(bits, context->count, 8);

	/* Pad out to 56 mod 64.
	 */
	index  = ((context->count[0] >> 3u) & 0x3fu);
	pad_len = (index < 56) ? (56 - index) : (120 - index);
	Update(context, g_padding, pad_len);

	/* Append length (before padding) */
	Update(context, bits, 8);

	/* Store state in digest */
	Encode(digest, context->state, 16);

	/* Zeroize sensitive information.
	 */
	std::memset(reinterpret_cast<uint8_t*>(context), 0, sizeof(*context));
}

static void Transform(uint32_t state[4], const uint8_t block[64])
{
	uint32_t a = state[0];
	uint32_t b = state[1];
	uint32_t c = state[2];
	uint32_t d = state[3];
	uint32_t x[16];

	Decode(x, block, 64);

	/* Round 1 */
	FF(&a, b, c, d, x[0], S11, 0xd76aa478);  /* 1 */
	FF(&d, a, b, c, x[1], S12, 0xe8c7b756);  /* 2 */
	FF(&c, d, a, b, x[2], S13, 0x242070db);  /* 3 */
	FF(&b, c, d, a, x[3], S14, 0xc1bdceee);  /* 4 */
	FF(&a, b, c, d, x[4], S11, 0xf57c0faf);  /* 5 */
	FF(&d, a, b, c, x[5], S12, 0x4787c62a);  /* 6 */
	FF(&c, d, a, b, x[6], S13, 0xa8304613);  /* 7 */
	FF(&b, c, d, a, x[7], S14, 0xfd469501);  /* 8 */
	FF(&a, b, c, d, x[8], S11, 0x698098d8);  /* 9 */
	FF(&d, a, b, c, x[9], S12, 0x8b44f7af);  /* 10 */
	FF(&c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF(&b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF(&a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF(&d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF(&c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF(&b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

	/* Round 2 */
	GG(&a, b, c, d, x[1], S21, 0xf61e2562);  /* 17 */
	GG(&d, a, b, c, x[6], S22, 0xc040b340);  /* 18 */
	GG(&c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG(&b, c, d, a, x[0], S24, 0xe9b6c7aa);  /* 20 */
	GG(&a, b, c, d, x[5], S21, 0xd62f105d);  /* 21 */
	GG(&d, a, b, c, x[10], S22, 0x2441453);  /* 22 */
	GG(&c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG(&b, c, d, a, x[4], S24, 0xe7d3fbc8);  /* 24 */
	GG(&a, b, c, d, x[9], S21, 0x21e1cde6);  /* 25 */
	GG(&d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG(&c, d, a, b, x[3], S23, 0xf4d50d87);  /* 27 */

	GG(&b, c, d, a, x[8], S24, 0x455a14ed);  /* 28 */
	GG(&a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG(&d, a, b, c, x[2], S22, 0xfcefa3f8);  /* 30 */
	GG(&c, d, a, b, x[7], S23, 0x676f02d9);  /* 31 */
	GG(&b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

	/* Round 3 */
	HH(&a, b, c, d, x[5], S31, 0xfffa3942);  /* 33 */
	HH(&d, a, b, c, x[8], S32, 0x8771f681);  /* 34 */
	HH(&c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH(&b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH(&a, b, c, d, x[1], S31, 0xa4beea44);  /* 37 */
	HH(&d, a, b, c, x[4], S32, 0x4bdecfa9);  /* 38 */
	HH(&c, d, a, b, x[7], S33, 0xf6bb4b60);  /* 39 */
	HH(&b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH(&a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH(&d, a, b, c, x[0], S32, 0xeaa127fa);  /* 42 */
	HH(&c, d, a, b, x[3], S33, 0xd4ef3085);  /* 43 */
	HH(&b, c, d, a, x[6], S34, 0x4881d05);   /* 44 */
	HH(&a, b, c, d, x[9], S31, 0xd9d4d039);  /* 45 */
	HH(&d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH(&c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH(&b, c, d, a, x[2], S34, 0xc4ac5665);  /* 48 */

	/* Round 4 */
	II(&a, b, c, d, x[0], S41, 0xf4292244);  /* 49 */
	II(&d, a, b, c, x[7], S42, 0x432aff97);  /* 50 */
	II(&c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II(&b, c, d, a, x[5], S44, 0xfc93a039);  /* 52 */
	II(&a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II(&d, a, b, c, x[3], S42, 0x8f0ccc92);  /* 54 */
	II(&c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II(&b, c, d, a, x[1], S44, 0x85845dd1);  /* 56 */
	II(&a, b, c, d, x[8], S41, 0x6fa87e4f);  /* 57 */
	II(&d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II(&c, d, a, b, x[6], S43, 0xa3014314);  /* 59 */
	II(&b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II(&a, b, c, d, x[4], S41, 0xf7537e82);  /* 61 */
	II(&d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II(&c, d, a, b, x[2], S43, 0x2ad7d2bb);  /* 63 */
	II(&b, c, d, a, x[9], S44, 0xeb86d391);  /* 64 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	/* Zeroize sensitive information.
	 */
	std::memset(reinterpret_cast<uint8_t*>(x), 0, sizeof(x));
}

/* Encodes input (uint32_t) into output (unsigned char). Assumes len is
  a multiple of 4.
 */
static void Encode(uint8_t* output, const uint32_t* input, uint32_t len)
{
	unsigned int i = 0;
	unsigned int j = 0;

	for (i = 0, j = 0; j < len; i++, j += 4)
	{
		output[j]     = static_cast<unsigned char>(input[i] & 0xffu);
		output[j + 1] = static_cast<unsigned char>((input[i] >> 8u) & 0xffu);
		output[j + 2] = static_cast<unsigned char>((input[i] >> 16u) & 0xffu);
		output[j + 3] = static_cast<unsigned char>((input[i] >> 24u) & 0xffu);
	}
}

/* Decodes input (unsigned char) into output (uint32_t). Assumes len is
 a multiple of 4.
 */
static void Decode(uint32_t* output, const uint8_t* input, uint32_t len)
{
	unsigned int i = 0;
	unsigned int j = 0;

	for (i = 0, j = 0; j < len; i++, j += 4)
	{
		output[i] = (static_cast<uint32_t>(input[j])) | ((static_cast<uint32_t>(input[j + 1])) << 8u) |
		            ((static_cast<uint32_t>(input[j + 2])) << 16u) | ((static_cast<uint32_t>(input[j + 3])) << 24u);
	}
}

Core::ByteBuffer Hash(const uint8_t* buf, uint32_t length)
{
	CTX              ctx {};
	Core::ByteBuffer ret(16);

	Init(&ctx);
	Update(&ctx, buf, length);
	Final(reinterpret_cast<uint8_t*>(ret.GetData()), &ctx);
	return ret;
}

Core::ByteBuffer Hash(const Core::ByteBuffer& buf)
{
	return Hash(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

Core::ByteBuffer Hash(const String& str)
{
	String::Utf8 utf8 = str.utf8_str();
	return Hash(reinterpret_cast<const uint8_t*>(utf8.GetDataConst()), utf8.Size() - 1);
}

} // namespace MD5

namespace CRC32 {

static uint32_t g_crc_table[256]  = {0};
static bool     g_crc_initialized = false;

uint32_t Hash(const uint8_t* buf, uint32_t length)
{
	uint32_t crc = 0;

	if (!g_crc_initialized)
	{
		for (int i = 0; i < 256; i++)
		{
			crc = i;
			for (int j = 0; j < 8; j++)
			{
				crc = (crc & 1u) != 0u ? (crc >> 1u) ^ 0xEDB88320u : crc >> 1u;
			}
			g_crc_table[i] = crc;
		};

		g_crc_initialized = true;
	}

	crc = 0xFFFFFFFF;
	while ((length--) != 0u)
	{
		crc = g_crc_table[(crc ^ *buf++) & 0xFFu] ^ (crc >> 8u);
	}

	return crc ^ 0xFFFFFFFF;
}

uint32_t Hash(const Core::ByteBuffer& buf)
{
	return Hash(reinterpret_cast<const uint8_t*>(buf.GetDataConst()), buf.Size());
}

uint32_t Hash(const String& str)
{
	String::Utf8 utf8 = str.utf8_str();
	return Hash(reinterpret_cast<const uint8_t*>(utf8.GetDataConst()), utf8.Size() - 1);
}

} // namespace CRC32

} // namespace Kyty::Math
