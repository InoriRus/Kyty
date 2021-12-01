#ifndef EMULATOR_INCLUDE_EMULATOR_LIBS_PRINTF_H_
#define EMULATOR_INCLUDE_EMULATOR_LIBS_PRINTF_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

struct VaContext;
struct VaList;

using libc_print_func_t   = KYTY_FORMAT_PRINTF(1, 2) KYTY_SYSV_ABI int (*)(const char* str, ...);
using libc_print_v_func_t = int (*)(VaContext* c);
using libc_vprint_func_t  = int (*)(const char* str, VaList* c);

libc_print_func_t   GetPrintFunc();
libc_print_v_func_t GetPrintFuncV();
libc_vprint_func_t  GetVPrintFunc();

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_LIBS_PRINTF_H_ */
