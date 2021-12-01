#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_TEXTURE_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_TEXTURE_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Graphics/GpuMemory.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct GraphicContext;
struct VulkanMemory;

class TextureObject: public GpuObject
{
public:
	static constexpr int PARAM_DFMT    = 0;
	static constexpr int PARAM_NFMT    = 1;
	static constexpr int PARAM_WIDTH   = 2;
	static constexpr int PARAM_HEIGHT  = 3;
	static constexpr int PARAM_LEVELS  = 4;
	static constexpr int PARAM_TILE    = 5;
	static constexpr int PARAM_NEO     = 6;
	static constexpr int PARAM_SWIZZLE = 7;

	TextureObject(uint32_t dfmt, uint32_t nfmt, uint32_t width, uint32_t height, uint32_t levels, bool htile, bool neo, uint32_t swizzle)
	{
		params[PARAM_DFMT]    = dfmt;
		params[PARAM_NFMT]    = nfmt;
		params[PARAM_WIDTH]   = width;
		params[PARAM_HEIGHT]  = height;
		params[PARAM_LEVELS]  = levels;
		params[PARAM_TILE]    = htile ? 1 : 0;
		params[PARAM_NEO]     = neo ? 1 : 0;
		params[PARAM_SWIZZLE] = swizzle;
		check_hash            = true;
		type                  = Graphics::GpuMemoryObjectType::Texture;
	}

	void* Create(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, VulkanMemory* mem) const override;
	bool  Equal(const uint64_t* other) const override;

	[[nodiscard]] write_back_func_t GetWriteBackFunc() const override { return nullptr; };
	[[nodiscard]] delete_func_t     GetDeleteFunc() const override;
	[[nodiscard]] update_func_t     GetUpdateFunc() const override;
};

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_TEXTURE_H_ */
