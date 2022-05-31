#include "Emulator/Graphics/Image.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/SafeDelete.h"

#include "SDL_blendmode.h"
#include "SDL_error.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_rwops.h"
#include "SDL_surface.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT ASSERT
#define STBI_NO_SIMD
#include "stb_image.h"

#define KYTY_SDL_BITSPERPIXEL(X) (((X) >> 8u) & 0xFFu)
#define KYTY_SDL_PIXELTYPE(X)    (((X) >> 24u) & 0x0Fu)
#define KYTY_SDL_PIXELLAYOUT(X)  (((X) >> 16u) & 0x0Fu)
#define KYTY_SDL_PIXELORDER(X)   (((X) >> 20u) & 0x0Fu)

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

// using namespace Core;

using Core::File;

class ImagePrivate
{
public:
	KYTY_CLASS_NO_COPY(ImagePrivate);

	explicit ImagePrivate(SDL_Surface* s): sdl(s) {}

	virtual ~ImagePrivate() { SDL_FreeSurface(sdl); }

	void BgrToRgb() const
	{
		auto* rowptr  = static_cast<uint8_t*>(sdl->pixels);
		int   row_num = sdl->h;
		int   col_num = sdl->w;
		int   pitch   = sdl->pitch;
		for (int row = 0; row < row_num; row++)
		{
			uint8_t* colptr = rowptr;

			for (int col = 0; col < col_num; col++)
			{
				uint8_t t = colptr[0];
				colptr[0] = colptr[2];
				colptr[2] = t;
				colptr += 3;
			}

			rowptr += pitch;
		}
	}

	void BgraToRgba() const
	{
		auto* rowptr  = static_cast<uint8_t*>(sdl->pixels);
		int   row_num = sdl->h;
		int   col_num = sdl->w;
		int   pitch   = sdl->pitch;
		for (int row = 0; row < row_num; row++)
		{
			uint8_t* colptr = rowptr;

			for (int col = 0; col < col_num; col++)
			{
				uint8_t t = colptr[0];
				colptr[0] = colptr[2];
				colptr[2] = t;
				colptr += 4;
			}

			rowptr += pitch;
		}
	}

	SDL_Surface* sdl;
};

Image::Image(const String& name): m_name(name) {}

Image::~Image()
{
	if (m_image != nullptr)
	{
		Delete(m_image);
	}
}

void Image::Load(const String& file_name)
{
	m_name = file_name;
	Load();
}

static int read(void* user, char* data, int size)
{
	auto* src = static_cast<SDL_RWops*>(user);
	return static_cast<int>(src->read(src, data, 1, size));
}

static void skip(void* user, int n)
{
	auto* src = static_cast<SDL_RWops*>(user);
	src->seek(src, n, RW_SEEK_CUR);
}

static int eof(void* user)
{
	auto* src = static_cast<SDL_RWops*>(user);
	return (src->seek(src, 0, RW_SEEK_CUR) >= src->size(src) ? 1 : 0);
}

static SDL_Surface* IMG_LoadPNG_RW(SDL_RWops* src)
{
	int width           = 0;
	int height          = 0;
	int bytes_per_pixel = 0;

	stbi_io_callbacks cb {};
	cb.read = read;
	cb.skip = skip;
	cb.eof  = eof;

	void* data = stbi_load_from_callbacks(&cb, src, &width, &height, &bytes_per_pixel, 0);

	uint32_t pitch = (width * bytes_per_pixel + 3u) & ~3u;

	uint32_t r_mask = 0x000000FF;
	uint32_t g_mask = 0x0000FF00;
	uint32_t b_mask = 0x00FF0000;
	uint32_t a_mask = (bytes_per_pixel == 4) ? 0xFF000000 : 0;

	SDL_Surface* surface =
	    SDL_CreateRGBSurfaceFrom(data, width, height, bytes_per_pixel * 8, static_cast<int>(pitch), r_mask, g_mask, b_mask, a_mask);

	EXIT_NOT_IMPLEMENTED(surface == nullptr);

	return surface;
}

void Image::Load()
{
	if (m_image != nullptr)
	{
		Delete(m_image);
	}

	auto file_name = m_name;

	File f;
	if (!f.Open(file_name, File::Mode::Read))
	{
		EXIT("Can't open file %s\n", file_name.C_Str());
	}

	SDL_RWops* ops = f.CreateSdlRWops();

	EXIT_IF(ops == nullptr);

	SDL_Surface* sdl = nullptr;

	if (file_name.EndsWith(U".bmp", String::Case::Insensitive))
	{
		/* sdl = IMG_LoadBMP_RW(ops); */
		KYTY_NOT_IMPLEMENTED;
	} else if (file_name.EndsWith(U".tga", String::Case::Insensitive))
	{
		/* sdl = IMG_LoadTGA_RW(ops); */
		KYTY_NOT_IMPLEMENTED;
	} else if (file_name.EndsWith(U".png", String::Case::Insensitive))
	{
		sdl = IMG_LoadPNG_RW(ops);
		// KYTY_NOT_IMPLEMENTED;
	} else if (file_name.EndsWith(U".webp", String::Case::Insensitive))
	{
		/* sdl = IMG_LoadWEBP_RW(ops); */
		KYTY_NOT_IMPLEMENTED;
	} else
	{
		EXIT("Unknown image type %s\n", file_name.utf8_str().GetData());
	}

	LoadSdl(sdl);

	SDL_RWclose(ops);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Image::LoadSdl(void* s)
{
	auto* sdl = static_cast<SDL_Surface*>(s);

	EXIT_IF(sdl == nullptr);

	m_bits_per_pixel = static_cast<int>(KYTY_SDL_BITSPERPIXEL(sdl->format->format));

	EXIT_IF(sdl->format->palette != nullptr && m_bits_per_pixel != 8);

	m_image = new ImagePrivate(sdl);

	m_width  = m_image->sdl->w;
	m_height = m_image->sdl->h;
	m_pitch  = m_image->sdl->pitch;

	EXIT_IF(KYTY_SDL_PIXELTYPE(m_image->sdl->format->format) != SDL_PIXELTYPE_ARRAYU8 &&
	        KYTY_SDL_PIXELTYPE(m_image->sdl->format->format) != SDL_PIXELTYPE_INDEX8 &&
	        KYTY_SDL_PIXELTYPE(m_image->sdl->format->format) != SDL_PIXELTYPE_PACKED32);

	EXIT_IF(KYTY_SDL_PIXELLAYOUT(m_image->sdl->format->format) != 0 &&
	        KYTY_SDL_PIXELLAYOUT(m_image->sdl->format->format) != SDL_PACKEDLAYOUT_8888);

	if (m_bits_per_pixel == 8)
	{
		m_order = ImageOrder::Alpha;
	} else if (SDL_ISPIXELFORMAT_PACKED(m_image->sdl->format->format)) // NOLINT(hicpp-signed-bitwise)
	{
		switch (KYTY_SDL_PIXELORDER(m_image->sdl->format->format))
		{
			case SDL_PACKEDORDER_NONE: EXIT("SDL_PACKEDORDER_NONE\n"); break;
			case SDL_PACKEDORDER_XRGB: EXIT("SDL_PACKEDORDER_XRGB\n"); break;
			case SDL_PACKEDORDER_RGBX: EXIT("SDL_PACKEDORDER_RGBX\n"); break;

			// TGA32, BMP32
			case SDL_PACKEDORDER_ARGB:
				m_image->BgraToRgba();
				m_order = ImageOrder::Rgba;
				break;

			case SDL_PACKEDORDER_RGBA: EXIT("SDL_PACKEDORDER_RGBA\n"); break;
			case SDL_PACKEDORDER_XBGR: EXIT("SDL_PACKEDORDER_XBGR\n"); break;
			case SDL_PACKEDORDER_BGRX: EXIT("SDL_PACKEDORDER_BGRX\n"); break;

			// WEBP32, PNG32
			case SDL_PACKEDORDER_ABGR: m_order = ImageOrder::Rgba; break;

			case SDL_PACKEDORDER_BGRA: EXIT("SDL_PACKEDORDER_BGRA\n"); break;
			default: EXIT("unknown packed format %d\n", int(KYTY_SDL_PIXELORDER(m_image->sdl->format->format)));
		}
	} else if (SDL_ISPIXELFORMAT_ARRAY(m_image->sdl->format->format)) // NOLINT(hicpp-signed-bitwise)
	{
		switch (KYTY_SDL_PIXELORDER(m_image->sdl->format->format))
		{
			case SDL_ARRAYORDER_NONE: EXIT("SDL_ARRAYORDER_NONE\n"); break;

			// WEBP24, PNG24
			case SDL_ARRAYORDER_RGB: m_order = ImageOrder::Rgb; break;

			case SDL_ARRAYORDER_RGBA: EXIT("SDL_ARRAYORDER_RGBA\n"); break;
			case SDL_ARRAYORDER_ARGB: EXIT("SDL_ARRAYORDER_ARGB\n"); break;

			// TGA24, BMP24,
			case SDL_ARRAYORDER_BGR:
				m_image->BgrToRgb();
				m_order = ImageOrder::Rgb;
				break;

			case SDL_ARRAYORDER_BGRA: EXIT("SDL_ARRAYORDER_BGRA\n"); break;
			case SDL_ARRAYORDER_ABGR: EXIT("SDL_ARRAYORDER_ABGR\n"); break;
			default: EXIT("unknown array format %d\n", int(KYTY_SDL_PIXELORDER(m_image->sdl->format->format)));
		}
	} else
	{
		EXIT("invalid format\n");
	}

	EXIT_IF((int)KYTY_SDL_BITSPERPIXEL(m_image->sdl->format->format) != m_image->sdl->format->BitsPerPixel);

	EXIT_IF(m_bits_per_pixel != 24 && m_bits_per_pixel != 32 && m_bits_per_pixel != 8);

	EXIT_IF(m_pitch - ((m_width * m_bits_per_pixel) >> 3u) >= 4);

	m_pixels = m_image->sdl->pixels;
}

void Image::DbgPrint() const
{
	printf("------\n");
	printf("%s:\n", m_name.utf8_str().GetData());
	printf("width = %d\n", (m_image->sdl->w));
	printf("height = %d\n", (m_image->sdl->h));
	printf("pitch = %d\n", (m_image->sdl->pitch));
	printf("type = %d\n", static_cast<int>(KYTY_SDL_PIXELTYPE(m_image->sdl->format->format)));

	if (m_bits_per_pixel == 8)
	{
		printf("order = alpha\n");
	} else if (SDL_ISPIXELFORMAT_PACKED(m_image->sdl->format->format)) // NOLINT(hicpp-signed-bitwise)
	{
		switch (KYTY_SDL_PIXELORDER(m_image->sdl->format->format))
		{
			case SDL_PACKEDORDER_NONE: printf("order = SDL_PACKEDORDER_NONE\n"); break;
			case SDL_PACKEDORDER_XRGB: printf("order = SDL_PACKEDORDER_XRGB\n"); break;
			case SDL_PACKEDORDER_RGBX: printf("order = SDL_PACKEDORDER_RGBX\n"); break;
			case SDL_PACKEDORDER_ARGB: printf("order = SDL_PACKEDORDER_ARGB\n"); break;
			case SDL_PACKEDORDER_RGBA: printf("order = SDL_PACKEDORDER_RGBA\n"); break;
			case SDL_PACKEDORDER_XBGR: printf("order = SDL_PACKEDORDER_XBGR\n"); break;
			case SDL_PACKEDORDER_BGRX: printf("order = SDL_PACKEDORDER_BGRX\n"); break;
			case SDL_PACKEDORDER_ABGR: printf("order = SDL_PACKEDORDER_ABGR\n"); break;
			case SDL_PACKEDORDER_BGRA: printf("order = SDL_PACKEDORDER_BGRA\n"); break;
			default: printf("order = <packed_invalid>\n");
		}
	} else if (SDL_ISPIXELFORMAT_ARRAY(m_image->sdl->format->format)) // NOLINT(hicpp-signed-bitwise)
	{
		switch (KYTY_SDL_PIXELORDER(m_image->sdl->format->format))
		{
			case SDL_ARRAYORDER_NONE: printf("order = SDL_ARRAYORDER_NONE\n"); break;
			case SDL_ARRAYORDER_RGB: printf("order = SDL_ARRAYORDER_RGB\n"); break;
			case SDL_ARRAYORDER_RGBA: printf("order = SDL_ARRAYORDER_RGBA\n"); break;
			case SDL_ARRAYORDER_ARGB: printf("order = SDL_ARRAYORDER_ARGB\n"); break;
			case SDL_ARRAYORDER_BGR: printf("order = SDL_ARRAYORDER_BGR\n"); break;
			case SDL_ARRAYORDER_BGRA: printf("order = SDL_ARRAYORDER_BGRA\n"); break;
			case SDL_ARRAYORDER_ABGR: printf("order = SDL_ARRAYORDER_ABGR\n"); break;
			default: printf("order = <array_invalid>\n");
		}
	}

	printf("layout = %d\n", static_cast<int>(KYTY_SDL_PIXELLAYOUT(m_image->sdl->format->format)));
	printf("bits_per_pixel = %d\n", static_cast<int>(KYTY_SDL_BITSPERPIXEL(m_image->sdl->format->format)));
	printf("bytes_per_pixel = %d\n", static_cast<int>(SDL_BYTESPERPIXEL(m_image->sdl->format->format))); // NOLINT(hicpp-signed-bitwise)
	printf("bits_per_pixel = %d\n", static_cast<int>(m_image->sdl->format->BitsPerPixel));
	printf("bytes_per_pixel = %d\n", static_cast<int>(m_image->sdl->format->BytesPerPixel));
}

bool Image::DbgEqual(const Image* img) const
{
	if (m_width != img->m_width || m_height != img->m_height || m_pitch != img->m_pitch || m_bits_per_pixel != img->m_bits_per_pixel ||
	    m_order != img->m_order)
	{
		return false;
	}

	for (uint32_t yi = 0; yi < m_height; yi++)
	{
		for (uint32_t xi = 0; xi < m_width; xi++)
		{
			if (GetPixel(xi, yi) != img->GetPixel(xi, yi))
			{
				return false;
			}
		}
	}

	return true;
}

void Image::Save(const String& file_name) const
{
	EXIT_IF(m_image == nullptr);

	File f;
	f.Create(file_name);

	SDL_RWops* ops = f.CreateSdlRWops();

	int result = -1;

	if (file_name.EndsWith(U".png", String::Case::Insensitive))
	{
		/*result = IMG_SavePNG_RW(m_image->sdl, ops, 1);*/
		KYTY_NOT_IMPLEMENTED;
	} else if (file_name.EndsWith(U".bmp", String::Case::Insensitive))
	{
		result = SDL_SaveBMP_RW(m_image->sdl, ops, 1);
	} else
	{
		EXIT("Unknown image type %s\n", file_name.utf8_str().GetData());
	}

	if (result != 0)
	{
		EXIT("SDL error: %s\n", SDL_GetError());
	}
}

Image* Image::Clone() const
{
	auto* img = new Image(m_name);
	if (m_image != nullptr)
	{
		SDL_Surface* copy_sdl = SDL_ConvertSurface(m_image->sdl, m_image->sdl->format, SDL_SWSURFACE);
		img->LoadSdl(copy_sdl);
	}
	return img;
}

Image* Image::Create(const String& name, uint32_t width, uint32_t height, int bits_per_pixel)
{
	auto* img = new Image(name);

	EXIT_IF(bits_per_pixel != 24 && bits_per_pixel != 32);

	//	uint32_t rmask = 0;
	//	uint32_t gmask = 0;
	//	uint32_t bmask = 0;
	//	uint32_t amask = 0;
	//
	//	if (bits_per_pixel == 32)
	//	{
	uint32_t amask = 0xFF000000;
	uint32_t bmask = 0x00FF0000;
	uint32_t gmask = 0x0000FF00;
	uint32_t rmask = 0x000000FF;
	//	}

	SDL_Surface* sdl =
	    SDL_CreateRGBSurface(0, static_cast<int>(width), static_cast<int>(height), bits_per_pixel, rmask, gmask, bmask, amask);

	SDL_Rect r = {0, 0, static_cast<int>(width), static_cast<int>(height)};
	SDL_FillRect(sdl, &r, 0);

	img->LoadSdl(sdl);

	return img;
}

rgba32_t Image::GetPixel(uint32_t x, uint32_t y) const
{
	if (m_order == ImageOrder::Rgb && m_bits_per_pixel == 24)
	{
		uint8_t* p = ((static_cast<uint8_t*>(m_pixels)) + m_pitch * static_cast<size_t>(y) + 3 * static_cast<size_t>(x));
		return Rgb(p[0], p[1], p[2]);
	}

	if (m_order == ImageOrder::Rgba && m_bits_per_pixel == 32)
	{
		uint8_t* p = ((static_cast<uint8_t*>(m_pixels)) + m_pitch * static_cast<size_t>(y) + 4 * static_cast<size_t>(x));
		return Rgb(p[0], p[1], p[2], p[3]);
	}

	EXIT("invalid format: order = %d, bits = %d\n", int(m_order), int(m_bits_per_pixel));

	return 0;
}

void Image::SetPixel(uint32_t x, uint32_t y, rgba32_t color)
{
	if (m_order == ImageOrder::Rgb && m_bits_per_pixel == 24)
	{
		uint8_t* p = ((static_cast<uint8_t*>(m_pixels)) + m_pitch * static_cast<size_t>(y) + 3 * static_cast<size_t>(x));
		p[0]       = RgbToRed(color);
		p[1]       = RgbToGreen(color);
		p[2]       = RgbToBlue(color);
	} else if (m_order == ImageOrder::Rgba && m_bits_per_pixel == 32)
	{
		uint8_t* p = ((static_cast<uint8_t*>(m_pixels)) + m_pitch * static_cast<size_t>(y) + 4 * static_cast<size_t>(x));
		p[0]       = RgbToRed(color);
		p[1]       = RgbToGreen(color);
		p[2]       = RgbToBlue(color);
		p[3]       = RgbToAlpha(color);
	} else
	{
		EXIT("invalid format: order = %d, bits = %d\n", int(m_order), int(m_bits_per_pixel));
	}
}

rgba32_t Image::GetAvgPixel(uint32_t x, uint32_t y, uint32_t w, uint32_t h) const
{
	EXIT_IF(x >= m_width);
	EXIT_IF(y >= m_height);
	EXIT_IF(x + w > m_width);
	EXIT_IF(y + h > m_height);

	vec4 sum(0.0f);

	for (uint32_t yi = y; yi < y + h; yi++)
	{
		for (uint32_t xi = x; xi < x + w; xi++)
		{
			Color s(GetPixel(xi, yi));
			sum = sum + s.Rgba();
		}
	}

	sum /= static_cast<float>(w * h);

	return Color(sum).ToRgba32();
}

bool Image::BlitTo(Image* img, const Math::Rect& src, const Math::Rect& dst) const
{
	EXIT_IF(!img);
	EXIT_IF(src.x >= m_width || src.y >= m_height || src.x + src.width - 1 >= m_width || src.y + src.height - 1 >= m_height);
	EXIT_IF(dst.x >= img->m_width || dst.y >= img->m_height || dst.x + dst.width - 1 >= img->m_width ||
	        dst.y + dst.height - 1 >= img->m_height);

	SDL_SetSurfaceBlendMode(m_image->sdl, SDL_BLENDMODE_NONE);

	int result = 0;

	SDL_Rect r1 = {static_cast<int>(src.x), static_cast<int>(src.y), static_cast<int>(src.width), static_cast<int>(src.height)};
	SDL_Rect r2 = {static_cast<int>(dst.x), static_cast<int>(dst.y), static_cast<int>(dst.width), static_cast<int>(dst.height)};

	if (src.width != dst.width || src.height != dst.height)
	{
		result = SDL_BlitScaled(m_image->sdl, &r1, img->m_image->sdl, &r2);
	} else
	{
		result = SDL_BlitSurface(m_image->sdl, &r1, img->m_image->sdl, &r2);
	}

	if (result != 0)
	{
		printf("Blit failed: %s\n", SDL_GetError());

		return false;
	}

	return true;
}

void* Image::GetSdlSurface() const
{
	return m_image->sdl;
}

Math::Size Image::GetSize(const String& name)
{
	if (!name.EndsWith(U".png", String::Case::Insensitive))
	{
		EXIT("unsupported format: %s\n", name.C_Str());
	}

	File f;
	if (!f.Open(name, File::Mode::Read))
	{
		EXIT("Can't open file %s\n", name.C_Str());
	}

	Math::Size s(0, 0);
	uint32_t   header = 0;

	f.Seek(12);
	f.Read(&header, 4);

	if (header != 0x52444849)
	{
		EXIT("wrong png file: %s\n", name.C_Str());
	}

	f.ReadR(&s.width, 4);
	f.ReadR(&s.height, 4);

	f.Close();

	return s;
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
