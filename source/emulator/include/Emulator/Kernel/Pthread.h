#ifndef EMULATOR_INCLUDE_EMULATOR_KERNEL_PTHREAD_H_
#define EMULATOR_INCLUDE_EMULATOR_KERNEL_PTHREAD_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"

// IWYU pragma: no_include <pthread.h>

#ifdef KYTY_EMU_ENABLED

extern "C" {
struct sched_param;
}

namespace Kyty::Loader {
struct Program;
} // namespace Kyty::Loader

namespace Kyty::Libs::LibKernel {

KYTY_SUBSYSTEM_DEFINE(Pthread);

struct PthreadAttrPrivate;
struct PthreadPrivate;
struct PthreadMutexPrivate;
struct PthreadMutexattrPrivate;
struct PthreadRwlockPrivate;
struct PthreadRwlockattrPrivate;
struct PthreadCondattrPrivate;
struct PthreadCondPrivate;

struct KernelTimespec
{
	int64_t tv_sec;
	int64_t tv_nsec;
};

struct KernelTimeval
{
	int64_t tv_sec;
	int64_t tv_usec;
};

using PthreadAttr       = PthreadAttrPrivate*;
using Pthread           = PthreadPrivate*;
using KernelCpumask     = uint64_t;
using PthreadMutex      = PthreadMutexPrivate*;
using PthreadMutexattr  = PthreadMutexattrPrivate*;
using KernelSchedParam  = struct sched_param;
using PthreadRwlock     = PthreadRwlockPrivate*;
using PthreadRwlockattr = PthreadRwlockattrPrivate*;
using KernelUseconds    = unsigned int;
using PthreadCondattr   = PthreadCondattrPrivate*;
using PthreadCond       = PthreadCondPrivate*;
using KernelClockid     = int32_t;

using pthread_entry_func_t = KYTY_SYSV_ABI void* (*)(void*);
using thread_dtors_func_t  = KYTY_SYSV_ABI void (*)();

void PthreadInitSelfForMainThread();
void PthreadDeleteStaticObjects(Loader::Program* program);

int KYTY_SYSV_ABI PthreadMutexattrInit(PthreadMutexattr* attr);
int KYTY_SYSV_ABI PthreadMutexattrDestroy(PthreadMutexattr* attr);
int KYTY_SYSV_ABI PthreadMutexattrSettype(PthreadMutexattr* attr, int type);
int KYTY_SYSV_ABI PthreadMutexattrSetprotocol(PthreadMutexattr* attr, int protocol);
int KYTY_SYSV_ABI PthreadMutexInit(PthreadMutex* mutex, const PthreadMutexattr* attr, const char* name);
int KYTY_SYSV_ABI PthreadMutexDestroy(PthreadMutex* mutex);
int KYTY_SYSV_ABI PthreadMutexLock(PthreadMutex* mutex);
int KYTY_SYSV_ABI PthreadMutexTrylock(PthreadMutex* mutex);
int KYTY_SYSV_ABI PthreadMutexUnlock(PthreadMutex* mutex);

Pthread KYTY_SYSV_ABI      PthreadSelf();
int KYTY_SYSV_ABI          PthreadCreate(Pthread* thread, const PthreadAttr* attr, pthread_entry_func_t entry, void* arg, const char* name);
int KYTY_SYSV_ABI          PthreadDetach(Pthread thread);
int KYTY_SYSV_ABI          PthreadJoin(Pthread thread, void** value);
int KYTY_SYSV_ABI          PthreadCancel(Pthread thread);
int KYTY_SYSV_ABI          PthreadSetcancelstate(int state, int* old_state);
int KYTY_SYSV_ABI          PthreadSetcanceltype(int type, int* old_type);
void KYTY_SYSV_ABI         PthreadTestcancel();
void KYTY_SYSV_ABI         PthreadExit(void* value);
int KYTY_SYSV_ABI          PthreadEqual(Pthread thread1, Pthread thread2);
int KYTY_SYSV_ABI          PthreadGetname(Pthread thread, char* name);
int KYTY_SYSV_ABI          KernelUsleep(KernelUseconds microseconds);
unsigned int KYTY_SYSV_ABI KernelSleep(unsigned int seconds);
int KYTY_SYSV_ABI          KernelNanosleep(const KernelTimespec* rqtp, KernelTimespec* rmtp);

void KYTY_SYSV_ABI KernelSetThreadDtors(thread_dtors_func_t dtors);

int KYTY_SYSV_ABI PthreadAttrInit(PthreadAttr* attr);
int KYTY_SYSV_ABI PthreadAttrDestroy(PthreadAttr* attr);
int KYTY_SYSV_ABI PthreadAttrGet(Pthread thread, PthreadAttr* attr);
int KYTY_SYSV_ABI PthreadAttrGetaffinity(const PthreadAttr* attr, KernelCpumask* mask);
int KYTY_SYSV_ABI PthreadAttrGetdetachstate(const PthreadAttr* attr, int* state);
int KYTY_SYSV_ABI PthreadAttrGetguardsize(const PthreadAttr* attr, size_t* guard_size);
int KYTY_SYSV_ABI PthreadAttrGetinheritsched(const PthreadAttr* attr, int* inherit_sched);
int KYTY_SYSV_ABI PthreadAttrGetschedparam(const PthreadAttr* attr, KernelSchedParam* param);
int KYTY_SYSV_ABI PthreadAttrGetschedpolicy(const PthreadAttr* attr, int* policy);
int KYTY_SYSV_ABI PthreadAttrGetstack(const PthreadAttr* __restrict attr, void** __restrict stack_addr, size_t* __restrict stack_size);
int KYTY_SYSV_ABI PthreadAttrGetstackaddr(const PthreadAttr* attr, void** stack_addr);
int KYTY_SYSV_ABI PthreadAttrGetstacksize(const PthreadAttr* attr, size_t* stack_size);
int KYTY_SYSV_ABI PthreadAttrSetaffinity(PthreadAttr* attr, KernelCpumask mask);
int KYTY_SYSV_ABI PthreadAttrSetdetachstate(PthreadAttr* attr, int state);
int KYTY_SYSV_ABI PthreadAttrSetguardsize(PthreadAttr* attr, size_t guard_size);
int KYTY_SYSV_ABI PthreadAttrSetinheritsched(PthreadAttr* attr, int inherit_sched);
int KYTY_SYSV_ABI PthreadAttrSetschedparam(PthreadAttr* attr, const KernelSchedParam* param);
int KYTY_SYSV_ABI PthreadAttrSetschedpolicy(PthreadAttr* attr, int policy);
int KYTY_SYSV_ABI PthreadAttrSetstack(PthreadAttr* attr, void* addr, size_t size);
int KYTY_SYSV_ABI PthreadAttrSetstackaddr(PthreadAttr* attr, void* addr);
int KYTY_SYSV_ABI PthreadAttrSetstacksize(PthreadAttr* attr, size_t stack_size);

int KYTY_SYSV_ABI PthreadRwlockDestroy(PthreadRwlock* rwlock);
int KYTY_SYSV_ABI PthreadRwlockInit(PthreadRwlock* rwlock, const PthreadRwlockattr* attr, const char* name);
int KYTY_SYSV_ABI PthreadRwlockRdlock(PthreadRwlock* rwlock);
int KYTY_SYSV_ABI PthreadRwlockTimedrdlock(PthreadRwlock* rwlock, KernelUseconds usec);
int KYTY_SYSV_ABI PthreadRwlockTimedwrlock(PthreadRwlock* rwlock, KernelUseconds usec);
int KYTY_SYSV_ABI PthreadRwlockTryrdlock(PthreadRwlock* rwlock);
int KYTY_SYSV_ABI PthreadRwlockTrywrlock(PthreadRwlock* rwlock);
int KYTY_SYSV_ABI PthreadRwlockUnlock(PthreadRwlock* rwlock);
int KYTY_SYSV_ABI PthreadRwlockWrlock(PthreadRwlock* rwlock);
int KYTY_SYSV_ABI PthreadRwlockattrDestroy(PthreadRwlockattr* attr);
int KYTY_SYSV_ABI PthreadRwlockattrInit(PthreadRwlockattr* attr);
int KYTY_SYSV_ABI PthreadRwlockattrGettype(PthreadRwlockattr* attr, int* type);
int KYTY_SYSV_ABI PthreadRwlockattrSettype(PthreadRwlockattr* attr, int type);

int KYTY_SYSV_ABI PthreadCondattrDestroy(PthreadCondattr* attr);
int KYTY_SYSV_ABI PthreadCondattrInit(PthreadCondattr* attr);
int KYTY_SYSV_ABI PthreadCondBroadcast(PthreadCond* cond);
int KYTY_SYSV_ABI PthreadCondDestroy(PthreadCond* cond);
int KYTY_SYSV_ABI PthreadCondInit(PthreadCond* cond, const PthreadCondattr* attr, const char* name);
int KYTY_SYSV_ABI PthreadCondSignal(PthreadCond* cond);
int KYTY_SYSV_ABI PthreadCondSignalto(PthreadCond* cond, Pthread thread);
int KYTY_SYSV_ABI PthreadCondTimedwait(PthreadCond* cond, PthreadMutex* mutex, KernelUseconds usec);
int KYTY_SYSV_ABI PthreadCondWait(PthreadCond* cond, PthreadMutex* mutex);

int KYTY_SYSV_ABI      KernelClockGetres(KernelClockid clock_id, KernelTimespec* tp);
int KYTY_SYSV_ABI      KernelClockGettime(KernelClockid clock_id, KernelTimespec* tp);
int KYTY_SYSV_ABI      KernelGettimeofday(KernelTimeval* tp);
uint64_t KYTY_SYSV_ABI KernelGetTscFrequency();
uint64_t KYTY_SYSV_ABI KernelReadTsc();
uint64_t KYTY_SYSV_ABI KernelGetProcessTime();
uint64_t KYTY_SYSV_ABI KernelGetProcessTimeCounter();
uint64_t KYTY_SYSV_ABI KernelGetProcessTimeCounterFrequency();

int KYTY_SYSV_ABI pthread_cond_broadcast_s(PthreadCond* cond);
int KYTY_SYSV_ABI pthread_cond_wait_s(PthreadCond* cond, PthreadMutex* mutex);
int KYTY_SYSV_ABI pthread_mutex_lock_s(PthreadMutex* mutex);
int KYTY_SYSV_ABI pthread_mutex_unlock_s(PthreadMutex* mutex);
int KYTY_SYSV_ABI pthread_rwlock_rdlock_s(PthreadRwlock* rwlock);
int KYTY_SYSV_ABI pthread_rwlock_unlock_s(PthreadRwlock* rwlock);
int KYTY_SYSV_ABI pthread_rwlock_wrlock_s(PthreadRwlock* rwlock);

} // namespace Kyty::Libs::LibKernel

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_KERNEL_PTHREAD_H_ */
