#include "Emulator/Libs/Errno.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Posix {

static thread_local int g_errno = 0;

KYTY_SYSV_ABI int* GetErrorAddr()
{
	return &g_errno;
}

} // namespace Kyty::Libs::Posix

#endif
