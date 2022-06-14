#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_RENDERTEXTURE_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_RENDERTEXTURE_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

enum class RenderTextureFormat : uint64_t
{
	Unknown,
	R8Unorm,
	R8G8B8A8Unorm,
	R8G8B8A8Srgb,
	B8G8R8A8Unorm,
	B8G8R8A8Srgb,
};

class RenderTextureObject: public GpuObject
{
public:
	static constexpr int PARAM_FORMAT     = 0;
	static constexpr int PARAM_WIDTH      = 1;
	static constexpr int PARAM_HEIGHT     = 2;
	static constexpr int PARAM_TILED      = 3;
	static constexpr int PARAM_NEO        = 4;
	static constexpr int PARAM_PITCH      = 5;
	static constexpr int PARAM_WRITE_BACK = 6;

	explicit RenderTextureObject(RenderTextureFormat pixel_format, uint32_t width, uint32_t height, bool tiled, bool neo, uint32_t pitch,
	                             bool write_back)
	{
		params[PARAM_FORMAT]     = static_cast<uint64_t>(pixel_format);
		params[PARAM_WIDTH]      = width;
		params[PARAM_HEIGHT]     = height;
		params[PARAM_TILED]      = tiled ? 1 : 0;
		params[PARAM_NEO]        = neo ? 1 : 0;
		params[PARAM_PITCH]      = pitch;
		params[PARAM_WRITE_BACK] = write_back ? 1 : 0;
		check_hash               = true;
		type                     = Graphics::GpuMemoryObjectType::RenderTexture;
	}

	bool Equal(const uint64_t* other) const override;

	[[nodiscard]] create_func_t              GetCreateFunc() const override;
	[[nodiscard]] create_from_objects_func_t GetCreateFromObjectsFunc() const override;
	[[nodiscard]] write_back_func_t          GetWriteBackFunc() const override;
	[[nodiscard]] delete_func_t              GetDeleteFunc() const override;
	[[nodiscard]] update_func_t              GetUpdateFunc() const override;
};

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_RENDERTEXTURE_H_ */
