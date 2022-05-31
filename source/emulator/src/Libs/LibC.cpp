#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/LinkList.h"
#include "Kyty/Core/MSpace.h"
#include "Kyty/Core/Singleton.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Libs/Printf.h"
#include "Emulator/Libs/VaContext.h"
#include "Emulator/Loader/SymbolDatabase.h"

#include <cstdlib>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

namespace LibC {

LIB_VERSION("libc", 1, "libc", 1, 1);

static uint32_t g_need_flag = 1;

using cxa_destructor_func_t = void (*)(void*);

struct CxaDestructor
{
	cxa_destructor_func_t destructor_func;
	void*                 destructor_object;
	void*                 module_id;
};

struct CContext
{
	Core::List<CxaDestructor> cxa;
};

static KYTY_SYSV_ABI void exit(int code)
{
	PRINT_NAME();

	::exit(code);
}

static KYTY_SYSV_ABI void init_env()
{
	PRINT_NAME();
}

static KYTY_SYSV_ABI int atexit(void (*func)())
{
	PRINT_NAME();

	::printf("func = %" PRIx64 "\n", reinterpret_cast<uint64_t>(func));

	int ok = ::atexit(func);

	EXIT_NOT_IMPLEMENTED(ok != 0);

	return 0;
}

static KYTY_SYSV_ABI int libc_printf(VA_ARGS)
{
	VA_CONTEXT(ctx); // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)

	PRINT_NAME();

	return GetPrintfCtxFunc()(&ctx);
}

static KYTY_SYSV_ABI int puts(const char* s)
{
	PRINT_NAME();

	return GetPrintfStdFunc()("%s\n", s);
}

static KYTY_SYSV_ABI void catchReturnFromMain(int status)
{
	PRINT_NAME();

	::printf("return from main = %d\n", status);
}

static KYTY_SYSV_ABI int cxa_atexit(void (*func)(void*), void* arg, void* d)
{
	PRINT_NAME();

	auto* cc = Core::Singleton<CContext>::Instance();

	CxaDestructor c {};
	c.destructor_func   = func;
	c.destructor_object = arg;
	c.module_id         = d;

	cc->cxa.Add(c);

	return 0;
}

void KYTY_SYSV_ABI cxa_finalize(void* d)
{
	PRINT_NAME();

	auto* cc = Core::Singleton<CContext>::Instance();

	FOR_LIST_R(i, cc->cxa)
	{
		auto& c = cc->cxa[i];
		if (c.module_id == d && c.destructor_func != nullptr)
		{
			c.destructor_func(c.destructor_object);
			c.destructor_func = nullptr;
		}
	}
}

} // namespace LibC

namespace LibcInternalExt {

LIB_VERSION("LibcInternalExt", 1, "LibcInternal", 1, 1);

static uint64_t g_mspace_atomic_id_mask = 0;
static uint64_t g_mstate_table[64]      = {0};

struct Info
{
	uint64_t  size;
	uint32_t  unknown1;
	uint32_t  unknown2;
	uint64_t* mspace_atomic_id_mask;
	uint64_t* mstate_table;
};

void KYTY_SYSV_ABI LibcHeapGetTraceInfo(Info* info)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(info->size != 32);

	info->mspace_atomic_id_mask = &g_mspace_atomic_id_mask;
	info->mstate_table          = g_mstate_table;
}

LIB_DEFINE(InitLibcInternalExt_1)
{
	LIB_FUNC("NWtTN10cJzE", LibcInternalExt::LibcHeapGetTraceInfo);
}

} // namespace LibcInternalExt

namespace LibcInternal {

LIB_VERSION("LibcInternal", 1, "LibcInternal", 1, 1);

static uint32_t g_need_flag = 1;

int KYTY_SYSV_ABI vprintf(const char* str, VaList* c)
{
	PRINT_NAME();

	return GetVprintfFunc()(str, c);
}

static KYTY_SYSV_ABI int snprintf(VA_ARGS)
{
	VA_CONTEXT(ctx); // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)

	PRINT_NAME();

	return GetSnrintfCtxFunc()(&ctx);
}

int KYTY_SYSV_ABI fflush(FILE* stream)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(stream != stdout);

	return ::fflush(stream);
}

void* KYTY_SYSV_ABI memset(void* s, int c, size_t n)
{
	PRINT_NAME();

	return ::memset(s, c, n);
}

void* KYTY_SYSV_ABI LibcMspaceCreate(const char* name, void* base, size_t capacity, uint32_t flag)
{
	PRINT_NAME();

	printf("\t name     = %s\n", name);
	printf("\t base     = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(base));
	printf("\t capacity = %016" PRIx64 "\n", capacity);
	printf("\t flag     = %u\n", flag);

	EXIT_NOT_IMPLEMENTED(flag != 0 && flag != 1);
	EXIT_NOT_IMPLEMENTED(name == nullptr);
	EXIT_NOT_IMPLEMENTED(base == nullptr);
	EXIT_NOT_IMPLEMENTED(capacity == 0);

	bool thread_safe = true;

	if (flag == 1)
	{
		thread_safe = false;
	}

	auto* msp = Core::MSpaceCreate(name, base, capacity, thread_safe, nullptr);

	EXIT_NOT_IMPLEMENTED(msp == nullptr);

	return msp;
}

void* KYTY_SYSV_ABI LibcMspaceMalloc(void* msp, size_t size)
{
	PRINT_NAME();

	printf("\t size = %016" PRIx64 "\n", size);

	auto* buf = Core::MSpaceMalloc(msp, size);

	printf("\t buf  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(buf));

	EXIT_NOT_IMPLEMENTED(buf == nullptr);

	return buf;
}

LIB_DEFINE(InitLibcInternal_1)
{
	LibcInternalExt::InitLibcInternalExt_1(s);

	LIB_OBJECT("ZT4ODD2Ts9o", &LibcInternal::g_need_flag);
	LIB_OBJECT("2sWzhYqFH4E", stdout);

	LIB_FUNC("GMpvxPFW924", LibcInternal::vprintf);
	LIB_FUNC("MUjC4lbHrK4", LibcInternal::fflush);
	LIB_FUNC("8zTFvBIAIN8", LibcInternal::memset);
	LIB_FUNC("eLdDw6l0-bU", LibcInternal::snprintf);

	LIB_FUNC("tsvEmnenz48", LibC::cxa_atexit);
	LIB_FUNC("H2e8t5ScQGc", LibC::cxa_finalize);

	LIB_FUNC("-hn1tcVHq5Q", LibcInternal::LibcMspaceCreate);
	LIB_FUNC("OJjm-QOIHlI", LibcInternal::LibcMspaceMalloc);
}

} // namespace LibcInternal

LIB_USING(LibC);

LIB_DEFINE(InitLibC_1)
{
	EXIT("deprecated\n");

	LibcInternal::InitLibcInternal_1(s);

	LIB_OBJECT("P330P3dFF68", &LibC::g_need_flag);

	LIB_FUNC("uMei1W9uyNo", LibC::exit);
	LIB_FUNC("bzQExy189ZI", LibC::init_env);
	LIB_FUNC("8G2LB+A3rzg", LibC::atexit);
	LIB_FUNC("hcuQgD53UxM", LibC::libc_printf);
	LIB_FUNC("YQ0navp+YIc", LibC::puts);
	LIB_FUNC("XKRegsFpEpk", LibC::catchReturnFromMain);
	LIB_FUNC("tsvEmnenz48", LibC::cxa_atexit);
	LIB_FUNC("H2e8t5ScQGc", LibC::cxa_finalize);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
