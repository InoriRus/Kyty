#include "Kyty/Core/Core.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Core/Singleton.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Subsystems.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"
#include "Kyty/Scripts/Scripts.h"
#include "Kyty/UnitTest.h"

#include "Emulator/Audio.h"
#include "Emulator/Common.h"
#include "Emulator/Config.h"
#include "Emulator/Controller.h"
#include "Emulator/Graphics/Graphics.h"
#include "Emulator/Graphics/Shader.h"
#include "Emulator/Graphics/Window.h"
#include "Emulator/Kernel/FileSystem.h"
#include "Emulator/Kernel/Memory.h"
#include "Emulator/Kernel/Pthread.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/RuntimeLinker.h"
#include "Emulator/Loader/SystemContent.h"
#include "Emulator/Loader/Timer.h"
#include "Emulator/Loader/VirtualMemory.h"
#include "Emulator/Network.h"
#include "Emulator/Profiler.h"

#include <cstdlib>

namespace Kyty::Emulator {

#ifdef KYTY_EMU_ENABLED

namespace LuaFunc {

static void load_symbols(const String& id, Loader::RuntimeLinker* rt)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(rt == nullptr);
	if (!Libs::Init(id, rt->Symbols()))
	{
		EXIT("Unknown library: %s\n", id.C_Str());
	}
}

static void load_symbols_all(Loader::RuntimeLinker* rt)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(rt == nullptr);

	Libs::InitAll(rt->Symbols());
}

static void print_system_info()
{
	Loader::SystemInfo info = Loader::GetSystemInfo();

	printf("PageSize                  = %" PRIu32 "\n", info.PageSize);
	printf("MinimumApplicationAddress = 0x%016" PRIx64 "\n", info.MinimumApplicationAddress);
	printf("MaximumApplicationAddress = 0x%016" PRIx64 "\n", info.MaximumApplicationAddress);
	printf("ActiveProcessorMask       = 0x%08" PRIx32 "\n", info.ActiveProcessorMask);
	printf("NumberOfProcessors        = %" PRIu32 "\n", info.NumberOfProcessors);
	printf("ProcessorArchitecture     = %s\n", Core::EnumName(info.ProcessorArchitecture).C_Str());
	printf("AllocationGranularity     = %" PRIu32 "\n", info.AllocationGranularity);
	printf("ProcessorLevel            = %" PRIu16 "\n", info.ProcessorLevel);
	printf("ProcessorRevision         = 0x%04" PRIx16 "\n", info.ProcessorRevision);
	printf("ProcessorName             = %s\n", info.ProcessorName.C_Str());
}

static void kyty_close()
{
	auto* rt = Core::Singleton<Loader::RuntimeLinker>::Instance();

	rt->Clear();

	printf("done!\n");

	Core::SubsystemsListSingleton::Instance()->ShutdownAll();
}

static void Init(const Scripts::ScriptVar& cfg)
{
	EXIT_IF(!Core::Thread::IsMainThread());

	auto* slist = Core::SubsystemsList::Instance();

	auto* audio       = Libs::Audio::AudioSubsystem::Instance();
	auto* config      = Config::ConfigSubsystem::Instance();
	auto* controller  = Libs::Controller::ControllerSubsystem::Instance();
	auto* core        = Core::CoreSubsystem::Instance();
	auto* file_system = Libs::LibKernel::FileSystem::FileSystemSubsystem::Instance();
	auto* graphics    = Libs::Graphics::GraphicsSubsystem::Instance();
	auto* log         = Log::LogSubsystem::Instance();
	auto* memory      = Libs::LibKernel::Memory::MemorySubsystem::Instance();
	auto* network     = Libs::Network::NetworkSubsystem::Instance();
	auto* profiler    = Profiler::ProfilerSubsystem::Instance();
	auto* pthread     = Libs::LibKernel::PthreadSubsystem::Instance();
	auto* scripts     = Scripts::ScriptsSubsystem::Instance();
	auto* timer       = Loader::Timer::TimerSubsystem::Instance();

	slist->Add(config, {core, scripts});
	slist->InitAll(true);

	Config::Load(cfg);

	slist->Add(audio, {core, log, pthread, memory});
	slist->Add(controller, {core, log, config});
	slist->Add(file_system, {core, log, pthread});
	slist->Add(graphics, {core, log, pthread, memory, config, profiler, controller});
	slist->Add(log, {core, config});
	slist->Add(memory, {core, log});
	slist->Add(network, {core, log, pthread});
	slist->Add(profiler, {core, config});
	slist->Add(pthread, {core, log, timer});
	slist->Add(timer, {core, log});

	slist->InitAll(true);
}

KYTY_SCRIPT_FUNC(kyty_load_cfg_func)
{
	if (Scripts::ArgGetVarCount() != 1)
	{
		EXIT("invalid args\n");
	}

	Scripts::ScriptVar cfg = Scripts::ArgGetVar(0);

	Config::Load(cfg);

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_init_func)
{
	if (Scripts::ArgGetVarCount() != 1)
	{
		EXIT("invalid args\n");
	}

	Scripts::ScriptVar cfg = Scripts::ArgGetVar(0);

	Init(cfg);

	print_system_info();

	int ok = atexit(kyty_close);
	EXIT_NOT_IMPLEMENTED(ok != 0);

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_load_elf_func)
{
	if (Scripts::ArgGetVarCount() != 1 && Scripts::ArgGetVarCount() != 2 && Scripts::ArgGetVarCount() != 3)
	{
		EXIT("invalid args\n");
	}

	Scripts::ScriptVar elf = Scripts::ArgGetVar(0);

	auto* rt = Core::Singleton<Loader::RuntimeLinker>::Instance();

	auto* program = rt->LoadProgram(Libs::LibKernel::FileSystem::GetRealFilename(elf.ToString()));

	if (Scripts::ArgGetVarCount() >= 2)
	{
		if (Scripts::ArgGetVar(1).ToInteger() == 1)
		{
			program->dbg_print_reloc = true;
		}
	}

	if (Scripts::ArgGetVarCount() >= 3)
	{
		auto save_name = Scripts::ArgGetVar(2).ToString();

		rt->SaveProgram(program, Libs::LibKernel::FileSystem::GetRealFilename(save_name));
	}

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_save_main_elf_func)
{
	if (Scripts::ArgGetVarCount() != 1)
	{
		EXIT("invalid args\n");
	}

	Scripts::ScriptVar elf = Scripts::ArgGetVar(0);

	auto* rt = Core::Singleton<Loader::RuntimeLinker>::Instance();

	rt->SaveMainProgram(Libs::LibKernel::FileSystem::GetRealFilename(elf.ToString()));

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_load_symbols_func)
{
	auto count = Scripts::ArgGetVarCount();

	if (count < 1)
	{
		EXIT("invalid args\n");
	}

	auto* rt = Core::Singleton<Loader::RuntimeLinker>::Instance();

	for (int i = 0; i < count; i++)
	{
		Scripts::ScriptVar id = Scripts::ArgGetVar(i);
		load_symbols(id.ToString(), rt);
	}

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_load_symbols_all_func)
{
	auto count = Scripts::ArgGetVarCount();

	if (count != 0)
	{
		EXIT("invalid args\n");
	}

	auto* rt = Core::Singleton<Loader::RuntimeLinker>::Instance();

	load_symbols_all(rt);

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_load_param_sfo_func)
{
	auto count = Scripts::ArgGetVarCount();

	if (count != 1)
	{
		EXIT("invalid args\n");
	}

	auto file_name = Scripts::ArgGetVar(0).ToString();

	if (!file_name.IsEmpty())
	{
		Loader::SystemContentLoadParamSfo(Scripts::ArgGetVar(0).ToString());
	}

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_dbg_dump_func)
{
	if (Scripts::ArgGetVarCount() != 1)
	{
		EXIT("invalid args\n");
	}

	Scripts::ScriptVar dbg_dir = Scripts::ArgGetVar(0);

	auto* rt = Core::Singleton<Loader::RuntimeLinker>::Instance();

	rt->DbgDump(dbg_dir.ToString());

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_execute_func)
{
	if (Scripts::ArgGetVarCount() != 0)
	{
		EXIT("invalid args\n");
	}

	int thread_model = 1;

	if (thread_model == 0)
	{
		Core::Thread t([](void* /*unused*/) { Libs::Graphics::WindowRun(); }, nullptr);
		t.Detach();
		auto* rt = Core::Singleton<Loader::RuntimeLinker>::Instance();
		rt->Execute();
	} else
	{
		Core::Thread t(
		    [](void* /*unused*/)
		    {
			    auto* rt = Core::Singleton<Loader::RuntimeLinker>::Instance();
			    rt->Execute();
		    },
		    nullptr);
		t.Detach();
		Libs::Graphics::WindowRun();
		t.Join();
	}

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_mount_func)
{
	if (Scripts::ArgGetVarCount() != 2)
	{
		EXIT("invalid args\n");
	}

	Scripts::ScriptVar folder = Scripts::ArgGetVar(0);
	Scripts::ScriptVar point  = Scripts::ArgGetVar(1);

	Libs::LibKernel::FileSystem::Mount(folder.ToString(), point.ToString());

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_shader_disable)
{
	if (Scripts::ArgGetVarCount() != 1)
	{
		EXIT("invalid args\n");
	}

	auto id = Scripts::ArgGetVar(0).ToString().ToUint64(16);

	Libs::Graphics::ShaderDisable(id);

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_shader_printf)
{
	if (Scripts::ArgGetVarCount() != 4)
	{
		EXIT("invalid args\n");
	}

	auto id     = Scripts::ArgGetVar(0).ToString().ToUint64(16);
	auto pc     = Scripts::ArgGetVar(1).ToInteger();
	auto format = Scripts::ArgGetVar(2).ToString();
	auto args   = Scripts::ArgGetVar(3);

	if (!args.IsTable())
	{
		EXIT("invalid args\n");
	}

	Libs::Graphics::ShaderDebugPrintf p;
	p.pc     = pc;
	p.format = format;

	for (const auto& t: args.GetPairs())
	{
		const auto& arg_t = t.GetValue();

		if (!arg_t.IsTable())
		{
			EXIT("invalid arg\n");
		}

		auto type = arg_t.GetValue(0).ToString();
		auto arg  = arg_t.GetValue(1).ToString();

		Libs::Graphics::ShaderOperand op;

		if (arg.StartsWith('s', String::Case::Insensitive) && Core::Char::IsDecimal(arg.At(1)))
		{
			op.type        = Libs::Graphics::ShaderOperandType::Sgpr;
			op.register_id = arg.RemoveFirst(1).ToInt32();
			op.size        = 1;
		} else if (arg.StartsWith('v', String::Case::Insensitive) && Core::Char::IsDecimal(arg.At(1)))
		{
			op.type        = Libs::Graphics::ShaderOperandType::Vgpr;
			op.register_id = arg.RemoveFirst(1).ToInt32();
			op.size        = 1;
		} else
		{
			EXIT("unknown arg: %s\n", arg.C_Str());
		}

		p.args.Add(op);

		Libs::Graphics::ShaderDebugPrintf::Type st = Libs::Graphics::ShaderDebugPrintf::Type::Int;

		if (type.EqualNoCase(U"int"))
		{
			st = Libs::Graphics::ShaderDebugPrintf::Type::Int;
		} else if (type.EqualNoCase(U"uint"))
		{
			st = Libs::Graphics::ShaderDebugPrintf::Type::Uint;
		} else if (type.EqualNoCase(U"float"))
		{
			st = Libs::Graphics::ShaderDebugPrintf::Type::Float;
		} else
		{
			EXIT("unknown type: %s\n", arg.C_Str());
		}

		p.types.Add(st);
	}

	Libs::Graphics::ShaderInjectDebugPrintf(id, p);

	return 0;
}

KYTY_SCRIPT_FUNC(kyty_run_tests)
{
	if (!UnitTest::unit_test_all())
	{
		EXIT("test failed\n");
	}

	return 0;
}

void kyty_help() {}

} // namespace LuaFunc

void kyty_reg()
{
	Scripts::RegisterFunc("kyty_init", LuaFunc::kyty_init_func, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_load_cfg", LuaFunc::kyty_load_cfg_func, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_load_elf", LuaFunc::kyty_load_elf_func, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_save_main_elf", LuaFunc::kyty_save_main_elf_func, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_load_symbols", LuaFunc::kyty_load_symbols_func, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_load_symbols_all", LuaFunc::kyty_load_symbols_all_func, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_load_param_sfo", LuaFunc::kyty_load_param_sfo_func, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_dbg_dump", LuaFunc::kyty_dbg_dump_func, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_execute", LuaFunc::kyty_execute_func, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_mount", LuaFunc::kyty_mount_func, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_shader_disable", LuaFunc::kyty_shader_disable, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_shader_printf", LuaFunc::kyty_shader_printf, LuaFunc::kyty_help);
	Scripts::RegisterFunc("kyty_run_tests", LuaFunc::kyty_run_tests, LuaFunc::kyty_help);
}

#else
void kyty_reg() {}
#endif // KYTY_EMU_ENABLED

} // namespace Kyty::Emulator
