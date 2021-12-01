
local cfg = {
	ScreenWidth = 1280;
	ScreenHeight = 720;	
	Neo = true;
	VulkanValidationEnabled = true;
	ShaderValidationEnabled = true;		
	ShaderOptimizationType = 'Performance'; -- None, Size, Performance
	ShaderLogDirection = 'File'; -- Silent, Console, File
	ShaderLogFolder = '_Shaders';
	CommandBufferDumpEnabled = true;
	CommandBufferDumpFolder = '_Buffers';
	PrintfDirection = 'Console'; -- Silent, Console, File
	PrintfOutputFile = '_kyty.txt';
	ProfilerDirection = 'None'; -- None, File, Network,	FileAndNetwork
	ProfilerOutputFile = '_profile.prof';
}

kyty_init(cfg);

kyty_mount('z:/dev/ps5/tests/01_Hello/Debug', '/app0');

kyty_load_elf('/app0/main.elf');
kyty_load_elf('/app0/sce_module/libc.prx');

kyty_load_symbols('libc_internal_1');
kyty_load_symbols('libkernel_1');
kyty_load_symbols('libVideoOut_1');
kyty_load_symbols('libSysmodule_1');
kyty_load_symbols('libDiscMap_1');

--kyty_dbg_dump('_elf/');

kyty_execute();



