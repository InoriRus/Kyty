#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_STORAGEBUFFER_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_STORAGEBUFFER_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct StorageVulkanBuffer;
class CommandBuffer;

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

	bool Equal(const uint64_t* other) const override;

	[[nodiscard]] create_func_t              GetCreateFunc() const override;
	[[nodiscard]] create_from_objects_func_t GetCreateFromObjectsFunc() const override { return nullptr; };
	[[nodiscard]] write_back_func_t          GetWriteBackFunc() const override;
	[[nodiscard]] delete_func_t              GetDeleteFunc() const override;
	[[nodiscard]] update_func_t              GetUpdateFunc() const override;
};

void StorageBufferSet(CommandBuffer* cmd_buffer, StorageVulkanBuffer* buffer);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_STORAGEBUFFER_H_ */
