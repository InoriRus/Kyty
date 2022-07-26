#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICSRENDER_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICSRENDER_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Kernel/EventQueue.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

namespace HW {
class Context;
class UserConfig;
class Shader;
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
	explicit CommandBuffer(int queue): m_queue(queue) { Allocate(); }
	virtual ~CommandBuffer() { Free(); }

	void              SetParent(CommandProcessor* parent) { m_parent = parent; }
	CommandProcessor* GetParent() { return m_parent; }

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

private:
	VulkanCommandPool* m_pool    = nullptr;
	uint32_t           m_index   = static_cast<uint32_t>(-1);
	int                m_queue   = -1;
	bool               m_execute = false;
	CommandProcessor*  m_parent  = nullptr;
};

void GraphicsRenderInit();
void GraphicsRenderCreateContext();

void GraphicsRenderDrawIndex(uint64_t submit_id, CommandBuffer* buffer, HW::Context* ctx, HW::UserConfig* ucfg, HW::Shader* sh_ctx,
                             uint32_t index_type_and_size, uint32_t index_count, const void* index_addr, uint32_t flags, uint32_t type);
void GraphicsRenderDrawIndexAuto(uint64_t submit_id, CommandBuffer* buffer, HW::Context* ctx, HW::UserConfig* ucfg, HW::Shader* sh_ctx,
                                 uint32_t index_count, uint32_t flags);
void GraphicsRenderWriteAtEndOfPipe64(uint64_t submit_id, CommandBuffer* buffer, uint64_t* dst_gpu_addr, uint64_t value);
void GraphicsRenderWriteAtEndOfPipeClockCounter(uint64_t submit_id, CommandBuffer* buffer, uint64_t* dst_gpu_addr);
void GraphicsRenderWriteAtEndOfPipe32(uint64_t submit_id, CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t value);
void GraphicsRenderWriteAtEndOfPipeGds32(uint64_t submit_id, CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t dw_offset,
                                         uint32_t dw_num);
void GraphicsRenderWriteAtEndOfPipeWithInterruptWriteBackFlip32(uint64_t submit_id, CommandBuffer* buffer, uint32_t* dst_gpu_addr,
                                                                uint32_t value, int handle, int index, int flip_mode, int64_t flip_arg);
void GraphicsRenderWriteAtEndOfPipeWithFlip32(uint64_t submit_id, CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t value, int handle,
                                              int index, int flip_mode, int64_t flip_arg);
void GraphicsRenderWriteAtEndOfPipeOnlyFlip(uint64_t submit_id, CommandBuffer* buffer, int handle, int index, int flip_mode,
                                            int64_t flip_arg);
void GraphicsRenderWriteAtEndOfPipeWithWriteBack64(uint64_t submit_id, CommandBuffer* buffer, uint64_t* dst_gpu_addr, uint64_t value);
void GraphicsRenderWriteAtEndOfPipeWithInterruptWriteBack64(uint64_t submit_id, CommandBuffer* buffer, uint64_t* dst_gpu_addr,
                                                            uint64_t value);
void GraphicsRenderWriteAtEndOfPipeWithInterrupt64(uint64_t submit_id, CommandBuffer* buffer, uint64_t* dst_gpu_addr, uint64_t value);
void GraphicsRenderWriteAtEndOfPipeWithInterrupt32(uint64_t submit_id, CommandBuffer* buffer, uint32_t* dst_gpu_addr, uint32_t value);
void GraphicsRenderWriteBack(CommandProcessor* cp);
void GraphicsRenderDispatchDirect(uint64_t submit_id, CommandBuffer* buffer, HW::Context* ctx, HW::Shader* sh_ctx, uint32_t thread_group_x,
                                  uint32_t thread_group_y, uint32_t thread_group_z, uint32_t mode);
void GraphicsRenderMemoryBarrier(CommandBuffer* buffer);
void GraphicsRenderRenderTextureBarrier(CommandBuffer* buffer, uint64_t vaddr, uint64_t size);
void GraphicsRenderDepthStencilBarrier(CommandBuffer* buffer, uint64_t vaddr, uint64_t size);
void GraphicsRenderMemoryFree(uint64_t vaddr, uint64_t size);
void GraphicsRenderDeleteIndexBuffers();
void GraphicsRenderMemoryFlush(uint64_t vaddr, uint64_t size);

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
