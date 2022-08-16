#include "Emulator/Config.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Scripts/Scripts.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Config {

struct Config
{
	uint32_t               screen_width                = 1280;
	uint32_t               screen_height               = 720;
	bool                   neo                         = true;
	bool                   next_gen                    = false;
	bool                   vulkan_validation_enabled   = false;
	bool                   shader_validation_enabled   = false;
	ShaderOptimizationType shader_optimization_type    = ShaderOptimizationType::None;
	ShaderLogDirection     shader_log_direction        = ShaderLogDirection::Silent;
	String                 shader_log_folder           = U"_Shaders";
	bool                   command_buffer_dump_enabled = false;
	String                 command_buffer_dump_folder  = U"_Buffers";
	Log::Direction         printf_direction            = Log::Direction::Console;
	String                 printf_output_file          = U"_kyty.txt";
	String                 printf_output_folder        = U"_Logs";
	ProfilerDirection      profiler_direction          = ProfilerDirection::None;
	String                 profiler_output_file        = U"_profile.prof";
	bool                   spirv_debug_printf_enabled  = false;
	bool                   pipeline_dump_enabled       = false;
	String                 pipeline_dump_folder        = U"_Pipelines";
};

static Config* g_config = nullptr;

KYTY_SUBSYSTEM_INIT(Config)
{
	EXIT_IF(g_config != nullptr);

	g_config = new Config;
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Config) {}

KYTY_SUBSYSTEM_DESTROY(Config) {}

template <class T>
void LoadInt(T& dst, const Scripts::ScriptVar& cfg, const String& key)
{
	auto var = cfg.At(key);
	if (!var.IsNil())
	{
		dst = static_cast<T>(var.ToInteger());
	}
}

void LoadBool(bool& dst, const Scripts::ScriptVar& cfg, const String& key)
{
	auto var = cfg.At(key);
	if (!var.IsNil())
	{
		dst = var.ToBool();
	}
}

template <class T>
void LoadEnum(T& dst, const Scripts::ScriptVar& cfg, const String& key)
{
	auto var = cfg.At(key);
	if (!var.IsNil())
	{
		dst = Core::EnumValue(var.ToString(), dst);
	}
}

void LoadStr(String& dst, const Scripts::ScriptVar& cfg, const String& key)
{
	auto var = cfg.At(key);
	if (!var.IsNil())
	{
		dst = var.ToString();
	}
}

void Load(const Scripts::ScriptVar& cfg)
{
	LoadInt(g_config->screen_width, cfg, U"ScreenWidth");
	LoadInt(g_config->screen_height, cfg, U"ScreenHeight");
	LoadBool(g_config->neo, cfg, U"Neo");
	LoadBool(g_config->vulkan_validation_enabled, cfg, U"VulkanValidationEnabled");
	LoadBool(g_config->shader_validation_enabled, cfg, U"ShaderValidationEnabled");
	LoadEnum(g_config->shader_optimization_type, cfg, U"ShaderOptimizationType");
	LoadEnum(g_config->shader_log_direction, cfg, U"ShaderLogDirection");
	LoadStr(g_config->shader_log_folder, cfg, U"ShaderLogFolder");
	LoadBool(g_config->command_buffer_dump_enabled, cfg, U"CommandBufferDumpEnabled");
	LoadStr(g_config->command_buffer_dump_folder, cfg, U"CommandBufferDumpFolder");
	LoadEnum(g_config->printf_direction, cfg, U"PrintfDirection");
	LoadStr(g_config->printf_output_file, cfg, U"PrintfOutputFile");
	LoadStr(g_config->printf_output_folder, cfg, U"PrintfOutputFolder");
	LoadEnum(g_config->profiler_direction, cfg, U"ProfilerDirection");
	LoadStr(g_config->profiler_output_file, cfg, U"ProfilerOutputFile");
	LoadBool(g_config->spirv_debug_printf_enabled, cfg, U"SpirvDebugPrintfEnabled");
	LoadBool(g_config->pipeline_dump_enabled, cfg, U"PipelineDumpEnabled");
	LoadStr(g_config->pipeline_dump_folder, cfg, U"PipelineDumpFolder");
}

uint32_t GetScreenWidth()
{
	return g_config->screen_width;
}

uint32_t GetScreenHeight()
{
	return g_config->screen_height;
}

bool IsNeo()
{
	return g_config->neo || g_config->next_gen;
}

bool VulkanValidationEnabled()
{
	return g_config->vulkan_validation_enabled;
}

bool ShaderValidationEnabled()
{
	return g_config->shader_validation_enabled;
}

ShaderOptimizationType GetShaderOptimizationType()
{
	return g_config->shader_optimization_type;
}

ShaderLogDirection GetShaderLogDirection()
{
	return g_config->shader_log_direction;
}

String GetShaderLogFolder()
{
	return g_config->shader_log_folder;
}

bool CommandBufferDumpEnabled()
{
	return g_config->command_buffer_dump_enabled;
}

String GetCommandBufferDumpFolder()
{
	return g_config->command_buffer_dump_folder;
}

Log::Direction GetPrintfDirection()
{
	return g_config->printf_direction;
}

String GetPrintfOutputFile()
{
	return g_config->printf_output_file;
}

String GetPrintfOutputFolder()
{
	return g_config->printf_output_folder;
}

ProfilerDirection GetProfilerDirection()
{
	return g_config->profiler_direction;
}

String GetProfilerOutputFile()
{
	return g_config->profiler_output_file;
}

bool SpirvDebugPrintfEnabled()
{
	return g_config->spirv_debug_printf_enabled;
}

bool PipelineDumpEnabled()
{
	return g_config->pipeline_dump_enabled;
}

String GetPipelineDumpFolder()
{
	return g_config->pipeline_dump_folder;
}

void SetNextGen(bool mode)
{
	g_config->next_gen = mode;
}

bool IsNextGen()
{
	return g_config->next_gen;
}

} // namespace Kyty::Config

#endif // KYTY_EMU_ENABLED
