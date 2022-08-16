#ifndef EMULATOR_INCLUDE_EMULATOR_CONFIG_H_
#define EMULATOR_INCLUDE_EMULATOR_CONFIG_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Scripts {
class ScriptVar;
} // namespace Kyty::Scripts

namespace Kyty::Config {

KYTY_SUBSYSTEM_DEFINE(Config);

enum class ShaderOptimizationType
{
	None,
	Size,
	Performance
};

enum class ShaderLogDirection
{
	Silent,
	Console,
	File
};

enum class ProfilerDirection
{
	None,
	File,
	Network,
	FileAndNetwork
};

void Load(const Scripts::ScriptVar& cfg);

void SetNextGen(bool mode);

uint32_t GetScreenWidth();
uint32_t GetScreenHeight();
bool     IsNeo();
bool     IsNextGen();
bool     VulkanValidationEnabled();

bool                   ShaderValidationEnabled();
ShaderOptimizationType GetShaderOptimizationType();
ShaderLogDirection     GetShaderLogDirection();
String                 GetShaderLogFolder();

bool   CommandBufferDumpEnabled();
String GetCommandBufferDumpFolder();

Log::Direction GetPrintfDirection();
String         GetPrintfOutputFile();
String         GetPrintfOutputFolder();

ProfilerDirection GetProfilerDirection();
String            GetProfilerOutputFile();

bool SpirvDebugPrintfEnabled();

bool   PipelineDumpEnabled();
String GetPipelineDumpFolder();

} // namespace Kyty::Config

#endif

#endif /* EMULATOR_INCLUDE_EMULATOR_CONFIG_H_ */
