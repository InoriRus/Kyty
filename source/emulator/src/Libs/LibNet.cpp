#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Network.h"
#include "Emulator/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NET_CALL(func)                                                                                                                     \
	[&]()                                                                                                                                  \
	{                                                                                                                                      \
		auto result = func;                                                                                                                \
		if (result < 0)                                                                                                                    \
		{                                                                                                                                  \
			*GetNetErrorAddr() = result;                                                                                                   \
		}                                                                                                                                  \
		return result;                                                                                                                     \
	}()

namespace Kyty::Libs {

namespace Network::Net {
struct NetEtherAddr;
} // namespace Network::Net

namespace LibNet {

LIB_VERSION("Net", 1, "Net", 1, 1);

static thread_local int g_net_errno = 0;

namespace Net = Network::Net;

KYTY_SYSV_ABI int* GetNetErrorAddr()
{
	return &g_net_errno;
}

static int KYTY_SYSV_ABI NetInit()
{
	return NET_CALL(Net::NetInit());
}

int KYTY_SYSV_ABI NetPoolCreate(const char* name, int size, int flags)
{
	return NET_CALL(Net::NetPoolCreate(name, size, flags));
}

int KYTY_SYSV_ABI NetInetPton(int af, const char* src, void* dst)
{
	return NET_CALL(Net::NetInetPton(af, src, dst));
}

int KYTY_SYSV_ABI NetEtherNtostr(const Net::NetEtherAddr* n, char* str, size_t len)
{
	return NET_CALL(Net::NetEtherNtostr(n, str, len));
}

int KYTY_SYSV_ABI NetGetMacAddress(Net::NetEtherAddr* addr, int flags)
{
	return NET_CALL(Net::NetGetMacAddress(addr, flags));
}

LIB_DEFINE(InitNet_1_Net)
{
	LIB_FUNC("Nlev7Lg8k3A", LibNet::NetInit);
	LIB_FUNC("dgJBaeJnGpo", LibNet::NetPoolCreate);
	LIB_FUNC("8Kcp5d-q1Uo", LibNet::NetInetPton);
	LIB_FUNC("v6M4txecCuo", LibNet::NetEtherNtostr);
	LIB_FUNC("6Oc0bLsIYe0", LibNet::NetGetMacAddress);
}

} // namespace LibNet

namespace LibSsl {

LIB_VERSION("Ssl", 1, "Ssl", 1, 1);

namespace Ssl = Network::Ssl;

LIB_DEFINE(InitNet_1_Ssl)
{
	LIB_FUNC("hdpVEUDFW3s", Ssl::SslInit);
}

} // namespace LibSsl

namespace LibHttp {

LIB_VERSION("Http", 1, "Http", 1, 1);

namespace Http = Network::Http;

LIB_DEFINE(InitNet_1_Http)
{
	LIB_FUNC("A9cVMUtEp4Y", Http::HttpInit);
	LIB_FUNC("0gYjPTR-6cY", Http::HttpCreateTemplate);
}

} // namespace LibHttp

namespace LibNetCtl {

LIB_VERSION("NetCtl", 1, "NetCtl", 1, 1);

namespace NetCtl = Network::NetCtl;

LIB_DEFINE(InitNet_1_NetCtl)
{
	LIB_FUNC("gky0+oaNM4k", NetCtl::NetCtlInit);
	LIB_FUNC("JO4yuTuMoKI", NetCtl::NetCtlGetNatInfo);
	LIB_FUNC("iQw3iQPhvUQ", NetCtl::NetCtlCheckCallback);
	LIB_FUNC("uBPlr0lbuiI", NetCtl::NetCtlGetState);
	LIB_FUNC("UJ+Z7Q+4ck0", NetCtl::NetCtlRegisterCallback);
	LIB_FUNC("obuxdTiwkF8", NetCtl::NetCtlGetInfo);
}

} // namespace LibNetCtl

namespace LibNpManager {

LIB_VERSION("NpManager", 1, "NpManager", 1, 1);

namespace NpManager = Network::NpManager;

LIB_DEFINE(InitNet_1_NpManager)
{
	LIB_FUNC("3Zl8BePTh9Y", NpManager::NpCheckCallback);
	LIB_FUNC("Ec63y59l9tw", NpManager::NpSetNpTitleId);
	LIB_FUNC("A2CQ3kgSopQ", NpManager::NpSetContentRestriction);
	LIB_FUNC("VfRSmPmj8Q8", NpManager::NpRegisterStateCallback);
	LIB_FUNC("uFJpaKNBAj4", NpManager::NpRegisterGamePresenceCallback);
	LIB_FUNC("GImICnh+boA", NpManager::NpRegisterPlusEventCallback);
}

} // namespace LibNpManager

namespace LibNpManagerForToolkit {

LIB_VERSION("NpManagerForToolkit", 1, "NpManager", 1, 1);

namespace NpManagerForToolkit = Network::NpManagerForToolkit;

LIB_DEFINE(InitNet_1_NpManagerForToolkit)
{
	LIB_FUNC("0c7HbXRKUt4", NpManagerForToolkit::NpRegisterStateCallbackForToolkit);
}

} // namespace LibNpManagerForToolkit

namespace LibNpTrophy {

LIB_VERSION("NpTrophy", 1, "NpTrophy", 1, 1);

namespace NpTrophy = Network::NpTrophy;

LIB_DEFINE(InitNet_1_NpTrophy)
{
	LIB_FUNC("q7U6tEAQf7c", NpTrophy::NpTrophyCreateHandle);
}

} // namespace LibNpTrophy

namespace LibNpWebApi {

LIB_VERSION("NpWebApi", 1, "NpWebApi", 1, 1);

namespace NpWebApi = Network::NpWebApi;

LIB_DEFINE(InitNet_1_NpWebApi)
{
	LIB_FUNC("G3AnLNdRBjE", NpWebApi::NpWebApiInitialize);
}

} // namespace LibNpWebApi

LIB_DEFINE(InitNet_1)
{
	LibNet::InitNet_1_Net(s);
	LibSsl::InitNet_1_Ssl(s);
	LibHttp::InitNet_1_Http(s);
	LibNetCtl::InitNet_1_NetCtl(s);
	LibNpManager::InitNet_1_NpManager(s);
	LibNpManagerForToolkit::InitNet_1_NpManagerForToolkit(s);
	LibNpTrophy::InitNet_1_NpTrophy(s);
	LibNpWebApi::InitNet_1_NpWebApi(s);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
