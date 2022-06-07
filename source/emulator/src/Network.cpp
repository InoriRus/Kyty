#include "Emulator/Network.h"

#include "Kyty/Core/ByteBuffer.h"
#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Kernel/Pthread.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#include <atomic>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Network {

class Network
{
public:
	class Id
	{
	public:
		static constexpr int MAX_ID = 65536;

		enum class Type : uint32_t
		{
			Invalid    = 0,
			Http       = 1,
			Ssl        = 2,
			Template   = 3,
			Connection = 4,
			Request    = 5,
		};

		explicit Id(int id): m_id(static_cast<uint32_t>(id) & 0xffffu), m_type(static_cast<uint32_t>(id) >> 16u) {}
		[[nodiscard]] int  ToInt() const { return static_cast<int>(m_id + (static_cast<uint32_t>(m_type) << 16u)); }
		[[nodiscard]] bool IsValid() const { return GetType() != Type::Invalid; }
		[[nodiscard]] Type GetType() const
		{
			switch (m_type)
			{
				case static_cast<uint32_t>(Type::Http): return Type::Http; break;
				case static_cast<uint32_t>(Type::Ssl): return Type::Ssl; break;
				case static_cast<uint32_t>(Type::Template): return Type::Template; break;
				case static_cast<uint32_t>(Type::Connection): return Type::Connection; break;
				case static_cast<uint32_t>(Type::Request): return Type::Request; break;
				default: return Type::Invalid;
			}
		}

		friend class Network;

	private:
		Id() = default;
		static Id Invalid() { return {}; }
		static Id Create(int net_id, Type type)
		{
			Id r;
			r.m_id   = net_id;
			r.m_type = static_cast<uint32_t>(type);
			return r;
		}
		[[nodiscard]] int GetId() const { return static_cast<int>(m_id); }

		uint32_t m_id   = 0;
		uint32_t m_type = static_cast<uint32_t>(Type::Invalid);
	};

	using HttpsCallback = KYTY_SYSV_ABI int (*)(int, unsigned int, void* const*, int, void*);

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
	bool HttpSetNonblock(Id id, bool enable);
	bool HttpsSetSslCallback(Id id, HttpsCallback cbfunc, void* user_arg);
	bool HttpsDisableOption(Id id, uint32_t ssl_flags);
	bool HttpAddRequestHeader(Id id, const char* name, const char* value, bool add);
	bool HttpValid(Id http_ctx_id);
	bool HttpValidTemplate(Id tmpl_id);
	bool HttpValidConnection(Id conn_id);
	bool HttpValidRequest(Id req_id);
	Id   HttpCreateConnectionWithURL(Id tmpl_id, const char* url, bool enable_keep_alive);
	bool HttpDeleteConnection(Id conn_id);
	Id   HttpCreateRequestWithURL2(Id conn_id, const char* method, const char* url, uint64_t content_length);
	bool HttpDeleteRequest(Id req_id);
	bool HttpSetResolveTimeOut(Id id, uint32_t usec);
	bool HttpSetResolveRetry(Id id, int32_t retry);
	bool HttpSetConnectTimeOut(Id id, uint32_t usec);
	bool HttpSetSendTimeOut(Id id, uint32_t usec);
	bool HttpSetRecvTimeOut(Id id, uint32_t usec);
	bool HttpSetAutoRedirect(Id id, int enable);
	bool HttpSetAuthEnabled(Id id, int enable);

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

	struct HttpHeader
	{
		String name;
		String value;
	};

	struct HttpBase
	{
		Vector<HttpHeader> headers;
		bool               used            = false;
		bool               nonblock        = false;
		bool               auto_redirect   = true;
		bool               auth_enabled    = true;
		HttpsCallback      ssl_cbfunc      = nullptr;
		void*              ssl_user_arg    = nullptr;
		uint32_t           ssl_flags       = 0xA7;
		int                http_ctx_id     = 0;
		uint32_t           resolve_timeout = 1'000000;
		int32_t            resolve_retry   = 4;
		uint32_t           connect_timeout = 30'000000;
		uint32_t           send_timeout    = 120'000000;
		uint32_t           recv_timeout    = 120'000000;
	};

	struct HttpTemplate: public HttpBase
	{
		String user_agent;
		int    http_ver           = 0;
		bool   is_auto_proxy_conf = true;
	};

	struct HttpConnection: public HttpTemplate
	{
		explicit HttpConnection(const HttpTemplate& tmpl): HttpTemplate(tmpl) {}
		// int    tmpl_id = 0;
		String url;
		bool   enable_keep_alive = false;
	};

	struct HttpRequest: public HttpConnection
	{
		explicit HttpRequest(HttpConnection& conn): HttpConnection(conn) {}
		// int      conn_id = 0;
		String   method;
		String   url;
		uint64_t content_length = 0;
	};

	static constexpr int POOLS_MAX = 32;
	static constexpr int SSL_MAX   = 32;
	static constexpr int HTTP_MAX  = 32;

	Core::Mutex            m_mutex;
	Pool                   m_pools[POOLS_MAX];
	Ssl                    m_ssl[SSL_MAX];
	Http                   m_http[HTTP_MAX];
	Vector<HttpTemplate>   m_templates;
	Vector<HttpConnection> m_connections;
	Vector<HttpRequest>    m_requests;
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

			return Id::Create(id, Id::Type::Ssl);
		}
	}

	return Id::Invalid();
}

bool Network::SslTerm(Id ssl_ctx_id)
{
	Core::LockGuard lock(m_mutex);

	if (ssl_ctx_id.GetType() == Id::Type::Ssl && ssl_ctx_id.GetId() >= 0 && ssl_ctx_id.GetId() < SSL_MAX && m_ssl[ssl_ctx_id.GetId()].used)
	{
		m_ssl[ssl_ctx_id.GetId()].used = false;

		return true;
	}

	return false;
}

Network::Id Network::HttpInit(int memid, Id ssl_ctx_id, uint64_t pool_size)
{
	Core::LockGuard lock(m_mutex);

	if (ssl_ctx_id.GetType() == Id::Type::Ssl && ssl_ctx_id.GetId() >= 0 && ssl_ctx_id.GetId() < SSL_MAX &&
	    m_ssl[ssl_ctx_id.GetId()].used && memid >= 0 && memid < POOLS_MAX && m_pools[memid].used)
	{
		for (int id = 0; id < HTTP_MAX; id++)
		{
			if (!m_http[id].used)
			{
				m_http[id].used       = true;
				m_http[id].size       = pool_size;
				m_http[id].ssl_ctx_id = ssl_ctx_id.GetId();
				m_http[id].memid      = memid;

				return Id::Create(id, Id::Type::Http);
			}
		}
	}

	return Id::Invalid();
}

bool Network::HttpValid(Id http_ctx_id)
{
	Core::LockGuard lock(m_mutex);

	return (http_ctx_id.GetType() == Id::Type::Http && http_ctx_id.GetId() >= 0 && http_ctx_id.GetId() < HTTP_MAX &&
	        m_http[http_ctx_id.GetId()].used);
}

bool Network::HttpValidTemplate(Id tmpl_id)
{
	Core::LockGuard lock(m_mutex);

	return (tmpl_id.GetType() == Id::Type::Template && m_templates.IndexValid(tmpl_id.GetId()) && m_templates.At(tmpl_id.GetId()).used);
}

bool Network::HttpValidConnection(Id conn_id)
{
	Core::LockGuard lock(m_mutex);

	return (conn_id.GetType() == Id::Type::Connection && m_connections.IndexValid(conn_id.GetId()) &&
	        m_connections.At(conn_id.GetId()).used);
}

bool Network::HttpValidRequest(Id req_id)
{
	Core::LockGuard lock(m_mutex);

	return (req_id.GetType() == Id::Type::Request && m_requests.IndexValid(req_id.GetId()) && m_requests.At(req_id.GetId()).used);
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

	if (HttpValid(http_ctx_id))
	{
		HttpTemplate tn {};
		tn.used               = true;
		tn.http_ver           = http_ver;
		tn.user_agent         = String::FromUtf8(user_agent);
		tn.is_auto_proxy_conf = is_auto_proxy_conf;
		tn.http_ctx_id        = http_ctx_id.GetId();
		tn.nonblock           = false;

		int index = 0;
		for (auto& t: m_templates)
		{
			if (!t.used)
			{
				t = tn;
				return Id::Create(index, Id::Type::Template);
			}
			index++;
		}

		if (index < Id::MAX_ID)
		{
			m_templates.Add(tn);
			return Id::Create(index, Id::Type::Template);
		}
	}

	return Id::Invalid();
}

Network::Id Network::HttpCreateConnectionWithURL(Id tmpl_id, const char* url, bool enable_keep_alive)
{
	Core::LockGuard lock(m_mutex);

	if (HttpValidTemplate(tmpl_id))
	{
		HttpConnection cn(m_templates[tmpl_id.GetId()]);
		cn.used              = true;
		cn.enable_keep_alive = enable_keep_alive;
		cn.url               = String::FromUtf8(url);
		// cn.tmpl_id           = tmpl_id.ToInt();

		int index = 0;
		for (auto& t: m_connections)
		{
			if (!t.used)
			{
				t = cn;
				return Id::Create(index, Id::Type::Connection);
			}
			index++;
		}

		if (index < Id::MAX_ID)
		{
			m_connections.Add(cn);
			return Id::Create(index, Id::Type::Connection);
		}
	}

	return Id::Invalid();
}

bool Network::HttpDeleteConnection(Id conn_id)
{
	Core::LockGuard lock(m_mutex);

	if (HttpValidConnection(conn_id))
	{
		m_connections[conn_id.GetId()].used = false;

		return true;
	}

	return false;
}

Network::Id Network::HttpCreateRequestWithURL2(Id conn_id, const char* method, const char* url, uint64_t content_length)
{
	Core::LockGuard lock(m_mutex);

	if (HttpValidConnection(conn_id))
	{
		HttpRequest cn(m_connections[conn_id.GetId()]);
		cn.used   = true;
		cn.method = String::FromUtf8(method);
		cn.url    = String::FromUtf8(url);
		// cn.conn_id        = conn_id.ToInt();
		cn.content_length = content_length;

		int index = 0;
		for (auto& t: m_requests)
		{
			if (!t.used)
			{
				t = cn;
				return Id::Create(index, Id::Type::Request);
			}
			index++;
		}

		if (index < Id::MAX_ID)
		{
			m_requests.Add(cn);
			return Id::Create(index, Id::Type::Request);
		}
	}

	return Id::Invalid();
}

bool Network::HttpDeleteRequest(Id req_id)
{
	Core::LockGuard lock(m_mutex);

	if (HttpValidRequest(req_id))
	{
		m_requests[req_id.GetId()].used = false;

		return true;
	}

	return false;
}

bool Network::HttpDeleteTemplate(Id tmpl_id)
{
	Core::LockGuard lock(m_mutex);

	if (HttpValidTemplate(tmpl_id))
	{
		m_templates[tmpl_id.GetId()].used = false;

		return true;
	}

	return false;
}

bool Network::HttpSetNonblock(Id id, bool enable)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	} else if (HttpValidRequest(id))
	{
		base = &m_requests[id.GetId()];
	}

	if (base != nullptr)
	{
		base->nonblock = enable;
		return true;
	}

	return false;
}

bool Network::HttpsSetSslCallback(Id id, HttpsCallback cbfunc, void* user_arg)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	} else if (HttpValidRequest(id))
	{
		base = &m_requests[id.GetId()];
	}

	if (base != nullptr)
	{
		base->ssl_cbfunc   = cbfunc;
		base->ssl_user_arg = user_arg;
		return true;
	}

	return false;
}

bool Network::HttpsDisableOption(Id id, uint32_t ssl_flags)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	} else if (HttpValidRequest(id))
	{
		base = &m_requests[id.GetId()];
	}

	if (base != nullptr)
	{
		base->ssl_flags &= ~ssl_flags;
		return true;
	}

	return false;
}

bool Network::HttpAddRequestHeader(Id id, const char* name, const char* value, bool add)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	} else if (HttpValidRequest(id))
	{
		base = &m_requests[id.GetId()];
	}

	if (base != nullptr)
	{
		HttpHeader nh({String::FromUtf8(name), String::FromUtf8(value)});
		if (add)
		{
			base->headers.Add(nh);
		} else
		{
			for (auto& h: base->headers)
			{
				if (h.name == nh.name)
				{
					h.value = nh.value;
				}
			}
		}
		return true;
	}

	return false;
}

bool Network::HttpSetResolveTimeOut(Id id, uint32_t usec)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	}

	if (base != nullptr)
	{
		base->resolve_timeout = usec;
		return true;
	}

	return false;
}

bool Network::HttpSetResolveRetry(Id id, int32_t retry)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	}

	if (base != nullptr)
	{
		base->resolve_retry = retry;
		return true;
	}

	return false;
}

bool Network::HttpSetConnectTimeOut(Id id, uint32_t usec)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	} else if (HttpValidRequest(id))
	{
		base = &m_requests[id.GetId()];
	}

	if (base != nullptr)
	{
		base->connect_timeout = usec;
		return true;
	}

	return false;
}

bool Network::HttpSetSendTimeOut(Id id, uint32_t usec)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	} else if (HttpValidRequest(id))
	{
		base = &m_requests[id.GetId()];
	}

	if (base != nullptr)
	{
		base->send_timeout = usec;
		return true;
	}

	return false;
}

bool Network::HttpSetRecvTimeOut(Id id, uint32_t usec)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	} else if (HttpValidRequest(id))
	{
		base = &m_requests[id.GetId()];
	}

	if (base != nullptr)
	{
		base->recv_timeout = usec;
		return true;
	}

	return false;
}

bool Network::HttpSetAutoRedirect(Id id, int enable)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	} else if (HttpValidRequest(id))
	{
		base = &m_requests[id.GetId()];
	}

	if (base != nullptr)
	{
		base->auto_redirect = (enable != 0);
		return true;
	}

	return false;
}

bool Network::HttpSetAuthEnabled(Id id, int enable)
{
	Core::LockGuard lock(m_mutex);

	HttpBase* base = nullptr;

	if (HttpValidTemplate(id))
	{
		base = &m_templates[id.GetId()];
	} else if (HttpValidConnection(id))
	{
		base = &m_connections[id.GetId()];
	} else if (HttpValidRequest(id))
	{
		base = &m_requests[id.GetId()];
	}

	if (base != nullptr)
	{
		base->auth_enabled = (enable != 0);
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

struct HttpEpoll
{
	Network::Id http_ctx_id = Network::Id(0);
	Network::Id request_id  = Network::Id(0);
	void*       user_arg    = nullptr;
};

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

int KYTY_SYSV_ABI HttpSetNonblock(int id, int enable)
{
	PRINT_NAME();

	printf("\t id     = %d\n", id);
	printf("\t enable = %d\n", enable);

	if (!g_net->HttpSetNonblock(Network::Id(id), enable != 0))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpsSetSslCallback(int id, HttpsCallback cbfunc, void* user_arg)
{
	PRINT_NAME();

	printf("\t id     = %d\n", id);

	if (!g_net->HttpsSetSslCallback(Network::Id(id), cbfunc, user_arg))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpsDisableOption(int id, uint32_t ssl_flags)
{
	PRINT_NAME();

	printf("\t id        = %d\n", id);
	printf("\t ssl_flags = %u\n", ssl_flags);

	if (!g_net->HttpsDisableOption(Network::Id(id), ssl_flags))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpSetResolveTimeOut(int id, uint32_t usec)
{
	PRINT_NAME();

	printf("\t id   = %d\n", id);
	printf("\t usec = %u\n", usec);

	if (!g_net->HttpSetResolveTimeOut(Network::Id(id), usec))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpSetResolveRetry(int id, int32_t retry)
{
	PRINT_NAME();

	printf("\t id    = %d\n", id);
	printf("\t retry = %d\n", retry);

	if (!g_net->HttpSetResolveRetry(Network::Id(id), retry))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpSetConnectTimeOut(int id, uint32_t usec)
{
	PRINT_NAME();

	printf("\t id   = %d\n", id);
	printf("\t usec = %u\n", usec);

	if (!g_net->HttpSetConnectTimeOut(Network::Id(id), usec))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpSetSendTimeOut(int id, uint32_t usec)
{
	PRINT_NAME();

	printf("\t id   = %d\n", id);
	printf("\t usec = %u\n", usec);

	if (!g_net->HttpSetSendTimeOut(Network::Id(id), usec))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpSetRecvTimeOut(int id, uint32_t usec)
{
	PRINT_NAME();

	printf("\t id   = %d\n", id);
	printf("\t usec = %u\n", usec);

	if (!g_net->HttpSetRecvTimeOut(Network::Id(id), usec))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpSetAutoRedirect(int id, int enable)
{
	PRINT_NAME();

	printf("\t id     = %d\n", id);
	printf("\t enable = %d\n", enable);

	if (!g_net->HttpSetAutoRedirect(Network::Id(id), enable))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpSetAuthEnabled(int id, int enable)
{
	PRINT_NAME();

	printf("\t id     = %d\n", id);
	printf("\t enable = %d\n", enable);

	if (!g_net->HttpSetAuthEnabled(Network::Id(id), enable))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpAddRequestHeader(int id, const char* name, const char* value, uint32_t mode)
{
	PRINT_NAME();

	printf("\t id    = %d\n", id);
	printf("\t name  = %s\n", name);
	printf("\t value = %s\n", value);
	printf("\t mode  = %u\n", mode);

	EXIT_NOT_IMPLEMENTED(mode != 0 && mode != 1);

	if (!g_net->HttpAddRequestHeader(Network::Id(id), name, value, mode == 1))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpCreateEpoll(int http_ctx_id, HttpEpollHandle* eh)
{
	PRINT_NAME();

	printf("\t http_ctx_id = %d\n", http_ctx_id);

	EXIT_IF(g_net == nullptr);

	EXIT_NOT_IMPLEMENTED(eh == nullptr);

	EXIT_NOT_IMPLEMENTED(!g_net->HttpValid(Network::Id(http_ctx_id)));

	*eh = new HttpEpoll;

	(*eh)->http_ctx_id = Network::Id(http_ctx_id);

	return OK;
}

int KYTY_SYSV_ABI HttpDestroyEpoll(int http_ctx_id, HttpEpollHandle eh)
{
	PRINT_NAME();

	printf("\t http_ctx_id = %d\n", http_ctx_id);

	EXIT_IF(g_net == nullptr);

	EXIT_NOT_IMPLEMENTED(eh == nullptr);

	EXIT_NOT_IMPLEMENTED(!g_net->HttpValid(Network::Id(http_ctx_id)));

	delete eh;

	return OK;
}

int KYTY_SYSV_ABI HttpSetEpoll(int id, HttpEpollHandle eh, void* user_arg)
{
	PRINT_NAME();

	printf("\t id = %d\n", id);

	EXIT_NOT_IMPLEMENTED(eh == nullptr);

	EXIT_NOT_IMPLEMENTED(!g_net->HttpValidRequest(Network::Id(id)));

	eh->request_id = Network::Id(id);
	eh->user_arg   = user_arg;

	return OK;
}

int KYTY_SYSV_ABI HttpUnsetEpoll(int id)
{
	PRINT_NAME();

	printf("\t id = %d\n", id);

	EXIT_NOT_IMPLEMENTED(!g_net->HttpValidRequest(Network::Id(id)));

	return OK;
}

int KYTY_SYSV_ABI HttpSendRequest(int request_id, const void* /*post_data*/, size_t /*size*/)
{
	PRINT_NAME();

	printf("\t request_id = %d\n", request_id);

	return HTTP_ERROR_TIMEOUT;
}

int KYTY_SYSV_ABI HttpCreateConnectionWithURL(int tmpl_id, const char* url, int enable_keep_alive)
{
	PRINT_NAME();

	printf("\t tmpl_id           = %d\n", tmpl_id);
	printf("\t url               = %s\n", url);
	printf("\t enable_keep_alive = %d\n", enable_keep_alive);

	EXIT_IF(g_net == nullptr);

	auto id = g_net->HttpCreateConnectionWithURL(Network::Id(tmpl_id), url, enable_keep_alive != 0);

	if (!id.IsValid())
	{
		return HTTP_ERROR_OUT_OF_MEMORY;
	}

	return id.ToInt();
}

int KYTY_SYSV_ABI HttpDeleteConnection(int conn_id)
{
	PRINT_NAME();

	printf("\t conn_id = %d\n", conn_id);

	EXIT_IF(g_net == nullptr);

	if (!g_net->HttpDeleteConnection(Network::Id(conn_id)))
	{
		return HTTP_ERROR_INVALID_ID;
	}

	return OK;
}

int KYTY_SYSV_ABI HttpCreateRequestWithURL2(int conn_id, const char* method, const char* url, uint64_t content_length)
{
	PRINT_NAME();

	printf("\t conn_id        = %d\n", conn_id);
	printf("\t url            = %s\n", url);
	printf("\t method         = %s\n", method);
	printf("\t content_length = %" PRIu64 "\n", content_length);

	EXIT_IF(g_net == nullptr);

	auto id = g_net->HttpCreateRequestWithURL2(Network::Id(conn_id), method, url, content_length);

	if (!id.IsValid())
	{
		return HTTP_ERROR_OUT_OF_MEMORY;
	}

	return id.ToInt();
}

int KYTY_SYSV_ABI HttpDeleteRequest(int req_id)
{
	PRINT_NAME();

	printf("\t req_id = %d\n", req_id);

	EXIT_IF(g_net == nullptr);

	if (!g_net->HttpDeleteRequest(Network::Id(req_id)))
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

struct NpOnlineId
{
	char data[16];
	char term;
	char dummy[3];
};

struct NpId
{
	NpOnlineId handle;
	uint8_t    opt[8];
	uint8_t    reserved[8];
};

struct NpCreateAsyncRequestParameter
{
	size_t                   size;
	LibKernel::KernelCpumask cpu_affinity_mask;
	int                      thread_priority;
	uint8_t                  padding[4];
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

int KYTY_SYSV_ABI NpRegisterNpReachabilityStateCallback(void* /*callback*/, void* /*userdata*/)
{
	PRINT_NAME();

	return OK;
}

int KYTY_SYSV_ABI NpGetNpId(int user_id, NpId* np_id)
{
	PRINT_NAME();

	printf("\t user_id = %d\n", user_id);

	EXIT_NOT_IMPLEMENTED(np_id == nullptr);

	int s = snprintf(np_id->handle.data, 16, "Kyty");

	EXIT_NOT_IMPLEMENTED(s >= 16);

	np_id->handle.term = 0;

	return OK;
}

int KYTY_SYSV_ABI NpGetOnlineId(int user_id, NpOnlineId* online_id)
{
	PRINT_NAME();

	printf("\t user_id = %d\n", user_id);

	EXIT_NOT_IMPLEMENTED(online_id == nullptr);

	int s = snprintf(online_id->data, 16, "Kyty");

	EXIT_NOT_IMPLEMENTED(s >= 16);

	online_id->term = 0;

	return OK;
}

int KYTY_SYSV_ABI NpCreateAsyncRequest(const NpCreateAsyncRequestParameter* param)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(param == nullptr);

	printf("\t size              = %" PRIu64 "\n", param->size);
	printf("\t cpu_affinity_mask = %" PRIu64 "\n", param->cpu_affinity_mask);
	printf("\t thread_priority   = %d\n", param->thread_priority);

	static std::atomic_int id = 0;

	EXIT_NOT_IMPLEMENTED(id >= 1);

	return ++id;
}

int KYTY_SYSV_ABI NpDeleteRequest(int req_id)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(req_id != 1);

	printf("\t req_id = %d\n", req_id);

	return OK;
}

int KYTY_SYSV_ABI NpCheckNpAvailability(int req_id, const char* user, void* result)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(req_id != 1);
	EXIT_NOT_IMPLEMENTED(user == nullptr);
	EXIT_NOT_IMPLEMENTED(result != nullptr);

	printf("\t req_id = %d\n", req_id);
	printf("\t user   = %s\n", user);

	return OK;
}

int KYTY_SYSV_ABI NpPollAsync(int req_id, int* result)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(req_id != 1);
	EXIT_NOT_IMPLEMENTED(result == nullptr);

	printf("\t req_id = %d\n", req_id);

	*result = 0;

	return 0;
}

int KYTY_SYSV_ABI NpGetState(int user_id, uint32_t* state)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(state == nullptr);

	printf("\t user_id = %d\n", user_id);

	*state = 1; // Signed out

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

int KYTY_SYSV_ABI NpCheckCallbackForLib()
{
	PRINT_NAME();

	return OK;
}

} // namespace NpManagerForToolkit

namespace NpTrophy {

LIB_NAME("NpTrophy", "NpTrophy");

struct NpTrophyFlagArray
{
	uint32_t flag_bits[4];
};

int KYTY_SYSV_ABI NpTrophyCreateHandle(int* handle)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(handle == nullptr);

	*handle = 1;

	return OK;
}

int KYTY_SYSV_ABI NpTrophyCreateContext(int* context, int user_id, uint32_t service_label, uint64_t options)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(context == nullptr);
	EXIT_NOT_IMPLEMENTED(options != 0);

	*context = 1;

	printf("\t user_id       = %d\n", user_id);
	printf("\t service_label = %u\n", service_label);

	return OK;
}

int KYTY_SYSV_ABI NpTrophyRegisterContext(int context, int handle, uint64_t options)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(options != 0);
	EXIT_NOT_IMPLEMENTED(context != 1);
	EXIT_NOT_IMPLEMENTED(handle != 1);

	printf("\t context = %d\n", context);
	printf("\t handle  = %d\n", handle);

	return OK;
}

int KYTY_SYSV_ABI NpTrophyDestroyHandle(int handle)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(handle != 1);

	printf("\t handle  = %d\n", handle);

	return OK;
}

int KYTY_SYSV_ABI NpTrophyGetTrophyUnlockState(int context, int handle, NpTrophyFlagArray* flags, uint32_t* count)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(flags == nullptr);
	EXIT_NOT_IMPLEMENTED(count == nullptr);
	EXIT_NOT_IMPLEMENTED(context != 1);
	EXIT_NOT_IMPLEMENTED(handle != 1);

	printf("\t context = %d\n", context);
	printf("\t handle  = %d\n", handle);

	flags->flag_bits[0] = 0;
	flags->flag_bits[1] = 0;
	flags->flag_bits[2] = 0;
	flags->flag_bits[3] = 0;

	*count = 0;

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
