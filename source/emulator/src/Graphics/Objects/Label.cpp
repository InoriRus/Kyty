#include "Emulator/Graphics/Objects/Label.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/GraphicsRun.h"
#include "Emulator/Profiler.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {


enum LabelStatus
{
	New,
	Active,
	ActiveDeleted,
	NotActive,
};

struct LabelCallbacks
{
	uint64_t*                  dst_gpu_addr64       = nullptr;
	uint64_t                   value64              = 0;
	uint32_t*                  dst_gpu_addr32       = nullptr;
	uint32_t                   value32              = 0;
	LabelGpuObject::callback_t callback_1           = nullptr;
	LabelGpuObject::callback_t callback_2           = nullptr;
	uint64_t                   args[LABEL_ARGS_MAX] = {};
};

struct Label
{
	VkDevice          device = nullptr;
	VkEvent           event  = nullptr;
	LabelStatus       status = LabelStatus::New;
	LabelCallbacks    callbacks;
	CommandBuffer*    cb     = nullptr;
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

	Label* Create64(GraphicContext* ctx, uint64_t* dst_gpu_addr, uint64_t value, LabelGpuObject::callback_t callback_1,
	                LabelGpuObject::callback_t callback_2, const uint64_t* args);
	Label* Create32(GraphicContext* ctx, uint32_t* dst_gpu_addr, uint32_t value, LabelGpuObject::callback_t callback_1,
	                LabelGpuObject::callback_t callback_2, const uint64_t* args);
	void   Delete(Label* label);
	void   Set(CommandBuffer* buffer, Label* label);

private:
	static void ThreadRun(void* data);

	bool        Remove(Label* label);
	static void Destroy(Label* label);

	Core::Mutex    m_mutex;
	Core::CondVar  m_cond_var;
	bool m_cond_flag = false;
	Vector<Label*> m_labels;
};

static LabelManager* g_label_manager = nullptr;

void LabelManager::ThreadRun(void* data)
{
	auto* manager = static_cast<LabelManager*>(data);

	for (;;)
	{
		manager->m_mutex.Lock();
		while (!manager->m_cond_flag)
		{
			manager->m_cond_var.Wait(&manager->m_mutex);
		}
		manager->m_cond_flag = false;
		Vector<Label*>         deleted_labels;

		for (auto& label: manager->m_labels)
		{
			if (label->status == LabelStatus::Active || label->status == LabelStatus::ActiveDeleted)
			{
				auto status = vkGetEventStatus(label->device, label->event);
				// May return reset -> try again (~1 time)
				while (status == VK_EVENT_RESET)
				{
					label->cb->WaitForFence();
					status = vkGetEventStatus(label->device, label->event);
				}
				
				if (status == VK_EVENT_SET)
				{
					if (label->status == LabelStatus::ActiveDeleted)
					{
						deleted_labels.Add(label);
					}
				}
			}
		}
		
		for (auto& label: deleted_labels)
		{
			bool removed = manager->Remove(label);
			EXIT_NOT_IMPLEMENTED(!removed);
		}

		manager->m_mutex.Unlock();

		for (auto& label: deleted_labels)
		{
			Destroy(label);
		}

		for (auto& label: manager->m_labels)
		{
			if (label->status != LabelStatus::Active){
				continue;
			}
			label->status = LabelStatus::NotActive;

			auto callback = label->callbacks;
			bool write = true;

			if (callback.callback_1 != nullptr)
			{
				write = callback.callback_1(callback.args);
			}

			if (write && callback.dst_gpu_addr64 != nullptr)
			{
				*callback.dst_gpu_addr64 = callback.value64;

				printf(FG_BRIGHT_GREEN "EndOfPipe Signal!!! [0x%016" PRIx64 "] <- 0x%016" PRIx64 "\n" FG_DEFAULT,
				       reinterpret_cast<uint64_t>(callback.dst_gpu_addr64), callback.value64);
			}

			if (write && callback.dst_gpu_addr32 != nullptr)
			{
				*callback.dst_gpu_addr32 = callback.value32;

				printf(FG_BRIGHT_GREEN "EndOfPipe Signal!!! [0x%016" PRIx64 "] <- 0x%08" PRIx32 "\n" FG_DEFAULT,
				       reinterpret_cast<uint64_t>(callback.dst_gpu_addr32), callback.value32);
			}

			if (callback.callback_2 != nullptr)
			{
				callback.callback_2(callback.args);
			}
		}
	}
}

Label* LabelManager::Create64(GraphicContext* ctx, uint64_t* dst_gpu_addr, uint64_t value, LabelGpuObject::callback_t callback_1,
                              LabelGpuObject::callback_t callback_2, const uint64_t* args)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(args == nullptr);
	
	auto* label = new Label;

	label->status                   = LabelStatus::New;
	label->callbacks.dst_gpu_addr64 = dst_gpu_addr;
	label->callbacks.value64        = value;
	label->callbacks.dst_gpu_addr32 = nullptr;
	label->callbacks.value32        = 0;
	label->event                    = nullptr;
	label->device                   = ctx->device;
	label->callbacks.callback_1     = callback_1;
	label->callbacks.callback_2     = callback_2;
	label->cb                       = nullptr;

	for (int i = 0; i < LABEL_ARGS_MAX; i++)
	{
		label->callbacks.args[i] = args[i];
	}

	VkEventCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;

	{
		Core::LockGuard lock(m_mutex);
		vkCreateEvent(ctx->device, &create_info, nullptr, &label->event);

		EXIT_NOT_IMPLEMENTED(label->event == nullptr);

		m_labels.Add(label);
	}

	return label;
}

Label* LabelManager::Create32(GraphicContext* ctx, uint32_t* dst_gpu_addr, uint32_t value, LabelGpuObject::callback_t callback_1,
                              LabelGpuObject::callback_t callback_2, const uint64_t* args)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(args == nullptr);

	auto* label = new Label;

	label->status                   = LabelStatus::New;
	label->callbacks.dst_gpu_addr32 = dst_gpu_addr;
	label->callbacks.value32        = value;
	label->callbacks.dst_gpu_addr64 = nullptr;
	label->callbacks.value64        = 0;
	label->event                    = nullptr;
	label->device                   = ctx->device;
	label->callbacks.callback_1     = callback_1;
	label->callbacks.callback_2     = callback_2;
	label->cb                       = nullptr;

	for (int i = 0; i < LABEL_ARGS_MAX; i++)
	{
		label->callbacks.args[i] = args[i];
	}

	VkEventCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;

	{
		Core::LockGuard lock(m_mutex);
		vkCreateEvent(ctx->device, &create_info, nullptr, &label->event);

		EXIT_NOT_IMPLEMENTED(label->event == nullptr);

		m_labels.Add(label);
	}
	return label;
}

bool LabelManager::Remove(Label* label)
{
	EXIT_IF(label == nullptr);
	EXIT_IF(label->event == nullptr);
	EXIT_IF(label->device == nullptr);
	
	Core::LockGuard lock(m_mutex);

	auto index = m_labels.Find(label);

	EXIT_NOT_IMPLEMENTED(!m_labels.IndexValid(index));

	EXIT_NOT_IMPLEMENTED(label->status != LabelStatus::NotActive && label->status != LabelStatus::Active);

	if (label->status == LabelStatus::Active)
	{
		label->status = LabelStatus::ActiveDeleted;

		return false;
	}
	
	m_labels.RemoveAt(index);

	return true;
}

void LabelManager::Destroy(Label* label)
{
	EXIT_IF(label == nullptr);
	EXIT_IF(label->event == nullptr);
	EXIT_IF(label->device == nullptr);
	EXIT_IF(label->cb == nullptr);

	// All submitted commands that refer to event must have completed execution
	GraphicsRunCommandProcessorWait(label->cb->GetParent());

	vkDestroyEvent(label->device, label->event, nullptr);

	delete label;
}

void LabelManager::Delete(Label* label)
{
	if (Remove(label))
	{
		Destroy(label);
	}
}

void LabelManager::Set(CommandBuffer* buffer, Label* label)
{
	EXIT_IF(label == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->IsInvalid());
	EXIT_IF(label->event == nullptr);
	EXIT_IF(label->device == nullptr);

	{
		Core::LockGuard lock(m_mutex);
		auto index = m_labels.Find(label);

		EXIT_NOT_IMPLEMENTED(!m_labels.IndexValid(index));

		EXIT_NOT_IMPLEMENTED(label->status != LabelStatus::New && label->status != LabelStatus::NotActive);

		label->status = LabelStatus::Active;

		EXIT_IF(label->event == nullptr);

		label->cb = buffer;
		auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

		EXIT_NOT_IMPLEMENTED(vk_buffer == nullptr);
		
		vkResetEvent(label->device, label->event);
		vkCmdSetEvent(vk_buffer, label->event, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
		m_cond_flag = true;
	}
	m_cond_var.Signal();
}

void LabelInit()
{
	EXIT_IF(g_label_manager != nullptr);

	g_label_manager = new LabelManager;
}

Label* LabelCreate64(GraphicContext* ctx, uint64_t* dst_gpu_addr, uint64_t value, LabelGpuObject::callback_t callback_1,
                     LabelGpuObject::callback_t callback_2, const uint64_t* args)
{
	EXIT_IF(g_label_manager == nullptr);

	return g_label_manager->Create64(ctx, dst_gpu_addr, value, callback_1, callback_2, args);
}

Label* LabelCreate32(GraphicContext* ctx, uint32_t* dst_gpu_addr, uint32_t value, LabelGpuObject::callback_t callback_1,
                     LabelGpuObject::callback_t callback_2, const uint64_t* args)
{
	EXIT_IF(g_label_manager == nullptr);

	return g_label_manager->Create32(ctx, dst_gpu_addr, value, callback_1, callback_2, args);
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

static void* create_func(GraphicContext* ctx, const uint64_t* params, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                         VulkanMemory* /*mem*/)
{
	KYTY_PROFILER_BLOCK("LabelGpuObject::Create");

	EXIT_IF(vaddr_num != 1 || size == nullptr || vaddr == nullptr || *vaddr == 0);

	EXIT_NOT_IMPLEMENTED(*size != 8 && *size != 4);

	auto value      = params[LabelGpuObject::PARAM_VALUE];
	auto callback_1 = reinterpret_cast<LabelGpuObject::callback_t>(params[LabelGpuObject::PARAM_CALLBACK_1]);
	auto callback_2 = reinterpret_cast<LabelGpuObject::callback_t>(params[LabelGpuObject::PARAM_CALLBACK_2]);

	auto* label_obj = (*size == 8 ? LabelCreate64(ctx, reinterpret_cast<uint64_t*>(*vaddr), value, callback_1, callback_2,
	                                              params + LabelGpuObject::PARAM_ARG_1)
	                              : (*size == 4 ? LabelCreate32(ctx, reinterpret_cast<uint32_t*>(*vaddr), static_cast<uint32_t>(value),
	                                                            callback_1, callback_2, params + LabelGpuObject::PARAM_ARG_1)
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

static void delete_func(GraphicContext* /*ctx*/, void* obj, VulkanMemory* /*mem*/)
{
	KYTY_PROFILER_BLOCK("LabelGpuObject::delete_func");

	auto* label_obj = reinterpret_cast<Label*>(obj);

	EXIT_IF(label_obj == nullptr);

	LabelDelete(label_obj);
}

bool LabelGpuObject::Equal(const uint64_t* other) const
{
	return (params[PARAM_VALUE] == other[PARAM_VALUE] && params[PARAM_CALLBACK_1] == other[PARAM_CALLBACK_1] &&
	        params[PARAM_CALLBACK_2] == other[PARAM_CALLBACK_2] && params[PARAM_ARG_1] == other[PARAM_ARG_1] &&
	        params[PARAM_ARG_2] == other[PARAM_ARG_2] && params[PARAM_ARG_3] == other[PARAM_ARG_3] &&
	        params[PARAM_ARG_4] == other[PARAM_ARG_4]);
}

GpuObject::create_func_t LabelGpuObject::GetCreateFunc() const
{
	return create_func;
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
