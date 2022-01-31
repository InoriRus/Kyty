#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICS_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICS_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"
#include "Emulator/Kernel/EventQueue.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct VsStageRegisters;

KYTY_SUBSYSTEM_DEFINE(Graphics);

void GraphicsDbgDumpDcb(const char* type, uint32_t num_dw, uint32_t* cmd_buffer);

int KYTY_SYSV_ABI      GraphicsSetVsShader(uint32_t* cmd, uint64_t size, const VsStageRegisters* vs_regs, uint32_t shader_modifier);
int KYTY_SYSV_ABI      GraphicsUpdateVsShader(uint32_t* cmd, uint64_t size, const VsStageRegisters* vs_regs, uint32_t shader_modifier);
int KYTY_SYSV_ABI      GraphicsSetPsShader(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs);
int KYTY_SYSV_ABI      GraphicsSetPsShader350(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs);
int KYTY_SYSV_ABI      GraphicsUpdatePsShader(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs);
int KYTY_SYSV_ABI      GraphicsSetCsShaderWithModifier(uint32_t* cmd, uint64_t size, const uint32_t* cs_regs, uint32_t shader_modifier);
int KYTY_SYSV_ABI      GraphicsSetEmbeddedVsShader(uint32_t* cmd, uint64_t size, uint32_t id, uint32_t shader_modifier);
int KYTY_SYSV_ABI      GraphicsDrawIndex(uint32_t* cmd, uint64_t size, uint32_t index_count, const void* index_addr, uint32_t flags,
                                         uint32_t type);
int KYTY_SYSV_ABI      GraphicsDrawIndexAuto(uint32_t* cmd, uint64_t size, uint32_t index_count, uint32_t flags);
int KYTY_SYSV_ABI      GraphicsSubmitCommandBuffers(uint32_t count, void* dcb_gpu_addrs[], const uint32_t* dcb_sizes_in_bytes,
                                                    void* ccb_gpu_addrs[], const uint32_t* ccb_sizes_in_bytes);
int KYTY_SYSV_ABI      GraphicsSubmitAndFlipCommandBuffers(uint32_t count, void* dcb_gpu_addrs[], const uint32_t* dcb_sizes_in_bytes,
                                                           void* ccb_gpu_addrs[], const uint32_t* ccb_sizes_in_bytes, int handle, int index,
                                                           int flip_mode, int64_t flip_arg);
int KYTY_SYSV_ABI      GraphicsSubmitDone();
int KYTY_SYSV_ABI      GraphicsAreSubmitsAllowed();
void KYTY_SYSV_ABI     GraphicsFlushMemory();
int KYTY_SYSV_ABI      GraphicsAddEqEvent(LibKernel::EventQueue::KernelEqueue eq, int id, void* udata);
int KYTY_SYSV_ABI      GraphicsDeleteEqEvent(LibKernel::EventQueue::KernelEqueue eq, int id);
uint32_t KYTY_SYSV_ABI GraphicsDrawInitDefaultHardwareState(uint32_t* cmd, uint64_t size);
uint32_t KYTY_SYSV_ABI GraphicsDrawInitDefaultHardwareState175(uint32_t* cmd, uint64_t size);
uint32_t KYTY_SYSV_ABI GraphicsDrawInitDefaultHardwareState200(uint32_t* cmd, uint64_t size);
uint32_t KYTY_SYSV_ABI GraphicsDrawInitDefaultHardwareState350(uint32_t* cmd, uint64_t size);
uint32_t KYTY_SYSV_ABI GraphicsDispatchInitDefaultHardwareState(uint32_t* cmd, uint64_t size);
int KYTY_SYSV_ABI      GraphicsInsertWaitFlipDone(uint32_t* cmd, uint64_t size, uint32_t video_out_handle, uint32_t display_buffer_index);
int KYTY_SYSV_ABI      GraphicsDispatchDirect(uint32_t* cmd, uint64_t size, uint32_t thread_group_x, uint32_t thread_group_y,
                                              uint32_t thread_group_z, uint32_t mode);
uint32_t KYTY_SYSV_ABI GraphicsMapComputeQueue(uint32_t pipe_id, uint32_t queue_id, uint32_t* ring_addr, uint32_t ring_size_dw,
                                               uint32_t* read_ptr_addr);
int KYTY_SYSV_ABI      GraphicsComputeWaitOnAddress(uint32_t* cmd, uint64_t size, uint32_t* gpu_addr, uint32_t mask, uint32_t func,
                                                    uint32_t ref);
void KYTY_SYSV_ABI     GraphicsDingDong(uint32_t ring_id, uint32_t offset_dw);
void KYTY_SYSV_ABI     GraphicsUnmapComputeQueue(uint32_t id);
int KYTY_SYSV_ABI      GraphicsInsertPushMarker(uint32_t* cmd, uint64_t size, const char* str);
int KYTY_SYSV_ABI      GraphicsInsertPopMarker(uint32_t* cmd, uint64_t size);
uint64_t KYTY_SYSV_ABI GraphicsGetGpuCoreClockFrequency();
int KYTY_SYSV_ABI      GraphicsIsUserPaEnabled();

int KYTY_SYSV_ABI GraphicsRegisterOwner(uint32_t* owner_handle, const char* name);
int KYTY_SYSV_ABI GraphicsRegisterResource(uint32_t* resource_handle, uint32_t owner_handle, const void* memory, size_t size,
                                           const char* name, uint32_t type, uint64_t user_data);
int KYTY_SYSV_ABI GraphicsUnregisterAllResourcesForOwner(uint32_t owner_handle);
int KYTY_SYSV_ABI GraphicsUnregisterOwnerAndResources(uint32_t owner_handle);
int KYTY_SYSV_ABI GraphicsUnregisterResource(uint32_t resource_handle);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICS_H_ */
