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

struct HttpEpoll;

using HttpEpollHandle = HttpEpoll*;

using HttpsCallback = KYTY_SYSV_ABI int (*)(int, unsigned int, void* const*, int, void*);

int KYTY_SYSV_ABI HttpInit(int memid, int ssl_ctx_id, uint64_t pool_size);
int KYTY_SYSV_ABI HttpTerm(int http_ctx_id);
int KYTY_SYSV_ABI HttpCreateTemplate(int http_ctx_id, const char* user_agent, int http_ver, int is_auto_proxy_conf);
int KYTY_SYSV_ABI HttpDeleteTemplate(int tmpl_id);
int KYTY_SYSV_ABI HttpSetNonblock(int id, int enable);
int KYTY_SYSV_ABI HttpCreateEpoll(int http_ctx_id, HttpEpollHandle* eh);
int KYTY_SYSV_ABI HttpDestroyEpoll(int http_ctx_id, HttpEpollHandle eh);
int KYTY_SYSV_ABI HttpCreateConnectionWithURL(int tmpl_id, const char* url, int enable_keep_alive);
int KYTY_SYSV_ABI HttpDeleteConnection(int conn_id);
int KYTY_SYSV_ABI HttpCreateRequestWithURL2(int conn_id, const char* method, const char* url, uint64_t content_length);
int KYTY_SYSV_ABI HttpDeleteRequest(int req_id);
int KYTY_SYSV_ABI HttpAddRequestHeader(int id, const char* name, const char* value, uint32_t mode);
int KYTY_SYSV_ABI HttpSetEpoll(int id, HttpEpollHandle eh, void* user_arg);
int KYTY_SYSV_ABI HttpUnsetEpoll(int id);
int KYTY_SYSV_ABI HttpSendRequest(int request_id, const void* post_data, size_t size);
int KYTY_SYSV_ABI HttpsSetSslCallback(int id, HttpsCallback cbfunc, void* user_arg);
int KYTY_SYSV_ABI HttpsDisableOption(int id, uint32_t ssl_flags);
int KYTY_SYSV_ABI HttpSetResolveTimeOut(int id, uint32_t usec);
int KYTY_SYSV_ABI HttpSetResolveRetry(int id, int32_t retry);
int KYTY_SYSV_ABI HttpSetConnectTimeOut(int id, uint32_t usec);
int KYTY_SYSV_ABI HttpSetSendTimeOut(int id, uint32_t usec);
int KYTY_SYSV_ABI HttpSetRecvTimeOut(int id, uint32_t usec);
int KYTY_SYSV_ABI HttpSetAutoRedirect(int id, int enable);
int KYTY_SYSV_ABI HttpSetAuthEnabled(int id, int enable);

} // namespace Http

namespace NetCtl {

struct NetCtlNatInfo;
union NetCtlInfo;

using NetCtlCallback = KYTY_SYSV_ABI void (*)(int, void*);

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
struct NpId;
struct NpOnlineId;
struct NpCreateAsyncRequestParameter;

int KYTY_SYSV_ABI  NpCheckCallback();
int KYTY_SYSV_ABI  NpSetNpTitleId(const NpTitleId* title_id, const NpTitleSecret* title_secret);
int KYTY_SYSV_ABI  NpSetContentRestriction(const NpContentRestriction* restriction);
int KYTY_SYSV_ABI  NpRegisterStateCallback(void* callback, void* userdata);
void KYTY_SYSV_ABI NpRegisterGamePresenceCallback(void* callback, void* userdata);
int KYTY_SYSV_ABI  NpRegisterPlusEventCallback(void* callback, void* userdata);
int KYTY_SYSV_ABI  NpRegisterNpReachabilityStateCallback(void* callback, void* userdata);
int KYTY_SYSV_ABI  NpGetNpId(int user_id, NpId* np_id);
int KYTY_SYSV_ABI  NpGetOnlineId(int user_id, NpOnlineId* online_id);
int KYTY_SYSV_ABI  NpCreateAsyncRequest(const NpCreateAsyncRequestParameter* param);
int KYTY_SYSV_ABI  NpDeleteRequest(int req_id);
int KYTY_SYSV_ABI  NpCheckNpAvailability(int req_id, const char* user, void* result);
int KYTY_SYSV_ABI  NpPollAsync(int req_id, int* result);
int KYTY_SYSV_ABI  NpGetState(int user_id, uint32_t* state);

} // namespace NpManager

namespace NpManagerForToolkit {

int KYTY_SYSV_ABI NpRegisterStateCallbackForToolkit(void* callback, void* userdata);
int KYTY_SYSV_ABI NpCheckCallbackForLib();

} // namespace NpManagerForToolkit

namespace NpTrophy {

struct NpTrophyFlagArray;

int KYTY_SYSV_ABI NpTrophyCreateHandle(int* handle);
int KYTY_SYSV_ABI NpTrophyCreateContext(int* context, int user_id, uint32_t service_label, uint64_t options);
int KYTY_SYSV_ABI NpTrophyRegisterContext(int context, int handle, uint64_t options);
int KYTY_SYSV_ABI NpTrophyDestroyHandle(int handle);
int KYTY_SYSV_ABI NpTrophyGetTrophyUnlockState(int context, int handle, NpTrophyFlagArray* flags, uint32_t* count);

} // namespace NpTrophy

namespace NpWebApi {

int KYTY_SYSV_ABI NpWebApiInitialize(int http_ctx_id, size_t pool_size);
int KYTY_SYSV_ABI NpWebApiTerminate(int lib_ctx_id);

} // namespace NpWebApi

} // namespace Kyty::Libs::Network

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_NETWORK_H_ */
