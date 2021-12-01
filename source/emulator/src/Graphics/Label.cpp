#include "Emulator/Graphics/Label.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Profiler.h"

#include <vulkan/vulkan_core.h>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct Label
{
	VkDevice                   device         = nullptr;
	VkEvent                    event          = nullptr;
	bool                       active         = false;
	uint64_t*                  dst_gpu_addr64 = nullptr;
	uint64_t                   value64        = 0;
	uint32_t*                  dst_gpu_addr32 = nullptr;
	uint32_t                   value32        = 0;
	LabelGpuObject::callback_t callback_1     = nullptr;
	LabelGpuObject::callback_t callback_2     = nullptr;
	uint64_t                   args[4]        = {};
};

class LabelManager
{
public:
	LabelManager()
	{
		EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());
		Core::Thread t(ThreadRun, this);
		t.Detach();
	}
	virtual ~LabelManager() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(LabelManager);

	Label* Create(GraphicContext* ctx, uint64_t* dst_gpu_addr, uint64_t value, LabelGpuObject::callback_t callback_1,
	              LabelGpuObject::callback_t callback_2, const uint64_t* args);
	Label* Create(GraphicContext* ctx, uint32_t* dst_gpu_addr, uint32_t value, LabelGpuObject::callback_t callback_1,
	              LabelGpuObject::callback_t callback_2, const uint64_t* args);
	void   Delete(Label* label);
	void   Set(CommandBuffer* buffer, Label* label);

private:
	static void ThreadRun(void* data);

	Core::Mutex    m_mutex;
	Core::CondVar  m_cond_var;
	Vector<Label*> m_labels;
};

static LabelManager* g_label_manager = nullptr;

void LabelManager::ThreadRun(void* data)
{
	auto* manager = static_cast<LabelManager*>(data);

	for (;;)
	{
		manager->m_mutex.Lock();

		int active_count = 0;

		for (auto& label: manager->m_labels)
		{
			if (label->active)
			{
				active_count++;

				if (vkGetEventStatus(label->device, label->event) == VK_EVENT_SET)
				{
					label->active = false;

					bool write = true;

					if (label->callback_1 != nullptr)
					{
						write = label->callback_1(label->args);
					}

					if (write && label->dst_gpu_addr64 != nullptr)
					{
						*label->dst_gpu_addr64 = label->value64;

						printf(FG_BRIGHT_GREEN "EndOfPipe Signal!!! [0x%016" PRIx64 "] <- 0x%016" PRIx64 "\n" FG_DEFAULT,
						       reinterpret_cast<uint64_t>(label->dst_gpu_addr64), label->value64);
					}

					if (write && label->dst_gpu_addr32 != nullptr)
					{
						*label->dst_gpu_addr32 = label->value32;

						printf(FG_BRIGHT_GREEN "EndOfPipe Signal!!! [0x%016" PRIx64 "] <- 0x%08" PRIx32 "\n" FG_DEFAULT,
						       reinterpret_cast<uint64_t>(label->dst_gpu_addr32), label->value32);
					}

					if (label->callback_2 != nullptr)
					{
						label->callback_2(label->args);
					}
				}
			}
		}

		if (active_count == 0)
		{
			manager->m_cond_var.Wait(&manager->m_mutex);
		}

		manager->m_mutex.Unlock();

		Core::Thread::SleepMicro(100);
	}
}

Label* LabelManager::Create(GraphicContext* ctx, uint64_t* dst_gpu_addr, uint64_t value, LabelGpuObject::callback_t callback_1,
                            LabelGpuObject::callback_t callback_2, const uint64_t* args)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);

	Core::LockGuard lock(m_mutex);

	auto* label = new Label;

	label->active         = false;
	label->dst_gpu_addr64 = dst_gpu_addr;
	label->value64        = value;
	label->dst_gpu_addr32 = nullptr;
	label->value32        = 0;
	label->event          = nullptr;
	label->device         = ctx->device;
	label->callback_1     = callback_1;
	label->callback_2     = callback_2;
	label->args[0]        = args[0];
	label->args[1]        = args[1];
	label->args[2]        = args[2];
	label->args[3]        = args[3];

	VkEventCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;

	vkCreateEvent(ctx->device, &create_info, nullptr, &label->event);

	EXIT_NOT_IMPLEMENTED(label->event == nullptr);

	m_labels.Add(label);

	return label;
}

Label* LabelManager::Create(GraphicContext* ctx, uint32_t* dst_gpu_addr, uint32_t value, LabelGpuObject::callback_t callback_1,
                            LabelGpuObject::callback_t callback_2, const uint64_t* args)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(dst_gpu_addr == nullptr);
	EXIT_IF(args == nullptr);

	Core::LockGuard lock(m_mutex);

	auto* label = new Label;

	label->active         = false;
	label->dst_gpu_addr32 = dst_gpu_addr;
	label->value32        = value;
	label->dst_gpu_addr64 = nullptr;
	label->value64        = 0;
	label->event          = nullptr;
	label->device         = ctx->device;
	label->callback_1     = callback_1;
	label->callback_2     = callback_2;
	label->args[0]        = args[0];
	label->args[1]        = args[1];
	label->args[2]        = args[2];
	label->args[3]        = args[3];

	VkEventCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;

	vkCreateEvent(ctx->device, &create_info, nullptr, &label->event);

	EXIT_NOT_IMPLEMENTED(label->event == nullptr);

	m_labels.Add(label);

	return label;
}

void LabelManager::Delete(Label* label)
{
	EXIT_IF(label == nullptr);
	EXIT_IF(label->event == nullptr);
	EXIT_IF(label->device == nullptr);

	Core::LockGuard lock(m_mutex);

	auto index = m_labels.Find(label);

	EXIT_NOT_IMPLEMENTED(!m_labels.IndexValid(index));

	m_labels.RemoveAt(index);

	EXIT_NOT_IMPLEMENTED(label->active);

	vkDestroyEvent(label->device, label->event, nullptr);

	delete label;
}

void LabelManager::Set(CommandBuffer* buffer, Label* label)
{
	EXIT_IF(label == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());
	EXIT_IF(label->event == nullptr);
	EXIT_IF(label->device == nullptr);

	Core::LockGuard lock(m_mutex);

	auto index = m_labels.Find(label);

	EXIT_NOT_IMPLEMENTED(!m_labels.IndexValid(index));

	EXIT_NOT_IMPLEMENTED(label->active);

	label->active = true;

	EXIT_IF(label->event == nullptr);

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	EXIT_NOT_IMPLEMENTED(vk_buffer == nullptr);

	vkResetEvent(label->device, label->event);
	vkCmdSetEvent(vk_buffer, label->event, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

	m_cond_var.Signal();
}

void LabelInit()
{
	EXIT_IF(g_label_manager != nullptr);

	g_label_manager = new LabelManager;
}

Label* LabelCreate(GraphicContext* ctx, uint64_t* dst_gpu_addr, uint64_t value, LabelGpuObject::callback_t callback_1,
                   LabelGpuObject::callback_t callback_2, const uint64_t* args)
{
	EXIT_IF(g_label_manager == nullptr);

	return g_label_manager->Create(ctx, dst_gpu_addr, value, callback_1, callback_2, args);
}

Label* LabelCreate(GraphicContext* ctx, uint32_t* dst_gpu_addr, uint32_t value, LabelGpuObject::callback_t callback_1,
                   LabelGpuObject::callback_t callback_2, const uint64_t* args)
{
	EXIT_IF(g_label_manager == nullptr);

	return g_label_manager->Create(ctx, dst_gpu_addr, value, callback_1, callback_2, args);
}

void LabelDelete(Label* label)
{
	EXIT_IF(g_label_manager == nullptr);

	g_label_manager->Delete(label);
}

void LabelSet(CommandBuffer* buffer, Label* label)
{
	EXIT_IF(g_label_manager == nullptr);

	g_label_manager->Set(buffer, label);
}

void* LabelGpuObject::Create(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, VulkanMemory* /*mem*/) const
{
	KYTY_PROFILER_BLOCK("LabelGpuObject::Create");

	EXIT_IF(vaddr_num != 1 || size == nullptr || vaddr == nullptr || *vaddr == 0);

	EXIT_NOT_IMPLEMENTED(*size != 8 && *size != 4);

	auto value      = params[PARAM_VALUE];
	auto callback_1 = reinterpret_cast<LabelGpuObject::callback_t>(params[PARAM_CALLBACK_1]);
	auto callback_2 = reinterpret_cast<LabelGpuObject::callback_t>(params[PARAM_CALLBACK_2]);

	auto* label_obj =
	    (*size == 8 ? LabelCreate(ctx, reinterpret_cast<uint64_t*>(*vaddr), value, callback_1, callback_2, params + PARAM_ARG_1)
	                : (*size == 4 ? LabelCreate(ctx, reinterpret_cast<uint32_t*>(*vaddr), static_cast<uint32_t>(value), callback_1,
	                                            callback_2, params + PARAM_ARG_1)
	                              : nullptr));

	EXIT_NOT_IMPLEMENTED(label_obj == nullptr);

	return label_obj;
}

static void update_func(GraphicContext* /*ctx*/, const uint64_t* /*params*/, void* /*obj*/, const uint64_t* /*vaddr*/,
                        const uint64_t* /*size*/, int /*vaddr_num*/)
{
	KYTY_PROFILER_BLOCK("LabelGpuObject::update_func");

	KYTY_NOT_IMPLEMENTED;
}

bool LabelGpuObject::Equal(const uint64_t* other) const
{
	return (params[PARAM_VALUE] == other[PARAM_VALUE] && params[PARAM_CALLBACK_1] == other[PARAM_CALLBACK_1] &&
	        params[PARAM_CALLBACK_2] == other[PARAM_CALLBACK_2] && params[PARAM_ARG_1] == other[PARAM_ARG_1] &&
	        params[PARAM_ARG_2] == other[PARAM_ARG_2] && params[PARAM_ARG_3] == other[PARAM_ARG_3] &&
	        params[PARAM_ARG_4] == other[PARAM_ARG_4]);
}

static void delete_func(GraphicContext* /*ctx*/, void* obj, VulkanMemory* /*mem*/)
{
	KYTY_PROFILER_BLOCK("LabelGpuObject::delete_func");

	auto* label_obj = reinterpret_cast<Label*>(obj);

	EXIT_IF(label_obj == nullptr);

	LabelDelete(label_obj);
}

GpuObject::delete_func_t LabelGpuObject::GetDeleteFunc() const
{
	return delete_func;
}

GpuObject::update_func_t LabelGpuObject::GetUpdateFunc() const
{
	return update_func;
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
