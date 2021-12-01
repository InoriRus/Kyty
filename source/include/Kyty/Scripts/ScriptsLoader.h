#ifndef INCLUDE_KYTY_SCRIPTS_SCRIPTSLOADER_H_
#define INCLUDE_KYTY_SCRIPTS_SCRIPTSLOADER_H_

#include "Kyty/Core/String.h"

namespace Kyty::Scripts {

void   SetLoadError(const String& err);
String GetLoadError();
void   ResetLoadError();
bool   RunScript(const String& lua_file_name);

} // namespace Kyty::Scripts

#endif /* INCLUDE_KYTY_SCRIPTS_SCRIPTSLOADER_H_ */
