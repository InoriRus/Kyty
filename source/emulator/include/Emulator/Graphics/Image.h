#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_IMAGE_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_IMAGE_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Math/MathAll.h"

#include "Emulator/Common.h"
#include "Emulator/Graphics/Color.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

enum class ImageOrder
{
	Unknown,
	Rgb,
	Rgba,
	Argb,
	Bgr,
	Bgra,
	Abgr,
	Alpha
};

class ImagePrivate;

class Image
{
public:
	explicit Image(const String& name);
	virtual ~Image();

	[[nodiscard]] Image* Clone() const;

	void LoadSdl(void* sdl);
	void Load();
	void Load(const String& file_name);
	void Save(const String& file_name) const;

	[[nodiscard]] const String& GetName() const { return m_name; }
	[[nodiscard]] uint32_t      GetWidth() const { return m_width; }
	[[nodiscard]] uint32_t      GetHeight() const { return m_height; }
	[[nodiscard]] uint32_t      GetPitch() const { return m_pitch; }
	[[nodiscard]] int           GetBitsPerPixel() const { return m_bits_per_pixel; }
	[[nodiscard]] int           GetBytesPerPixel() const { return static_cast<int>(static_cast<unsigned int>(m_bits_per_pixel) >> 3u); }
	[[nodiscard]] int           GetAlignment() const
	{
		static const int p[] = {8, 1, 2, 1, 4, 1, 2, 1};
		return p[m_pitch & 7u];
	}
	[[nodiscard]] ImageOrder  GetOrder() const { return m_order; }
	[[nodiscard]] const void* GetData() const { return m_pixels; }
	[[nodiscard]] void*       GetSdlSurface() const;

	void DbgPrint() const;
	bool DbgEqual(const Image* img) const;

	static Image*     Create(const String& name, uint32_t width, uint32_t height, int bits_per_pixel);
	static Math::Size GetSize(const String& name);

	bool BlitTo(Image* img, const Math::Rect& src, const Math::Rect& dst) const;

	[[nodiscard]] rgba32_t GetPixel(uint32_t x, uint32_t y) const;
	[[nodiscard]] rgba32_t GetAvgPixel(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const;
	void                   SetPixel(uint32_t x, uint32_t y, rgba32_t color);

	KYTY_CLASS_NO_COPY(Image);

private:
	String        m_name;
	uint32_t      m_width          = 0;
	uint32_t      m_height         = 0;
	uint32_t      m_pitch          = 0;
	int           m_bits_per_pixel = 0;
	ImageOrder    m_order          = ImageOrder::Unknown;
	void*         m_pixels         = nullptr;
	ImagePrivate* m_image          = nullptr;
};

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_IMAGE_H_ */
