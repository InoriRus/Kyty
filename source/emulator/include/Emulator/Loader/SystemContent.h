#ifndef EMULATOR_INCLUDE_EMULATOR_LOADER_SYSTEMCONTENT_H_
#define EMULATOR_INCLUDE_EMULATOR_LOADER_SYSTEMCONTENT_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {
class Image;
} // namespace Kyty::Libs::Graphics

namespace Kyty::Loader {

void                   SystemContentLoadParamSfo(const String& file_name);
bool                   SystemContentParamSfoGetInt(const char* name, int32_t* value);
bool                   SystemContentParamSfoGetString(const char* name, String* value);
bool                   SystemContentParamSfoGetString(const char* name, char* value, size_t value_size);
Libs::Graphics::Image* SystemContentGetIcon();
bool                   SystemContentGetChunksNum(uint32_t* num);

} // namespace Kyty::Loader

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_LOADER_SYSTEMCONTENT_H_ */
