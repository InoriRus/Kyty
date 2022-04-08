#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICSRENDER_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICSRENDER_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Kernel/EventQueue.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

namespace HW {
class HardwareContext;
class UserConfig;
} // namespace HW

class CommandProcessor;
struct VideoOutVulkanImage;
struct DepthStencilVulkanImage;
struct TextureVulkanImage;
struct StorageTextureVulkanImage;
struct RenderTextureVulkanImage;
struct VulkanCommandPool;
struct VulkanBuffer;
struct VulkanFramebuffer;
struct RenderDepthInfo;
struct RenderColorInfo;

class CommandBuffer
{
public:
	CommandBuffer() { Allocate(); }
	virtual ~CommandBuffer() { Free(); }

	void SetParent(CommandProcessor* parent) { m_parent = parent; }

	KYTY_CLASS_NO_COPY(CommandBuffer);

	[[nodiscard]] bool IsInvalid() const;

	void Allocate();
	void Free();
	void Begin() const;
	void End() const;
	void Execute();
	void ExecuteWithSemaphore();
	void BeginRenderPass(VulkanFramebuffer* framebuffer, RenderColorInfo* color, RenderDepthInfo* depth) const;
	void EndRenderPass() const;
	void WaitForFence();
	void WaitForFenceAndReset();

	[[nodiscard]] uint32_t GetIndex() const { return m_index; }
	VulkanCommandPool*     GetPool() { return m_pool; }
	[[nodiscard]] bool     IsExecute() const { return m_execute; }

	void SetQueue(int queue) { m_queue = queue; }

	void CommandProcessorWait();

private:
	VulkanCommandPool* m_pool    = nullptr;
	uint32_t           m_index   = static_cast<uint32_t>(-1);
	int                m_queue   = -1;
	bool               m_execute = false;
	CommandProcessor*  m_parent  = nullptr;
};

void GraphicsRenderInit();
void GraphicsRenderCreateContext();

void GraphicsRenderDrawIndex(CommandBuffer* buffer, HW::HardwareContext* ctx, HW::UserConfig* ucfg, uint32_t index_type_and_size,
                             uint32_t index_count, const void* index_addr, uint32_t flags, uint32_t type);
void GraphicsRenderDrawIndexAuto(CommandBuffer* buffer, HW::HardwareContext* ctx, HW::UserConfig* ucfg, uint32_t index_count,
                                 uint32_t flags);
void GraphicsRenderWriteAtEndOfPipe(CommandBuffer* buffer, uint64_t* dst_gpu_addr, uint64_t value);
void GraphicsRenderWriteAtEndOfPipeClockCounter(CommandBuffer* buffer, uint64_t* dst_gpu_addr);
void GraphicsRenderWriteAtEndOfPipe(CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t value);
void GraphicsRenderWriteAtEndOfPipeGds(CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t dw_offset, uint32_t dw_num);
void GraphicsRenderWriteAtEndOfPipeWithInterruptWriteBackFlip(CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t value, int handle,
                                                              int index, int flip_mode, int64_t flip_arg);
void GraphicsRenderWriteAtEndOfPipeWithFlip(CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t value, int handle, int index,
                                            int flip_mode, int64_t flip_arg);
void GraphicsRenderWriteAtEndOfPipeWithWriteBack(CommandBuffer* buffer, uint64_t* dst_gpu_addr, uint64_t value);
void GraphicsRenderWriteAtEndOfPipeWithInterruptWriteBack(CommandBuffer* buffer, uint64_t* dst_gpu_addr, uint64_t value);
void GraphicsRenderWriteAtEndOfPipeWithInterrupt(CommandBuffer* buffer, uint64_t* dst_gpu_addr, uint64_t value);
void GraphicsRenderWriteBack();
void GraphicsRenderDispatchDirect(CommandBuffer* buffer, HW::HardwareContext* ctx, uint32_t thread_group_x, uint32_t thread_group_y,
                                  uint32_t thread_group_z, uint32_t mode);
void GraphicsRenderMemoryBarrier(CommandBuffer* buffer);
void GraphicsRenderRenderTextureBarrier(CommandBuffer* buffer, uint64_t vaddr, uint64_t size);
void GraphicsRenderDepthStencilBarrier(CommandBuffer* buffer, uint64_t vaddr, uint64_t size);
void GraphicsRenderMemoryFree(uint64_t vaddr, uint64_t size);

void DeleteFramebuffer(VideoOutVulkanImage* image);
void DeleteFramebuffer(DepthStencilVulkanImage* image);
void DeleteFramebuffer(RenderTextureVulkanImage* image);
void DeleteDescriptor(VulkanBuffer* buffer);
void DeleteDescriptor(TextureVulkanImage* image);
void DeleteDescriptor(StorageTextureVulkanImage* image);
void DeleteDescriptor(RenderTextureVulkanImage* image);

int GraphicsRenderAddEqEvent(LibKernel::EventQueue::KernelEqueue eq, int id, void* udata);
int GraphicsRenderDeleteEqEvent(LibKernel::EventQueue::KernelEqueue eq, int id);

void GraphicsRenderClearGds(uint64_t dw_offset, uint32_t dw_num, uint32_t clear_value);
void GraphicsRenderReadGds(uint32_t* dst, uint32_t dw_offset, uint32_t dw_size);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICSRENDER_H_ */
