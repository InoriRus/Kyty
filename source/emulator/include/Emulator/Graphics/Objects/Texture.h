#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_TEXTURE_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_TEXTURE_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct GraphicContext;
struct VulkanMemory;

class TextureObject: public GpuObject
{
public:
	static constexpr int TEXTURE_USAGE_SAMPLED = 0;
	static constexpr int TEXTURE_USAGE_STORAGE = 1;

	static constexpr int PARAM_DFMT_NFMT    = 0;
	static constexpr int PARAM_PITCH        = 1;
	static constexpr int PARAM_WIDTH_HEIGHT = 2;
	static constexpr int PARAM_USAGE        = 3;
	static constexpr int PARAM_LEVELS       = 4;
	static constexpr int PARAM_TILE         = 5;
	static constexpr int PARAM_NEO          = 6;
	static constexpr int PARAM_SWIZZLE      = 7;

	TextureObject(uint32_t dfmt, uint32_t nfmt, uint32_t width, uint32_t height, uint32_t pitch, uint32_t levels, uint32_t tile, bool neo,
	              uint32_t swizzle, uint32_t usage)
	{
		params[PARAM_DFMT_NFMT]    = (static_cast<uint64_t>(dfmt) << 32u) | nfmt;
		params[PARAM_PITCH]        = pitch;
		params[PARAM_WIDTH_HEIGHT] = (static_cast<uint64_t>(width) << 32u) | height;
		params[PARAM_USAGE]        = usage;
		params[PARAM_LEVELS]       = levels;
		params[PARAM_TILE]         = tile;
		params[PARAM_NEO]          = neo ? 1 : 0;
		params[PARAM_SWIZZLE]      = swizzle;
		check_hash                 = true;
		type                       = Graphics::GpuMemoryObjectType::Texture;
	}

	void* Create(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, VulkanMemory* mem) const override;
	bool  Equal(const uint64_t* other) const override;

	[[nodiscard]] write_back_func_t GetWriteBackFunc() const override { return nullptr; };
	[[nodiscard]] delete_func_t     GetDeleteFunc() const override;
	[[nodiscard]] update_func_t     GetUpdateFunc() const override;
};

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_TEXTURE_H_ */
