#include "Emulator/Network.h"

#include "Kyty/Core/ByteBuffer.h"
#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Network {

class Network
{
public:
	class Id
	{
	public:
		explicit Id(int id): m_id(id - 1) {}
		[[nodiscard]] int  ToInt() const { return m_id + 1; }
		[[nodiscard]] bool IsValid() const { return m_id >= 0; }

		friend class Network;

	private:
		Id() = default;
		static Id Invalid() { return Id(); }
		static Id Create(int net_id)
		{
			Id r;
			r.m_id = net_id;
			return r;
		}
		[[nodiscard]] int GetId() const { return m_id; }

		int m_id = -1;
	};

	Network()          = default;
	virtual ~Network() = default;

	KYTY_CLASS_NO_COPY(Network);

	int  PoolCreate(const char* name, int size);
	bool PoolDestroy(int memid);

	Id   SslInit(uint64_t pool_size);
	bool SslTerm(Id ssl_ctx_id);

	Id   HttpInit(int memid, Id ssl_ctx_id, uint64_t pool_size);
	bool HttpTerm(Id http_ctx_id);
	Id   HttpCreateTemplate(Id http_ctx_id, const char* user_agent, int http_ver, bool is_auto_proxy_conf);
	bool HttpDeleteTemplate(Id tmpl_id);
	bool HttpValid(Id http_ctx_id);

private:
	struct Pool
	{
		bool   used = false;
		String name;
		int    size = 0;
	};

	struct Ssl
	{
		bool     used = false;
		uint64_t size = 0;
	};

	struct Http
	{
		bool     used       = false;
		uint64_t size       = 0;
		int      memid      = 0;
		int      ssl_ctx_id = 0;
	};

	struct HttpTemplate
	{
		bool   used        = false;
		int    http_ctx_id = 0;
		String user_agent;
		int    http_ver           = 0;
		bool   is_auto_proxy_conf = true;
	};

	static constexpr int POOLS_MAX = 32;
	static constexpr int SSL_MAX   = 32;
	static constexpr int HTTP_MAX  = 32;

	Core::Mutex          m_mutex;
	Pool                 m_pools[POOLS_MAX];
	Ssl                  m_ssl[SSL_MAX];
	Http                 m_http[HTTP_MAX];
	Vector<HttpTemplate> m_templates;
};

static Network* g_net = nullptr;

KYTY_SUBSYSTEM_INIT(Network)
{
	EXIT_IF(g_net != nullptr);

	g_net = new Network;
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Network) {}

KYTY_SUBSYSTEM_DESTROY(Network) {}

int Network::PoolCreate(const char* name, int size)
{
	Core::LockGuard lock(m_mutex);

	for (int id = 0; id < POOLS_MAX; id++)
	{
		if (!m_pools[id].used)
		{
			m_pools[id].used = true;
			m_pools[id].size = size;
			m_pools[id].name = String::FromUtf8(name);

			return id;
		}
	}

	return -1;
}

bool Network::PoolDestroy(int memid)
{
	Core::LockGuard lock(m_mutex);

	if (memid >= 0 && memid < POOLS_MAX && m_pools[memid].used)
	{
		m_pools[memid].used = false;

		return true;
	}

	return false;
}

Network::Id Network::SslInit(uint64_t pool_size)
{
	Core::LockGuard lock(m_mutex);

	for (int id = 0; id < SSL_MAX; id++)
	{
		if (!m_ssl[id].used)
		{
			m_ssl[id].used = true;
			m_ssl[id].size = pool_size;

			return Id::Create(id);
		}
	}

	return Id::Invalid();
}

bool Network::SslTerm(Id ssl_ctx_id)
{
	Core::LockGuard lock(m_mutex);

	if (ssl_ctx_id.GetId() >= 0 && ssl_ctx_id.GetId() < SSL_MAX && m_ssl[ssl_ctx_id.GetId()].used)
	{
		m_ssl[ssl_ctx_id.GetId()].used = false;

		return true;
	}

	return false;
}

Network::Id Network::HttpInit(int memid, Id ssl_ctx_id, uint64_t pool_size)
{
	Core::LockGuard lock(m_mutex);

	if (ssl_ctx_id.GetId() >= 0 && ssl_ctx_id.GetId() < SSL_MAX && m_ssl[ssl_ctx_id.GetId()].used && memid >= 0 && memid < POOLS_MAX &&
	    m_pools[memid].used)
	{
		for (int id = 0; id < HTTP_MAX; id++)
		{
			if (!m_http[id].used)
			{
				m_http[id].used       = true;
				m_http[id].size       = pool_size;
				m_http[id].ssl_ctx_id = ssl_ctx_id.GetId();
				m_http[id].memid      = memid;

				return Id::Create(id);
			}
		}
	}

	return Id::Invalid();
}

bool Network::HttpValid(Id http_ctx_id)
{
	Core::LockGuard lock(m_mutex);

	return (http_ctx_id.GetId() >= 0 && http_ctx_id.GetId() < HTTP_MAX && m_http[http_ctx_id.GetId()].used);
}

bool Network::HttpTerm(Id http_ctx_id)
{
	Core::LockGuard lock(m_mutex);

	if (HttpValid(http_ctx_id))
	{
		m_http[http_ctx_id.GetId()].used = false;

		return true;
	}

	return false;
}

Network::Id Network::HttpCreateTemplate(Id http_ctx_id, const char* user_agent, int http_ver, bool is_auto_proxy_conf)
{
	Core::LockGuard lock(m_mutex);

	if (http_ctx_id.GetId() >= 0 && http_ctx_id.GetId() < HTTP_MAX && m_http[http_ctx_id.GetId()].used)
	{
		HttpTemplate tn {};
		tn.used               = true;
		tn.http_ver           = http_ver;
		tn.user_agent         = String::FromUtf8(user_agent);
		tn.is_auto_proxy_conf = is_auto_proxy_conf;
		tn.http_ctx_id        = http_ctx_id.GetId();

		int index = 0;
		for (auto& t: m_templates)
		{
			if (!t.used)
			{
				t = tn;
				return Id::Create(index);
			}
			index++;
		}

		m_templates.Add(tn);

		return Id::Create(index);
	}

	return Id::Invalid();
}

bool Network::HttpDeleteTemplate(Id tmpl_id)
{
	Core::LockGuard lock(m_mutex);

	if (m_templates.IndexValid(tmpl_id.GetId()) && m_templates.At(tmpl_id.GetId()).used)
	{
		m_templates[tmpl_id.GetId()].used = false;

		return true;
	}

	return false;
}

namespace Net {

LIB_NAME("Net", "Net");

struct NetEtherAddr
{
	uint8_t data[6] = {0};
};

int KYTY_SYSV_ABI NetInit()
{
	PRINT_NAME();

	return OK;
}

int KYTY_SYSV_ABI NetTerm()
{
	PRINT_NAME();

	return OK;
}

int KYTY_SYSV_ABI NetPoolCreate(const char* name, int size, int flags)
{
	PRINT_NAME();

	printf("\t name = %s\n", name);
	printf("\t size = %d\n", size);
	printf("\t flags = %d\n", flags);

	EXIT_IF(g_net == nullptr);

	EXIT_NOT_IMPLEMENTED(flags != 0);
	EXIT_NOT_IMPLEMENTED(size == 0);

	int id = g_net->PoolCreate(name, size);

	if (id < 0)
	{
		return NET_ERROR_ENFILE;
	}

	return id;
}

int KYTY_SYSV_ABI NetPoolDestroy(int memid)
{
	PRINT_NAME();

	EXIT_IF(g_net == nullptr);

	if (!g_net->PoolDestroy(memid))
	{
		return NET_ERROR_EBADF;
	}

	return OK;
}

int KYTY_SYSV_ABI NetInetPton(int af, const char* src, void* dst)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(af != 2);
	EXIT_NOT_IMPLEMENTED(src == nullptr);
	EXIT_NOT_IMPLEMENTED(dst == nullptr);
	EXIT_NOT_IMPLEMENTED(strcmp(src, "127.0.0.1") != 0);

	printf("\t src = %.16s\n", src);

	*static_cast<uint32_t*>(dst) = 0x7f000001;

	return OK;
}

int KYTY_SYSV_ABI NetEtherNtostr(const NetEtherAddr* n, char* str, size_t len)
{
	PRINT_NAME();

	NetEtherAddr zero {};

	EXIT_NOT_IMPLEMENTED(len != 18);
	EXIT_NOT_IMPLEMENTED(n == nullptr);
	EXIT_NOT_IMPLEMENTED(str == nullptr);
	EXIT_NOT_IMPLEMENTED(memcmp(n->data, zero.data, sizeof(zero.data)) != 0);

	strcpy(str, "00:00:00:00:00:00"); // NOLINT

	return OK;
}

int KYTY_SYSV_ABI NetGetMacAddress(NetEtherAddr* addr, int flags)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(addr == nullptr);
	EXIT_NOT_IMPLEMENTED(flags != 0);

	memset(addr->data, 0, sizeof(addr->data));

	return OK;
}

} // namespace Net

namespace Ssl {

LIB_NAME("Ssl", "Ssl");

int KYTY_SYSV_ABI SslInit(uint64_t pool_size)
{
	PRINT_NAME();

	printf("\t size = %" PRIu64 "\n", pool_size);

	EXIT_IF(g_net == nullptr);

	EXIT_NOT_IMPLEMENTED(pool_size == 0);

	auto id = g_net->SslInit(pool_size);

	if (!id.IsValid())
	{
		return SSL_ERROR_OUT_OF_SIZE;
	}

	return id.ToInt();
}

int KYTY_SYSV_ABI SslTerm(int ssl_ctx_id)
{
	PRINT_NAME();

	EXIT_IF(g_net == nullptr);

	if (!g_net->SslTerm(Network::Id(ssl_ctx_id)))
	{
		return SSL_ERROR_INVALID_ID;
	}

	return OK;
}

} // namespace Ssl

namespace Http {

LIB_NAME("Http", "Http");

int KYTY_SYSV_ABI HttpInit(int memid, int ssl_ctx_id, uint64_t pool_size)
{
	PRINT_NAME();

	printf("\t memid      = %d\n", memid);
	printf("\t ssl_ctx_id = %d\n", ssl_ctx_id);
	printf("\t size       = %" PRIu64 "\n", pool_size);

	EXIT_IF(g_net == nullptr);

	EXIT_NOT_IMPLEMENTED(pool_size == 0);

	auto id = g_net->HttpInit(memid, Network::Id(ssl_ctx_id), pool_size);

	if (!id.IsValid())
	{
		return HTTP_ERROR_OUT_OF_MEMORY;
	}

	return id.ToInt();
}

int KYTY_SYSV_ABI HttpTerm(int http_ctx_id)
{
	PRINT_NAME();

	EXIT_IF(g_net == nullptr);

	if (!g_net->HttpTerm(Network::Id(http_ctx_id)))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpCreateTemplate(int http_ctx_id, const char* user_agent, int http_ver, int is_auto_proxy_conf)
{
	PRINT_NAME();

	printf("\t http_ctx_id        = %d\n", http_ctx_id);
	printf("\t user_agent         = %s\n", user_agent);
	printf("\t http_ver           = %d\n", http_ver);
	printf("\t is_auto_proxy_conf = %d\n", is_auto_proxy_conf);

	EXIT_IF(g_net == nullptr);

	auto id = g_net->HttpCreateTemplate(Network::Id(http_ctx_id), user_agent, http_ver, is_auto_proxy_conf != 0);

	if (!id.IsValid())
	{
		return HTTP_ERROR_OUT_OF_MEMORY;
	}

	return id.ToInt();
}

int KYTY_SYSV_ABI HttpDeleteTemplate(int tmpl_id)
{
	PRINT_NAME();

	EXIT_IF(g_net == nullptr);

	if (!g_net->HttpDeleteTemplate(Network::Id(tmpl_id)))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

} // namespace Http

namespace NetCtl {

LIB_NAME("NetCtl", "NetCtl");

struct NetInAddr
{
	uint32_t s_addr = 0;
};

struct NetEtherAddr
{
	uint8_t data[6];
};

struct NetCtlNatInfo
{
	unsigned int size       = sizeof(NetCtlNatInfo);
	int          stunStatus = 0;
	int          natType    = 0;
	NetInAddr    mappedAddr;
};

union NetCtlInfo
{
	uint32_t     device;
	NetEtherAddr ether_addr;
	uint32_t     mtu;
	uint32_t     link;
	NetEtherAddr bssid;
	char         ssid[32 + 1];
	uint32_t     wifi_security;
	uint8_t      rssi_dbm;
	uint8_t      rssi_percentage;
	uint8_t      channel;
	uint32_t     ip_config;
	char         dhcp_hostname[255 + 1];
	char         pppoe_auth_name[127 + 1];
	char         ip_address[16];
	char         netmask[16];
	char         default_route[16];
	char         primary_dns[16];
	char         secondary_dns[16];
	uint32_t     http_proxy_config;
	char         http_proxy_server[255 + 1];
	uint16_t     http_proxy_port;
};

int KYTY_SYSV_ABI NetCtlInit()
{
	PRINT_NAME();

	return OK;
}

void KYTY_SYSV_ABI NetCtlTerm()
{
	PRINT_NAME();
}

int KYTY_SYSV_ABI NetCtlGetNatInfo(NetCtlNatInfo* nat_info)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(nat_info == nullptr);
	EXIT_NOT_IMPLEMENTED(nat_info->size != sizeof(NetCtlNatInfo));

	nat_info->stunStatus        = 1;
	nat_info->natType           = 3;
	nat_info->mappedAddr.s_addr = 0x7f000001;

	return OK;
}

int KYTY_SYSV_ABI NetCtlCheckCallback()
{
	PRINT_NAME();

	return OK;
}

int KYTY_SYSV_ABI NetCtlGetState(int* state)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(state == nullptr);

	*state = 0; // Disconnected

	return OK;
}

int KYTY_SYSV_ABI NetCtlRegisterCallback(NetCtlCallback func, void* /*arg*/, int* cid)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(func == nullptr);
	EXIT_NOT_IMPLEMENTED(cid == nullptr);

	*cid = 1;

	return OK;
}

int KYTY_SYSV_ABI NetCtlGetInfo(int code, NetCtlInfo* info)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(info == nullptr);

	printf("\t code = %d\n", code);

	switch (code)
	{
		case 2: memset(info->ether_addr.data, 0, sizeof(info->ether_addr.data)); break;
		case 11: info->ip_config = 0; break;
		case 14: strcpy(info->ip_address, "127.0.0.1"); break;
		default: EXIT("unknown code: %d\n", code);
	}

	return OK;
}

} // namespace NetCtl

namespace NpManager {

LIB_NAME("NpManager", "NpManager");

struct NpTitleId
{
	char    id[12 + 1];
	uint8_t padding[3];
};

struct NpTitleSecret
{
	uint8_t data[128];
};

struct NpCountryCode
{
	char data[2];
	char term;
	char padding[1];
};

struct NpAgeRestriction
{
	NpCountryCode country_code;
	int8_t        age;
	uint8_t       padding[3];
};

struct NpContentRestriction
{
	size_t                  size;
	int8_t                  default_age_restriction;
	char                    padding[3];
	int32_t                 age_restriction_count;
	const NpAgeRestriction* age_restriction;
};

int KYTY_SYSV_ABI NpCheckCallback()
{
	PRINT_NAME();

	return OK;
}

int KYTY_SYSV_ABI NpSetNpTitleId(const NpTitleId* title_id, const NpTitleSecret* title_secret)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(title_id == nullptr);
	EXIT_NOT_IMPLEMENTED(title_secret == nullptr);

	printf("\t title_id = %.12s\n", title_id->id);
	printf("\t title_secret = %s\n", String::HexFromBin(Core::ByteBuffer(title_secret->data, 128)).C_Str());

	return OK;
}

int KYTY_SYSV_ABI NpSetContentRestriction(const NpContentRestriction* restriction)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(restriction == nullptr);
	EXIT_NOT_IMPLEMENTED(restriction->size != sizeof(NpContentRestriction));

	printf("\t default_age_restriction = %" PRIi8 "\n", restriction->default_age_restriction);
	printf("\t age_restriction_count   = %" PRIi32 "\n", restriction->age_restriction_count);

	for (int i = 0; i < restriction->age_restriction_count; i++)
	{
		printf("\t age_restriction[%d].age = %" PRIi8 "\n", i, restriction->age_restriction[i].age);
		printf("\t age_restriction[%d].country_code.data = %.2s\n", i, restriction->age_restriction[i].country_code.data);
	}

	return OK;
}

int KYTY_SYSV_ABI NpRegisterStateCallback(void* /*callback*/, void* /*userdata*/)
{
	PRINT_NAME();

	return OK;
}

void KYTY_SYSV_ABI NpRegisterGamePresenceCallback(void* /*callback*/, void* /*userdata*/)
{
	PRINT_NAME();
}

int KYTY_SYSV_ABI NpRegisterPlusEventCallback(void* /*callback*/, void* /*userdata*/)
{
	PRINT_NAME();

	return OK;
}

} // namespace NpManager

namespace NpManagerForToolkit {

LIB_NAME("NpManagerForToolkit", "NpManager");

int KYTY_SYSV_ABI NpRegisterStateCallbackForToolkit(void* /*callback*/, void* /*userdata*/)
{
	PRINT_NAME();

	return OK;
}

} // namespace NpManagerForToolkit

namespace NpTrophy {

LIB_NAME("NpTrophy", "NpTrophy");

int KYTY_SYSV_ABI NpTrophyCreateHandle(int* handle)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(handle == nullptr);

	*handle = 1;

	return OK;
}

} // namespace NpTrophy

namespace NpWebApi {

LIB_NAME("NpWebApi", "NpWebApi");

int KYTY_SYSV_ABI NpWebApiInitialize(int http_ctx_id, size_t pool_size)
{
	PRINT_NAME();

	EXIT_IF(g_net == nullptr);

	printf("\t http_ctx_id = %d\n", http_ctx_id);
	printf("\t pool_size   = %" PRIu64 "\n", pool_size);

	EXIT_NOT_IMPLEMENTED(!g_net->HttpValid(Network::Id(http_ctx_id)));

	static int id = 0;

	return ++id;
}

int KYTY_SYSV_ABI NpWebApiTerminate(int lib_ctx_id)
{
	PRINT_NAME();

	printf("\t lib_ctx_id = %d\n", lib_ctx_id);

	return OK;
}

} // namespace NpWebApi

} // namespace Kyty::Libs::Network

#endif // KYTY_EMU_ENABLED
