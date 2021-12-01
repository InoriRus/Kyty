#ifndef EMULATOR_INCLUDE_EMULATOR_KERNEL_EVENTFLAG_H_
#define EMULATOR_INCLUDE_EMULATOR_KERNEL_EVENTFLAG_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Kernel/Pthread.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::LibKernel::EventFlag {

class KernelEventFlagPrivate;

using KernelEventFlag = KernelEventFlagPrivate*;

int KYTY_SYSV_ABI KernelCreateEventFlag(KernelEventFlag* ef, const char* name, uint32_t attr, uint64_t init_pattern, const void* param);
int KYTY_SYSV_ABI KernelDeleteEventFlag(KernelEventFlag ef);
int KYTY_SYSV_ABI KernelWaitEventFlag(KernelEventFlag ef, uint64_t bit_pattern, uint32_t wait_mode, uint64_t* result_pat,
                                      KernelUseconds* timeout);
int KYTY_SYSV_ABI KernelPollEventFlag(KernelEventFlag ef, uint64_t bit_pattern, uint32_t wait_mode, uint64_t* result_pat);
int KYTY_SYSV_ABI KernelSetEventFlag(KernelEventFlag ef, uint64_t bit_pattern);
int KYTY_SYSV_ABI KernelClearEventFlag(KernelEventFlag ef, uint64_t bit_pattern);
int KYTY_SYSV_ABI KernelCancelEventFlag(KernelEventFlag ef, uint64_t set_pattern, int* num_wait_threads);

} // namespace Kyty::Libs::LibKernel::EventFlag

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_KERNEL_EVENTFLAG_H_ */
