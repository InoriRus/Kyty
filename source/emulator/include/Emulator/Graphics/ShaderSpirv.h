#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_SHADERSPIRV_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_SHADERSPIRV_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String8.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

class ShaderCode;
struct ShaderVertexInputInfo;
struct ShaderPixelInputInfo;
struct ShaderComputeInputInfo;

String8 SpirvGenerateSource(const ShaderCode& code, const ShaderVertexInputInfo* vs_input_info, const ShaderPixelInputInfo* ps_input_info,
                            const ShaderComputeInputInfo* cs_input_info);
String8 SpirvGetEmbeddedVs(uint32_t id);
String8 SpirvGetEmbeddedPs(uint32_t id);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_SHADERSPIRV_H_ */
