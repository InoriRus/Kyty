#include "Kyty/Core/Subsystems.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Sys/SysStdio.h"

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <new>

namespace Kyty::Core {

class SubsystemPrivate
{
public:
	SubsystemPrivate() = default;

	virtual ~SubsystemPrivate()
	{
		if (fail_msg != nullptr)
		{
			DeleteArray(fail_msg);
		}
	}

	KYTY_CLASS_NO_COPY(SubsystemPrivate);

	bool  failed {false};
	char* fail_msg {nullptr};
};

class SubsystemsListPrivate
{
public:
	explicit SubsystemsListPrivate(SubsystemsList* p): parent(p) {}

	virtual ~SubsystemsListPrivate()
	{
		SubsListStruct* n = list;

		for (;;)
		{
			if (n == nullptr)
			{
				break;
			}

			SubsListStruct* nn = n;

			DepsListStruct* dl = n->deps;

			for (;;)
			{
				if (dl == nullptr)
				{
					break;
				}

				DepsListStruct* ddl = dl;

				dl = dl->next;

				Delete(ddl);
			}

			n = n->next;

			Delete(nn);
		}
	}

	void SetArgs(int argc, char** argv)
	{
		this->m_argc = argc;
		this->m_argv = argv;
	}

	void Add(Subsystem* s, std::initializer_list<Subsystem*> deps)
	{
		EXIT_IF(!s);

		const char* name = s->Id();

		auto* nl = new SubsListStruct;

		nl->s         = s;
		nl->name      = name;
		nl->deps      = nullptr;
		nl->next      = list;
		nl->prev_init = nullptr;

		list = nl;

		for (auto* dep: deps)
		{
			const char* str = dep->Id();

			auto* l     = new DepsListStruct;
			l->dep_name = str;
			l->next     = nl->deps;

			nl->deps = l;
		}

		nl->initialized = false;
	}

	bool InitAll(bool print_msg)
	{
		for (;;)
		{
			SubsListStruct* n = FindNextToInitialize();

			if (n == nullptr)
			{
				break;
			}

			n->s->Init(parent);

			if (n->s->m_p->failed)
			{
				fail_msg  = n->s->m_p->fail_msg;
				fail_name = n->name;
				return false;
			}

			if (print_msg)
			{
				printf("Initialized: %s\n", n->name);
			}

			n->initialized = true;

			SubsListStruct* last = last_init;
			last_init            = n;
			n->prev_init         = last;
		}

		return true;
	}

	void DestroyAll(bool print_msg)
	{
		SubsListStruct* n = last_init;

		for (;;)
		{
			if (n == nullptr)
			{
				break;
			}

			n->s->Destroy(parent);
			n->initialized = false;

			if (print_msg)
			{
				printf("Destroyed: %s\n", n->name);
			}

			n = n->prev_init;
		}

		last_init = nullptr;
	}

	void ShutdownAll()
	{
		SubsListStruct* n = last_init;

		for (;;)
		{
			if (n == nullptr)
			{
				break;
			}

			n->s->UnexpectedShutdown(parent);
			n->initialized = false;

			n = n->prev_init;
		}

		last_init = nullptr;
	}

	struct DepsListStruct
	{
		const char*     dep_name;
		DepsListStruct* next;
	};

	struct SubsListStruct
	{
		Subsystem*      s;
		const char*     name;
		DepsListStruct* deps;
		SubsListStruct* next;
		SubsListStruct* prev_init;
		bool            initialized;
	};

	[[nodiscard]] SubsListStruct* FindByName(const char* name) const
	{
		SubsListStruct* n = list;

		for (;;)
		{
			if ((n == nullptr) || std::strcmp(n->name, name) == 0)
			{
				break;
			}

			n = n->next;
		}

		return n;
	}

	[[nodiscard]] SubsListStruct* FindNextToInitialize() const
	{
		SubsListStruct* n = list;

		for (;;)
		{
			if (n == nullptr)
			{
				break;
			}

			if (!n->initialized)
			{
				DepsListStruct* d = n->deps;

				for (;;)
				{
					if (d == nullptr)
					{
						break;
					}

					SubsListStruct* s = FindByName(d->dep_name);

					if ((s == nullptr) || !s->initialized)
					{
						break;
					}

					d = d->next;
				}

				if (d == nullptr)
				{
					return n;
				}
			}

			n = n->next;
		}

		return nullptr;
	}

	KYTY_CLASS_NO_COPY(SubsystemsListPrivate);

	SubsListStruct* list      = nullptr;
	SubsListStruct* last_init = nullptr;
	int             m_argc    = 0;
	char**          m_argv    = nullptr;
	const char*     fail_msg  = nullptr;
	const char*     fail_name = nullptr;
	SubsystemsList* parent;
};

// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
SubsystemsList::SubsystemsList(): m_p(static_cast<SubsystemsListPrivate*>(std::malloc(sizeof(SubsystemsListPrivate))))
{
	new (m_p) SubsystemsListPrivate(this);
}

SubsystemsList::~SubsystemsList()
{
	m_p->~SubsystemsListPrivate();
	// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	std::free(m_p);
	// delete p;
}

void SubsystemsList::Add(Subsystem* s, std::initializer_list<Subsystem*> deps)
{
	m_p->Add(s, deps);
}

bool SubsystemsList::InitAll(bool print_msg)
{
	return m_p->InitAll(print_msg);
}

void SubsystemsList::DestroyAll(bool print_msg)
{
	m_p->DestroyAll(print_msg);
}

int* SubsystemsList::GetArgc()
{
	return &m_p->m_argc;
}

char** SubsystemsList::GetArgv()
{
	return m_p->m_argv;
}

// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
Subsystem::Subsystem(): m_p(static_cast<SubsystemPrivate*>(std::malloc(sizeof(SubsystemPrivate))))
{
	new (m_p) SubsystemPrivate;
}

Subsystem::~Subsystem()
{
	m_p->~SubsystemPrivate();
	// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	std::free(m_p);
	// delete p;
}

void Subsystem::Fail(const char* format, ...)
{
	va_list args {};
	va_start(args, format);

	uint32_t len = sys_vscprintf(format, args);

	if (len != 0)
	{
		// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
		char* d = static_cast<char*>(std::malloc(len + 1));
		std::memset(d, 0, len + 1);
		/*len = */ sys_vsnprintf(d, len, format, args);
		m_p->fail_msg = d;
		m_p->failed   = true;
	}

	va_end(args);
}

const char* SubsystemsList::GetFailName() const
{
	return m_p->fail_name;
}

const char* SubsystemsList::GetFailMsg() const
{
	return m_p->fail_msg;
}

void SubsystemsList::SetArgs(int argc, char* argv[])
{
	m_p->SetArgs(argc, argv);
}

void SubsystemsList::ShutdownAll()
{
	m_p->ShutdownAll();
}

} // namespace Kyty::Core
