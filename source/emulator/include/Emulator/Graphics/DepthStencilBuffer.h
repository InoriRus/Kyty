#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_DEPTHSTENCILBUFFER_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_DEPTHSTENCILBUFFER_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Graphics/GpuMemory.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct GraphicContext;
struct VulkanMemory;

class DepthStencilBufferObject: public GpuObject
{
public:
	static constexpr int PARAM_FORMAT = 0;
	static constexpr int PARAM_WIDTH  = 1;
	static constexpr int PARAM_HEIGHT = 2;
	static constexpr int PARAM_HTILE  = 3;
	static constexpr int PARAM_NEO    = 4;

	DepthStencilBufferObject(uint64_t vk_format, uint32_t width, uint32_t height, bool htile, bool neo)
	{
		params[PARAM_FORMAT] = vk_format;
		params[PARAM_WIDTH]  = width;
		params[PARAM_HEIGHT] = height;
		params[PARAM_HTILE]  = htile ? 1 : 0;
		params[PARAM_NEO]    = neo ? 1 : 0;
		check_hash           = false;
		type                 = Graphics::GpuMemoryObjectType::DepthStencilBuffer;
	}

	void* Create(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, VulkanMemory* mem) const override;
	bool  Equal(const uint64_t* other) const override;

	[[nodiscard]] write_back_func_t GetWriteBackFunc() const override { return nullptr; };
	[[nodiscard]] delete_func_t     GetDeleteFunc() const override;
	[[nodiscard]] update_func_t     GetUpdateFunc() const override;
};

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_DEPTHSTENCILBUFFER_H_ */
