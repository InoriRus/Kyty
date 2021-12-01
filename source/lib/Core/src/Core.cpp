#include "Kyty/Core/Core.h"

#include "Kyty/Core/ArrayWrapper.h" // IWYU pragma: associated
#include "Kyty/Core/ByteBuffer.h"   // IWYU pragma: associated
#include "Kyty/Core/Common.h"       // IWYU pragma: associated
#include "Kyty/Core/Database.h"
#include "Kyty/Core/Debug.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/Hash.h" // IWYU pragma: associated
#include "Kyty/Core/Language.h"
#include "Kyty/Core/LinkList.h"  // IWYU pragma: associated
#include "Kyty/Core/MagicEnum.h" // IWYU pragma: associated
#include "Kyty/Core/MemoryAlloc.h"
#include "Kyty/Core/RefCounter.h"  // IWYU pragma: associated
#include "Kyty/Core/SafeDelete.h"  // IWYU pragma: associated
#include "Kyty/Core/SimpleArray.h" // IWYU pragma: associated
#include "Kyty/Core/Singleton.h"   // IWYU pragma: associated
#include "Kyty/Core/Vector.h"      // IWYU pragma: associated

namespace Kyty::Core {

KYTY_SUBSYSTEM_INIT(Core)
{
	core_memory_init();
	core_file_init();
	core_debug_init(parent->GetArgv()[0]);
	Language::Init();
	Database::Init();
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Core) {}

KYTY_SUBSYSTEM_DESTROY(Core) {}

} // namespace Kyty::Core
