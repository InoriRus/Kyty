#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_STORAGEBUFFER_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_STORAGEBUFFER_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Graphics/GpuMemory.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct GraphicContext;
struct VulkanMemory;

class StorageBufferGpuObject: public GpuObject
{
public:
	StorageBufferGpuObject(uint64_t stride, uint64_t num_records, bool ronly)
	{
		params[0]  = stride;
		params[1]  = num_records;
		check_hash = true;
		read_only  = ronly;
		type       = Graphics::GpuMemoryObjectType::StorageBuffer;
	}

	void* Create(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, VulkanMemory* mem) const override;
	bool  Equal(const uint64_t* other) const override;

	[[nodiscard]] write_back_func_t GetWriteBackFunc() const override;
	[[nodiscard]] delete_func_t     GetDeleteFunc() const override;
	[[nodiscard]] update_func_t     GetUpdateFunc() const override;
};

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_STORAGEBUFFER_H_ */
