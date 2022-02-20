#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_VERTEXBUFFER_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_VERTEXBUFFER_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

class VertexBufferGpuObject: public GpuObject
{
public:
	VertexBufferGpuObject()
	{
		check_hash = true;
		type       = Graphics::GpuMemoryObjectType::VertexBuffer;
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

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_VERTEXBUFFER_H_ */
