#ifndef EMULATOR_INCLUDE_EMULATOR_AUDIO_H_
#define EMULATOR_INCLUDE_EMULATOR_AUDIO_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Audio {

KYTY_SUBSYSTEM_DEFINE(Audio);

namespace AudioOut {

struct AudioOutOutputParam;
struct AudioOutPortState;

int KYTY_SYSV_ABI AudioOutInit();
int KYTY_SYSV_ABI AudioOutOpen(int user_id, int type, int index, uint32_t len, uint32_t freq, uint32_t param);
int KYTY_SYSV_ABI AudioOutSetVolume(int handle, uint32_t flag, int* vol);
int KYTY_SYSV_ABI AudioOutOutputs(AudioOutOutputParam* param, uint32_t num);
int KYTY_SYSV_ABI AudioOutOutput(int handle, const void* ptr);
int KYTY_SYSV_ABI AudioOutClose(int handle);
int KYTY_SYSV_ABI AudioOutGetPortState(int handle, AudioOutPortState* state);

} // namespace AudioOut

namespace AudioIn {

int KYTY_SYSV_ABI AudioInOpen(int user_id, uint32_t type, uint32_t index, uint32_t len, uint32_t freq, uint32_t param);
int KYTY_SYSV_ABI AudioInInput(int handle, void* dest);

} // namespace AudioIn

namespace VoiceQoS {

int KYTY_SYSV_ABI VoiceQoSInit(void* mem_block, uint32_t mem_size, int32_t app_type);

} // namespace VoiceQoS

namespace Ajm {

int KYTY_SYSV_ABI AjmInitialize(int64_t reserved, uint32_t* context);
int KYTY_SYSV_ABI AjmModuleRegister(uint32_t context, uint32_t codec, int64_t reserved);

} // namespace Ajm

namespace AvPlayer {

struct AvPlayerInitData;
struct AvPlayerFrameInfoEx;
struct AvPlayerFrameInfo;
struct AvPlayerInternal;

using Bool = uint8_t;

AvPlayerInternal* KYTY_SYSV_ABI AvPlayerInit(AvPlayerInitData* init);
int KYTY_SYSV_ABI               AvPlayerAddSource(AvPlayerInternal* h, const char* filename);
int KYTY_SYSV_ABI               AvPlayerSetLooping(AvPlayerInternal* h, Bool loop);
Bool KYTY_SYSV_ABI              AvPlayerGetVideoDataEx(AvPlayerInternal* h, AvPlayerFrameInfoEx* video_info);
Bool KYTY_SYSV_ABI              AvPlayerGetAudioData(AvPlayerInternal* h, AvPlayerFrameInfo* audio_info);
Bool KYTY_SYSV_ABI              AvPlayerIsActive(AvPlayerInternal* h);
int KYTY_SYSV_ABI               AvPlayerClose(AvPlayerInternal* h);

} // namespace AvPlayer

namespace Audio3d {

struct Audio3dOpenParameters;

int KYTY_SYSV_ABI  Audio3dInitialize(int64_t reserved);
void KYTY_SYSV_ABI Audio3dGetDefaultOpenParameters(Audio3dOpenParameters* p);
int KYTY_SYSV_ABI  Audio3dPortOpen(int user_id, const Audio3dOpenParameters* parameters, uint32_t* id);
int KYTY_SYSV_ABI  Audio3dPortSetAttribute(uint32_t port_id, uint32_t attribute_id, const void* attribute, size_t attribute_size);
int KYTY_SYSV_ABI  Audio3dPortGetQueueLevel(uint32_t port_id, uint32_t* queue_level, uint32_t* queue_available);
int KYTY_SYSV_ABI  Audio3dPortAdvance(uint32_t port_id);
int KYTY_SYSV_ABI  Audio3dPortPush(uint32_t port_id, uint32_t blocking);

} // namespace Audio3d

namespace Ngs2 {

struct Ngs2SystemOption;
struct Ngs2RackOption;
struct Ngs2BufferAllocator;
struct Ngs2VoiceParamHeader;
struct Ngs2RenderBufferInfo;
struct Ngs2ContextBufferInfo;
struct Ngs2VoiceState;

int KYTY_SYSV_ABI Ngs2RackQueryBufferSize(uint32_t rack_id, const Ngs2RackOption* option, Ngs2ContextBufferInfo* buffer_info);
int KYTY_SYSV_ABI Ngs2RackCreate(uintptr_t system_handle, uint32_t rack_id, const Ngs2RackOption* option,
                                 const Ngs2ContextBufferInfo* buffer_info, uintptr_t* handle);
int KYTY_SYSV_ABI Ngs2SystemCreateWithAllocator(const Ngs2SystemOption* option, const Ngs2BufferAllocator* allocator, uintptr_t* handle);
int KYTY_SYSV_ABI Ngs2RackCreateWithAllocator(uintptr_t system_handle, uint32_t rack_id, const Ngs2RackOption* option,
                                              const Ngs2BufferAllocator* allocator, uintptr_t* handle);
int KYTY_SYSV_ABI Ngs2RackGetVoiceHandle(uintptr_t rack_handle, uint32_t voice_id, uintptr_t* handle);
int KYTY_SYSV_ABI Ngs2VoiceControl(uintptr_t voice_handle, const Ngs2VoiceParamHeader* param_list);
int KYTY_SYSV_ABI Ngs2VoiceGetState(uintptr_t voice_handle, Ngs2VoiceState* state, size_t state_size);
int KYTY_SYSV_ABI Ngs2SystemRender(uintptr_t system_handle, const Ngs2RenderBufferInfo* buffer_info, uint32_t num_buffer_info);

} // namespace Ngs2

} // namespace Kyty::Libs::Audio

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_AUDIO_H_ */
