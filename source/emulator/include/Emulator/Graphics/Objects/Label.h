#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_LABEL_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_LABEL_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct Label;
class CommandBuffer;
struct GraphicContext;

void LabelInit();

constexpr int LABEL_ARGS_MAX = 5;

class LabelGpuObject: public GpuObject
{
public:
	using callback_t = bool (*)(const uint64_t* args);

	static constexpr int PARAM_VALUE      = 0;
	static constexpr int PARAM_CALLBACK_1 = 1;
	static constexpr int PARAM_CALLBACK_2 = 2;
	static constexpr int PARAM_ARG_1      = 3;
	static constexpr int PARAM_ARG_2      = 4;
	static constexpr int PARAM_ARG_3      = 5;
	static constexpr int PARAM_ARG_4      = 6;
	static constexpr int PARAM_ARG_5      = 7;

	explicit LabelGpuObject(uint64_t value, callback_t callback_1, callback_t callback_2, const uint64_t* args = nullptr)
	{
		params[PARAM_VALUE]      = value;
		params[PARAM_CALLBACK_1] = reinterpret_cast<uint64_t>(callback_1);
		params[PARAM_CALLBACK_2] = reinterpret_cast<uint64_t>(callback_2);
		if (args != nullptr)
		{
			params[PARAM_ARG_1] = args[0];
			params[PARAM_ARG_2] = args[1];
			params[PARAM_ARG_3] = args[2];
			params[PARAM_ARG_4] = args[3];
			params[PARAM_ARG_5] = args[4];
		}
		check_hash = false;
		type       = Graphics::GpuMemoryObjectType::Label;
	}

	bool Equal(const uint64_t* other) const override;

	[[nodiscard]] create_func_t              GetCreateFunc() const override;
	[[nodiscard]] create_from_objects_func_t GetCreateFromObjectsFunc() const override { return nullptr; };
	[[nodiscard]] write_back_func_t          GetWriteBackFunc() const override { return nullptr; };
	[[nodiscard]] delete_func_t              GetDeleteFunc() const override;
	[[nodiscard]] update_func_t              GetUpdateFunc() const override;
};

Label* LabelCreate64(GraphicContext* ctx, uint64_t* dst_gpu_addr, uint64_t value, LabelGpuObject::callback_t callback_1,
                     LabelGpuObject::callback_t callback_2, const uint64_t* args);
Label* LabelCreate32(GraphicContext* ctx, uint32_t* dst_gpu_addr, uint32_t value, LabelGpuObject::callback_t callback_1,
                     LabelGpuObject::callback_t callback_2, const uint64_t* args);
void   LabelDelete(Label* label);
void   LabelSet(CommandBuffer* buffer, Label* label);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_LABEL_H_ */
