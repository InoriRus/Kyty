#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_SHADERPARSE_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_SHADERPARSE_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

class ShaderCode;

void ShaderParse(const uint32_t* src, ShaderCode* dst);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_SHADERPARSE_H_ */
