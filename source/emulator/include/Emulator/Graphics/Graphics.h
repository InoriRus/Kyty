#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICS_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICS_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"
#include "Emulator/Kernel/EventQueue.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct Shader;
struct ShaderRegister;

KYTY_SUBSYSTEM_DEFINE(Graphics);

void GraphicsDbgDumpDcb(const char* type, uint32_t num_dw, uint32_t* cmd_buffer);

namespace Gen4 {

int KYTY_SYSV_ABI      GraphicsSetVsShader(uint32_t* cmd, uint64_t size, const uint32_t* vs_regs, uint32_t shader_modifier);
int KYTY_SYSV_ABI      GraphicsUpdateVsShader(uint32_t* cmd, uint64_t size, const uint32_t* vs_regs, uint32_t shader_modifier);
int KYTY_SYSV_ABI      GraphicsSetPsShader(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs);
int KYTY_SYSV_ABI      GraphicsSetPsShader350(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs);
int KYTY_SYSV_ABI      GraphicsUpdatePsShader(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs);
int KYTY_SYSV_ABI      GraphicsUpdatePsShader350(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs);
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
void* KYTY_SYSV_ABI    GraphicsGetTheTessellationFactorRingBufferBaseAddress();

int KYTY_SYSV_ABI GraphicsRegisterOwner(uint32_t* owner_handle, const char* name);
int KYTY_SYSV_ABI GraphicsRegisterResource(uint32_t* resource_handle, uint32_t owner_handle, const void* memory, size_t size,
                                           const char* name, uint32_t type, uint64_t user_data);
int KYTY_SYSV_ABI GraphicsUnregisterAllResourcesForOwner(uint32_t owner_handle);
int KYTY_SYSV_ABI GraphicsUnregisterOwnerAndResources(uint32_t owner_handle);
int KYTY_SYSV_ABI GraphicsUnregisterResource(uint32_t resource_handle);

} // namespace Gen4

namespace Gen5 {

struct CommandBuffer;
struct Label;

int KYTY_SYSV_ABI   GraphicsInit(uint32_t* state, uint32_t ver);
void* KYTY_SYSV_ABI GraphicsGetRegisterDefaults2(uint32_t ver);
void* KYTY_SYSV_ABI GraphicsGetRegisterDefaults2Internal(uint32_t ver);
int KYTY_SYSV_ABI   GraphicsCreateShader(Shader** dst, void* header, const volatile void* code);
int KYTY_SYSV_ABI   GraphicsSetCxRegIndirectPatchSetAddress(uint32_t* cmd, const volatile ShaderRegister* regs);
int KYTY_SYSV_ABI   GraphicsSetShRegIndirectPatchSetAddress(uint32_t* cmd, const volatile ShaderRegister* regs);
int KYTY_SYSV_ABI   GraphicsSetUcRegIndirectPatchSetAddress(uint32_t* cmd, const volatile ShaderRegister* regs);
int KYTY_SYSV_ABI   GraphicsSetCxRegIndirectPatchAddRegisters(uint32_t* cmd, uint32_t num_regs);
int KYTY_SYSV_ABI   GraphicsSetShRegIndirectPatchAddRegisters(uint32_t* cmd, uint32_t num_regs);
int KYTY_SYSV_ABI   GraphicsSetUcRegIndirectPatchAddRegisters(uint32_t* cmd, uint32_t num_regs);
int KYTY_SYSV_ABI   GraphicsCreatePrimState(ShaderRegister* cx_regs, ShaderRegister* uc_regs, const Shader* hs, const Shader* gs,
                                            uint32_t prim_type);
int KYTY_SYSV_ABI   GraphicsCreateInterpolantMapping(ShaderRegister* regs, const Shader* gs, const Shader* ps);
int KYTY_SYSV_ABI   GraphicsGetDataPacketPayloadAddress(uint32_t** addr, uint32_t* cmd, int type);
int KYTY_SYSV_ABI   GraphicsSuspendPoint();

uint32_t* KYTY_SYSV_ABI GraphicsCbSetShRegisterRangeDirect(CommandBuffer* buf, uint32_t offset, const uint32_t* values,
                                                           uint32_t num_values);
uint32_t* KYTY_SYSV_ABI GraphicsCbReleaseMem(CommandBuffer* buf, uint8_t action, uint16_t gcr_cntl, uint8_t dst, uint8_t cache_policy,
                                             const volatile Label* address, uint8_t data_sel, uint64_t data, uint16_t gds_offset,
                                             uint16_t gds_size, uint8_t interrupt, uint32_t interrupt_ctx_id);
uint32_t* KYTY_SYSV_ABI GraphicsDcbResetQueue(CommandBuffer* buf, uint32_t op, uint32_t state);
uint32_t* KYTY_SYSV_ABI GraphicsDcbWaitUntilSafeForRendering(CommandBuffer* buf, uint32_t video_out_handle, uint32_t display_buffer_index);
uint32_t* KYTY_SYSV_ABI GraphicsDcbSetShRegisterDirect(CommandBuffer* buf, ShaderRegister reg);
uint32_t* KYTY_SYSV_ABI GraphicsDcbSetCxRegistersIndirect(CommandBuffer* buf, const volatile ShaderRegister* regs, uint32_t num_regs);
uint32_t* KYTY_SYSV_ABI GraphicsDcbSetShRegistersIndirect(CommandBuffer* buf, const volatile ShaderRegister* regs, uint32_t num_regs);
uint32_t* KYTY_SYSV_ABI GraphicsDcbSetUcRegistersIndirect(CommandBuffer* buf, const volatile ShaderRegister* regs, uint32_t num_regs);
uint32_t* KYTY_SYSV_ABI GraphicsDcbSetIndexSize(CommandBuffer* buf, uint8_t index_size, uint8_t cache_policy);
uint32_t* KYTY_SYSV_ABI GraphicsDcbDrawIndexAuto(CommandBuffer* buf, uint32_t index_count, uint64_t modifier);
uint32_t* KYTY_SYSV_ABI GraphicsDcbEventWrite(CommandBuffer* buf, uint8_t event_type, const volatile void* address);
uint32_t* KYTY_SYSV_ABI GraphicsDcbAcquireMem(CommandBuffer* buf, uint8_t engine, uint32_t cb_db_op, uint32_t gcr_cntl,
                                              const volatile void* base, uint64_t size_bytes, uint32_t poll_cycles);
uint32_t* KYTY_SYSV_ABI GraphicsDcbWriteData(CommandBuffer* buf, uint8_t dst, uint8_t cache_policy, uint64_t address_or_offset,
                                             const void* data, uint32_t num_dwords, uint8_t increment, uint8_t write_confirm);
uint32_t* KYTY_SYSV_ABI GraphicsDcbWaitRegMem(CommandBuffer* buf, uint8_t size, uint8_t compare_function, uint8_t op, uint8_t cache_policy,
                                              const volatile void* address, uint64_t reference, uint64_t mask, uint32_t poll_cycles);
uint32_t* KYTY_SYSV_ABI GraphicsDcbSetFlip(CommandBuffer* buf, uint32_t video_out_handle, int32_t display_buffer_index, uint32_t flip_mode,
                                           int64_t flip_arg);

} // namespace Gen5

namespace Gen5Driver {

struct Packet;

int KYTY_SYSV_ABI GraphicsDriverSubmitDcb(const Packet* packet);

} // namespace Gen5Driver

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICS_H_ */
