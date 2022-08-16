#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_STORAGETEXTURE_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_STORAGETEXTURE_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

class StorageTextureObject: public GpuObject
{
public:
	static constexpr int PARAM_FORMAT       = 0;
	static constexpr int PARAM_PITCH        = 1;
	static constexpr int PARAM_WIDTH_HEIGHT = 2;
	static constexpr int PARAM_LEVELS       = 3;
	static constexpr int PARAM_TILE         = 4;
	static constexpr int PARAM_NEO          = 5;
	static constexpr int PARAM_SWIZZLE      = 6;

	StorageTextureObject(uint8_t dfmt, uint8_t nfmt, uint16_t fmt, uint32_t width, uint32_t height, uint32_t pitch, uint32_t base_level,
	                     uint32_t levels, uint32_t tile, bool neo, uint32_t swizzle)
	{
		params[PARAM_FORMAT]       = (static_cast<uint64_t>(fmt) << 16u) | (static_cast<uint64_t>(dfmt) << 8u) | nfmt;
		params[PARAM_PITCH]        = pitch;
		params[PARAM_WIDTH_HEIGHT] = (static_cast<uint64_t>(width) << 32u) | height;
		params[PARAM_LEVELS]       = (static_cast<uint64_t>(base_level) << 32u) | levels;
		params[PARAM_TILE]         = tile;
		params[PARAM_NEO]          = neo ? 1 : 0;
		params[PARAM_SWIZZLE]      = swizzle;
		check_hash                 = true;
		type                       = Graphics::GpuMemoryObjectType::StorageTexture;
	}

	bool Equal(const uint64_t* other) const override;

	[[nodiscard]] create_func_t              GetCreateFunc() const override;
	[[nodiscard]] create_from_objects_func_t GetCreateFromObjectsFunc() const override { return nullptr; };
	[[nodiscard]] write_back_func_t          GetWriteBackFunc() const override { return nullptr; };
	[[nodiscard]] delete_func_t              GetDeleteFunc() const override;
	[[nodiscard]] update_func_t              GetUpdateFunc() const override;
};

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_STORAGETEXTURE_H_ */
