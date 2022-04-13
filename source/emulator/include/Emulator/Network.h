#ifndef EMULATOR_INCLUDE_EMULATOR_NETWORK_H_
#define EMULATOR_INCLUDE_EMULATOR_NETWORK_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Network {

KYTY_SUBSYSTEM_DEFINE(Network);

namespace Net {

struct NetEtherAddr;

int KYTY_SYSV_ABI NetInit();
int KYTY_SYSV_ABI NetTerm();
int KYTY_SYSV_ABI NetPoolCreate(const char* name, int size, int flags);
int KYTY_SYSV_ABI NetPoolDestroy(int memid);
int KYTY_SYSV_ABI NetInetPton(int af, const char* src, void* dst);
int KYTY_SYSV_ABI NetEtherNtostr(const NetEtherAddr* n, char* str, size_t len);
int KYTY_SYSV_ABI NetGetMacAddress(NetEtherAddr* addr, int flags);

} // namespace Net

namespace Ssl {

int KYTY_SYSV_ABI SslInit(uint64_t pool_size);
int KYTY_SYSV_ABI SslTerm(int ssl_ctx_id);

} // namespace Ssl

namespace Http {

int KYTY_SYSV_ABI HttpInit(int memid, int ssl_ctx_id, uint64_t pool_size);
int KYTY_SYSV_ABI HttpTerm(int http_ctx_id);
int KYTY_SYSV_ABI HttpCreateTemplate(int http_ctx_id, const char* user_agent, int http_ver, int is_auto_proxy_conf);
int KYTY_SYSV_ABI HttpDeleteTemplate(int tmpl_id);

} // namespace Http

namespace NetCtl {

struct NetCtlNatInfo;
union NetCtlInfo;

using NetCtlCallback = void (*)(int, void*);

int KYTY_SYSV_ABI  NetCtlInit();
void KYTY_SYSV_ABI NetCtlTerm();
int KYTY_SYSV_ABI  NetCtlGetNatInfo(NetCtlNatInfo* nat_info);
int KYTY_SYSV_ABI  NetCtlCheckCallback();
int KYTY_SYSV_ABI  NetCtlGetState(int* state);
int KYTY_SYSV_ABI  NetCtlRegisterCallback(NetCtlCallback func, void* arg, int* cid);
int KYTY_SYSV_ABI  NetCtlGetInfo(int code, NetCtlInfo* info);

} // namespace NetCtl

namespace NpManager {

struct NpTitleId;
struct NpTitleSecret;
struct NpContentRestriction;

int KYTY_SYSV_ABI  NpCheckCallback();
int KYTY_SYSV_ABI  NpSetNpTitleId(const NpTitleId* title_id, const NpTitleSecret* title_secret);
int KYTY_SYSV_ABI  NpSetContentRestriction(const NpContentRestriction* restriction);
int KYTY_SYSV_ABI  NpRegisterStateCallback(void* callback, void* userdata);
void KYTY_SYSV_ABI NpRegisterGamePresenceCallback(void* callback, void* userdata);
int KYTY_SYSV_ABI  NpRegisterPlusEventCallback(void* callback, void* userdata);

} // namespace NpManager

namespace NpManagerForToolkit {

int KYTY_SYSV_ABI NpRegisterStateCallbackForToolkit(void* callback, void* userdata);

} // namespace NpManagerForToolkit

namespace NpTrophy {

int KYTY_SYSV_ABI NpTrophyCreateHandle(int* handle);

} // namespace NpTrophy

namespace NpWebApi {

int KYTY_SYSV_ABI NpWebApiInitialize(int http_ctx_id, size_t pool_size);
int KYTY_SYSV_ABI NpWebApiTerminate(int lib_ctx_id);

} // namespace NpWebApi

} // namespace Kyty::Libs::Network

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_NETWORK_H_ */
