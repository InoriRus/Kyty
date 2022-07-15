#include "Emulator/Graphics/Graphics.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/String.h"

#include "Emulator/Config.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/GraphicsRun.h"
#include "Emulator/Graphics/HardwareContext.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"
#include "Emulator/Graphics/Objects/IndexBuffer.h"
#include "Emulator/Graphics/Objects/Label.h"
#include "Emulator/Graphics/Pm4.h"
#include "Emulator/Graphics/Tile.h"
#include "Emulator/Graphics/VideoOut.h"
#include "Emulator/Graphics/Window.h"
#include "Emulator/Kernel/Pthread.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/VirtualMemory.h"

#include <algorithm>
#include <atomic>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

KYTY_SUBSYSTEM_INIT(Graphics)
{
	auto width  = Config::GetScreenWidth();
	auto height = Config::GetScreenHeight();

	WindowInit(width, height);
	VideoOut::VideoOutInit(width, height);
	GraphicsRenderInit();
	GraphicsRunInit();
	GpuMemoryInit();
	LabelInit();
	TileInit();
	IndexBufferInit();
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Graphics) {}

KYTY_SUBSYSTEM_DESTROY(Graphics) {}

void GraphicsDbgDumpDcb(const char* type, uint32_t num_dw, uint32_t* cmd_buffer)
{
	EXIT_IF(type == nullptr);

	static std::atomic_int id = 0;

	if (Config::CommandBufferDumpEnabled() && num_dw > 0 && cmd_buffer != nullptr)
	{
		Core::File f;
		String     file_name = Config::GetCommandBufferDumpFolder().FixDirectorySlash() +
		                   String::FromPrintf("%04d_%04d_buffer_%s.log", GraphicsRunGetFrameNum(), id++, type);
		Core::File::CreateDirectories(file_name.DirectoryWithoutFilename());
		f.Create(file_name);
		if (f.IsInvalid())
		{
			printf(FG_BRIGHT_RED "Can't create file: %s\n" FG_DEFAULT, file_name.C_Str());
			return;
		}
		Pm4::DumpPm4PacketStream(&f, cmd_buffer, 0, num_dw);
		f.Close();
	}
}

namespace Gen4 {

LIB_NAME("GraphicsDriver", "GraphicsDriver");

int KYTY_SYSV_ABI GraphicsSetVsShader(uint32_t* cmd, uint64_t size, const HW::VsStageRegisters* vs_regs, uint32_t shader_modifier)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < sizeof(HW::VsStageRegisters) / 4 + 2);

	printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size            = %" PRIu64 "\n", size);
	printf("\t shader_modifier = %" PRIu32 "\n", shader_modifier);

	printf("\t vs_regs.m_spiShaderPgmLoVs    = %08" PRIx32 "\n", vs_regs->m_spiShaderPgmLoVs);
	printf("\t vs_regs.m_spiShaderPgmHiVs    = %08" PRIx32 "\n", vs_regs->m_spiShaderPgmHiVs);
	printf("\t vs_regs.m_spiShaderPgmRsrc1Vs = %08" PRIx32 "\n", vs_regs->m_spiShaderPgmRsrc1Vs);
	printf("\t vs_regs.m_spiShaderPgmRsrc2Vs = %08" PRIx32 "\n", vs_regs->m_spiShaderPgmRsrc2Vs);
	printf("\t vs_regs.m_spiVsOutConfig      = %08" PRIx32 "\n", vs_regs->m_spiVsOutConfig);
	printf("\t vs_regs.m_spiShaderPosFormat  = %08" PRIx32 "\n", vs_regs->m_spiShaderPosFormat);
	printf("\t vs_regs.m_paClVsOutCntl       = %08" PRIx32 "\n", vs_regs->m_paClVsOutCntl);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_VS);
	cmd[1] = shader_modifier;
	memcpy(&cmd[2], vs_regs, sizeof(HW::VsStageRegisters));

	return OK;
}

int KYTY_SYSV_ABI GraphicsUpdateVsShader(uint32_t* cmd, uint64_t size, const HW::VsStageRegisters* vs_regs, uint32_t shader_modifier)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < sizeof(HW::VsStageRegisters) / 4 + 2);

	printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size            = %" PRIu64 "\n", size);
	printf("\t shader_modifier = %" PRIu32 "\n", shader_modifier);

	printf("\t vs_regs.m_spiShaderPgmLoVs    = %08" PRIx32 "\n", vs_regs->m_spiShaderPgmLoVs);
	printf("\t vs_regs.m_spiShaderPgmHiVs    = %08" PRIx32 "\n", vs_regs->m_spiShaderPgmHiVs);
	printf("\t vs_regs.m_spiShaderPgmRsrc1Vs = %08" PRIx32 "\n", vs_regs->m_spiShaderPgmRsrc1Vs);
	printf("\t vs_regs.m_spiShaderPgmRsrc2Vs = %08" PRIx32 "\n", vs_regs->m_spiShaderPgmRsrc2Vs);
	printf("\t vs_regs.m_spiVsOutConfig      = %08" PRIx32 "\n", vs_regs->m_spiVsOutConfig);
	printf("\t vs_regs.m_spiShaderPosFormat  = %08" PRIx32 "\n", vs_regs->m_spiShaderPosFormat);
	printf("\t vs_regs.m_paClVsOutCntl       = %08" PRIx32 "\n", vs_regs->m_paClVsOutCntl);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_VS_UPDATE);
	cmd[1] = shader_modifier;
	memcpy(&cmd[2], vs_regs, sizeof(HW::VsStageRegisters));

	return OK;
}

int KYTY_SYSV_ABI GraphicsSetEmbeddedVsShader(uint32_t* cmd, uint64_t size, uint32_t id, uint32_t shader_modifier)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 3);

	printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size            = %" PRIu64 "\n", size);
	printf("\t id              = %" PRIu32 "\n", id);
	printf("\t shader_modifier = %" PRIu32 "\n", shader_modifier);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_VS_EMBEDDED);
	cmd[1] = shader_modifier;
	cmd[2] = id;

	return OK;
}

int KYTY_SYSV_ABI GraphicsSetPsShader(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs)
{
	PRINT_NAME();

	if (ps_regs == nullptr)
	{
		EXIT_NOT_IMPLEMENTED(size < 1 + 1);

		printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
		printf("\t size            = %" PRIu64 "\n", size);
		printf("\t embedded_id     = %d\n", 0);

		cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_PS_EMBEDDED);
		cmd[1] = 0;
	} else
	{
		EXIT_NOT_IMPLEMENTED(size < 12 + 1);

		printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
		printf("\t size            = %" PRIu64 "\n", size);

		printf("\t ps_regs.m_spiShaderPgmLoPs    = %08" PRIx32 "\n", ps_regs[0]);
		printf("\t ps_regs.m_spiShaderPgmHiPs    = %08" PRIx32 "\n", ps_regs[1]);
		printf("\t ps_regs.m_spiShaderPgmRsrc1Ps = %08" PRIx32 "\n", ps_regs[2]);
		printf("\t ps_regs.m_spiShaderPgmRsrc2Ps = %08" PRIx32 "\n", ps_regs[3]);
		printf("\t ps_regs.m_spiShaderZFormat    = %08" PRIx32 "\n", ps_regs[4]);
		printf("\t ps_regs.m_spiShaderColFormat  = %08" PRIx32 "\n", ps_regs[5]);
		printf("\t ps_regs.m_spiPsInputEna       = %08" PRIx32 "\n", ps_regs[6]);
		printf("\t ps_regs.m_spiPsInputAddr      = %08" PRIx32 "\n", ps_regs[7]);
		printf("\t ps_regs.m_spiPsInControl      = %08" PRIx32 "\n", ps_regs[8]);
		printf("\t ps_regs.m_spiBarycCntl        = %08" PRIx32 "\n", ps_regs[9]);
		printf("\t ps_regs.m_dbShaderControl     = %08" PRIx32 "\n", ps_regs[10]);
		printf("\t ps_regs.m_cbShaderMask        = %08" PRIx32 "\n", ps_regs[11]);

		cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_PS);
		memcpy(&cmd[1], ps_regs, static_cast<size_t>(12) * 4);
	}

	return OK;
}

int KYTY_SYSV_ABI GraphicsSetPsShader350(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs)
{
	PRINT_NAME();

	if (ps_regs == nullptr)
	{
		EXIT_NOT_IMPLEMENTED(size < 1 + 1);

		printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
		printf("\t size            = %" PRIu64 "\n", size);
		printf("\t embedded_id     = %d\n", 0);

		cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_PS_EMBEDDED);
		cmd[1] = 0;
	} else
	{
		EXIT_NOT_IMPLEMENTED(size < 12 + 1);

		printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
		printf("\t size            = %" PRIu64 "\n", size);

		printf("\t ps_regs.m_spiShaderPgmLoPs    = %08" PRIx32 "\n", ps_regs[0]);
		printf("\t ps_regs.m_spiShaderPgmHiPs    = %08" PRIx32 "\n", ps_regs[1]);
		printf("\t ps_regs.m_spiShaderPgmRsrc1Ps = %08" PRIx32 "\n", ps_regs[2]);
		printf("\t ps_regs.m_spiShaderPgmRsrc2Ps = %08" PRIx32 "\n", ps_regs[3]);
		printf("\t ps_regs.m_spiShaderZFormat    = %08" PRIx32 "\n", ps_regs[4]);
		printf("\t ps_regs.m_spiShaderColFormat  = %08" PRIx32 "\n", ps_regs[5]);
		printf("\t ps_regs.m_spiPsInputEna       = %08" PRIx32 "\n", ps_regs[6]);
		printf("\t ps_regs.m_spiPsInputAddr      = %08" PRIx32 "\n", ps_regs[7]);
		printf("\t ps_regs.m_spiPsInControl      = %08" PRIx32 "\n", ps_regs[8]);
		printf("\t ps_regs.m_spiBarycCntl        = %08" PRIx32 "\n", ps_regs[9]);
		printf("\t ps_regs.m_dbShaderControl     = %08" PRIx32 "\n", ps_regs[10]);
		printf("\t ps_regs.m_cbShaderMask        = %08" PRIx32 "\n", ps_regs[11]);

		cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_PS);
		memcpy(&cmd[1], ps_regs, static_cast<size_t>(12) * 4);
	}

	// printf("ok\n");

	return OK;
}

int KYTY_SYSV_ABI GraphicsUpdatePsShader(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(ps_regs == nullptr);
	EXIT_NOT_IMPLEMENTED(size < 12 + 1);

	printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size            = %" PRIu64 "\n", size);

	printf("\t ps_regs.m_spiShaderPgmLoPs    = %08" PRIx32 "\n", ps_regs[0]);
	printf("\t ps_regs.m_spiShaderPgmHiPs    = %08" PRIx32 "\n", ps_regs[1]);
	printf("\t ps_regs.m_spiShaderPgmRsrc1Ps = %08" PRIx32 "\n", ps_regs[2]);
	printf("\t ps_regs.m_spiShaderPgmRsrc2Ps = %08" PRIx32 "\n", ps_regs[3]);
	printf("\t ps_regs.m_spiShaderZFormat    = %08" PRIx32 "\n", ps_regs[4]);
	printf("\t ps_regs.m_spiShaderColFormat  = %08" PRIx32 "\n", ps_regs[5]);
	printf("\t ps_regs.m_spiPsInputEna       = %08" PRIx32 "\n", ps_regs[6]);
	printf("\t ps_regs.m_spiPsInputAddr      = %08" PRIx32 "\n", ps_regs[7]);
	printf("\t ps_regs.m_spiPsInControl      = %08" PRIx32 "\n", ps_regs[8]);
	printf("\t ps_regs.m_spiBarycCntl        = %08" PRIx32 "\n", ps_regs[9]);
	printf("\t ps_regs.m_dbShaderControl     = %08" PRIx32 "\n", ps_regs[10]);
	printf("\t ps_regs.m_cbShaderMask        = %08" PRIx32 "\n", ps_regs[11]);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_PS_UPDATE);
	memcpy(&cmd[1], ps_regs, static_cast<size_t>(12) * 4);

	return OK;
}

int KYTY_SYSV_ABI GraphicsUpdatePsShader350(uint32_t* cmd, uint64_t size, const uint32_t* ps_regs)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(ps_regs == nullptr);
	EXIT_NOT_IMPLEMENTED(size < 12 + 1);

	printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size            = %" PRIu64 "\n", size);

	printf("\t ps_regs.m_spiShaderPgmLoPs    = %08" PRIx32 "\n", ps_regs[0]);
	printf("\t ps_regs.m_spiShaderPgmHiPs    = %08" PRIx32 "\n", ps_regs[1]);
	printf("\t ps_regs.m_spiShaderPgmRsrc1Ps = %08" PRIx32 "\n", ps_regs[2]);
	printf("\t ps_regs.m_spiShaderPgmRsrc2Ps = %08" PRIx32 "\n", ps_regs[3]);
	printf("\t ps_regs.m_spiShaderZFormat    = %08" PRIx32 "\n", ps_regs[4]);
	printf("\t ps_regs.m_spiShaderColFormat  = %08" PRIx32 "\n", ps_regs[5]);
	printf("\t ps_regs.m_spiPsInputEna       = %08" PRIx32 "\n", ps_regs[6]);
	printf("\t ps_regs.m_spiPsInputAddr      = %08" PRIx32 "\n", ps_regs[7]);
	printf("\t ps_regs.m_spiPsInControl      = %08" PRIx32 "\n", ps_regs[8]);
	printf("\t ps_regs.m_spiBarycCntl        = %08" PRIx32 "\n", ps_regs[9]);
	printf("\t ps_regs.m_dbShaderControl     = %08" PRIx32 "\n", ps_regs[10]);
	printf("\t ps_regs.m_cbShaderMask        = %08" PRIx32 "\n", ps_regs[11]);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_PS_UPDATE);
	memcpy(&cmd[1], ps_regs, static_cast<size_t>(12) * 4);

	return OK;
}

int KYTY_SYSV_ABI GraphicsSetCsShaderWithModifier(uint32_t* cmd, uint64_t size, const uint32_t* cs_regs, uint32_t shader_modifier)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 7 + 2);

	printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size            = %" PRIu64 "\n", size);
	printf("\t shader_modifier = %" PRIu32 "\n", shader_modifier);

	printf("\t cs_regs.m_computePgmLo      = %08" PRIx32 "\n", cs_regs[0]);
	printf("\t cs_regs.m_computePgmHi      = %08" PRIx32 "\n", cs_regs[1]);
	printf("\t cs_regs.m_computePgmRsrc1   = %08" PRIx32 "\n", cs_regs[2]);
	printf("\t cs_regs.m_computePgmRsrc2   = %08" PRIx32 "\n", cs_regs[3]);
	printf("\t cs_regs.m_computeNumThreadX = %08" PRIx32 "\n", cs_regs[4]);
	printf("\t cs_regs.m_computeNumThreadY = %08" PRIx32 "\n", cs_regs[5]);
	printf("\t cs_regs.m_computeNumThreadZ = %08" PRIx32 "\n", cs_regs[6]);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_CS);
	cmd[1] = shader_modifier;
	memcpy(&cmd[2], cs_regs, static_cast<size_t>(7) * 4);

	return OK;
}

int KYTY_SYSV_ABI GraphicsDrawIndex(uint32_t* cmd, uint64_t size, uint32_t index_count, const void* index_addr, uint32_t flags,
                                    uint32_t type)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 6);

	printf("\tcmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\tsize        = %" PRIu64 "\n", size);
	printf("\tindex_count = %" PRIu32 "\n", index_count);
	printf("\tindex_addr  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(index_addr));
	printf("\tflags       = %08" PRIx32 "\n", flags);
	printf("\ttype        = %" PRIu32 "\n", type);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_DRAW_INDEX);
	cmd[1] = index_count;
	cmd[2] = static_cast<uint32_t>(reinterpret_cast<uint64_t>(index_addr) & 0xffffffffu);
	cmd[3] = static_cast<uint32_t>((reinterpret_cast<uint64_t>(index_addr) >> 32u) & 0xffffffffu);
	cmd[4] = flags;
	cmd[5] = type;

	return OK;
}

int KYTY_SYSV_ABI GraphicsDrawIndexAuto(uint32_t* cmd, uint64_t size, uint32_t index_count, uint32_t flags)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 3);

	printf("\t cmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size        = %" PRIu64 "\n", size);
	printf("\t index_count = %" PRIu32 "\n", index_count);
	printf("\t flags       = %08" PRIx32 "\n", flags);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_DRAW_INDEX_AUTO);
	cmd[1] = index_count;
	cmd[2] = flags;

	return OK;
}

int KYTY_SYSV_ABI GraphicsInsertWaitFlipDone(uint32_t* cmd, uint64_t size, uint32_t video_out_handle, uint32_t display_buffer_index)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 3);

	printf("\t cmd_buffer           = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size                 = %" PRIu64 "\n", size);
	printf("\t video_out_handle     = %" PRIu32 "\n", video_out_handle);
	printf("\t display_buffer_index = %" PRIu32 "\n", display_buffer_index);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_WAIT_FLIP_DONE);
	cmd[1] = video_out_handle;
	cmd[2] = display_buffer_index;

	return OK;
}

int KYTY_SYSV_ABI GraphicsDispatchDirect(uint32_t* cmd, uint64_t size, uint32_t thread_group_x, uint32_t thread_group_y,
                                         uint32_t thread_group_z, uint32_t mode)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 5);

	printf("\t cmd_buffer     = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size           = %" PRIu64 "\n", size);
	printf("\t thread_group_x = %" PRIu32 "\n", thread_group_x);
	printf("\t thread_group_y = %" PRIu32 "\n", thread_group_y);
	printf("\t thread_group_z = %" PRIu32 "\n", thread_group_z);
	printf("\t mode           = %" PRIu32 "\n", mode);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_DISPATCH_DIRECT);
	cmd[1] = thread_group_x;
	cmd[2] = thread_group_y;
	cmd[3] = thread_group_z;
	cmd[4] = mode;

	return OK;
}

uint32_t KYTY_SYSV_ABI GraphicsDrawInitDefaultHardwareState(uint32_t* cmd, uint64_t size)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 2);

	printf("\t cmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size        = %" PRIu64 "\n", size);

	cmd[0] = KYTY_PM4(2, Pm4::IT_NOP, Pm4::R_DRAW_RESET);

	return 2;
}

uint32_t KYTY_SYSV_ABI GraphicsDrawInitDefaultHardwareState175(uint32_t* cmd, uint64_t size)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 2);

	printf("\t cmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size        = %" PRIu64 "\n", size);

	cmd[0] = KYTY_PM4(2, Pm4::IT_NOP, Pm4::R_DRAW_RESET);

	return 2;
}

uint32_t KYTY_SYSV_ABI GraphicsDrawInitDefaultHardwareState200(uint32_t* cmd, uint64_t size)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 2);

	printf("\t cmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size        = %" PRIu64 "\n", size);

	cmd[0] = KYTY_PM4(2, Pm4::IT_NOP, Pm4::R_DRAW_RESET);

	return 2;
}

uint32_t KYTY_SYSV_ABI GraphicsDrawInitDefaultHardwareState350(uint32_t* cmd, uint64_t size)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 2);

	printf("\t cmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size        = %" PRIu64 "\n", size);

	cmd[0] = KYTY_PM4(2, Pm4::IT_NOP, Pm4::R_DRAW_RESET);

	return 2;
}

uint32_t KYTY_SYSV_ABI GraphicsDispatchInitDefaultHardwareState(uint32_t* cmd, uint64_t size)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 2);

	printf("\t cmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size        = %" PRIu64 "\n", size);

	cmd[0] = KYTY_PM4(2, Pm4::IT_NOP, Pm4::R_DISPATCH_RESET);

	return 2;
}

int KYTY_SYSV_ABI GraphicsSubmitCommandBuffers(uint32_t count, void* dcb_gpu_addrs[], const uint32_t* dcb_sizes_in_bytes,
                                               void* ccb_gpu_addrs[], const uint32_t* ccb_sizes_in_bytes)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(count != 1);

	auto* dcb      = (dcb_gpu_addrs == nullptr ? nullptr : static_cast<uint32_t*>(dcb_gpu_addrs[0]));
	auto  dcb_size = (dcb_sizes_in_bytes == nullptr ? 0 : dcb_sizes_in_bytes[0] / 4);
	auto* ccb      = (ccb_gpu_addrs == nullptr ? nullptr : static_cast<uint32_t*>(ccb_gpu_addrs[0]));
	auto  ccb_size = (ccb_sizes_in_bytes == nullptr ? 0 : ccb_sizes_in_bytes[0] / 4);

	GraphicsDbgDumpDcb("d", dcb_size, dcb);
	GraphicsDbgDumpDcb("c", ccb_size, ccb);

	GraphicsRunSubmit(dcb, dcb_size, ccb, ccb_size);

	return OK;
}

int KYTY_SYSV_ABI GraphicsSubmitAndFlipCommandBuffers(uint32_t count, void* dcb_gpu_addrs[], const uint32_t* dcb_sizes_in_bytes,
                                                      void* ccb_gpu_addrs[], const uint32_t* ccb_sizes_in_bytes, int handle, int index,
                                                      int flip_mode, int64_t flip_arg)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(count != 1);

	auto* dcb      = (dcb_gpu_addrs == nullptr ? nullptr : static_cast<uint32_t*>(dcb_gpu_addrs[0]));
	auto  dcb_size = (dcb_sizes_in_bytes == nullptr ? 0 : dcb_sizes_in_bytes[0] / 4);
	auto* ccb      = (ccb_gpu_addrs == nullptr ? nullptr : static_cast<uint32_t*>(ccb_gpu_addrs[0]));
	auto  ccb_size = (ccb_sizes_in_bytes == nullptr ? 0 : ccb_sizes_in_bytes[0] / 4);

	GraphicsDbgDumpDcb("d", dcb_size, dcb);
	GraphicsDbgDumpDcb("c", ccb_size, ccb);

	printf("\t handle    = %" PRId32 "\n", handle);
	printf("\t index     = %" PRId32 "\n", index);
	printf("\t flip_mode = %" PRId32 "\n", flip_mode);
	printf("\t flip_arg  = %" PRId64 "\n", flip_arg);

	GraphicsRunSubmitAndFlip(dcb, dcb_size, ccb, ccb_size, handle, index, flip_mode, flip_arg);

	return OK;
}

int KYTY_SYSV_ABI GraphicsSubmitDone()
{
	PRINT_NAME();

	GraphicsRunDone();

	return OK;
}

int KYTY_SYSV_ABI GraphicsAreSubmitsAllowed()
{
	return GraphicsRunAreSubmitsAllowed() ? 1 : 0;
}

void KYTY_SYSV_ABI GraphicsFlushMemory()
{
	PRINT_NAME();

	GpuMemoryFlushAll(WindowGetGraphicContext());
}

int KYTY_SYSV_ABI GraphicsAddEqEvent(LibKernel::EventQueue::KernelEqueue eq, int id, void* udata)
{
	PRINT_NAME();

	if (eq == nullptr)
	{
		return LibKernel::KERNEL_ERROR_EBADF;
	}

	return GraphicsRenderAddEqEvent(eq, id, udata);
}

int KYTY_SYSV_ABI GraphicsDeleteEqEvent(LibKernel::EventQueue::KernelEqueue eq, int id)
{
	PRINT_NAME();

	if (eq == nullptr)
	{
		return LibKernel::KERNEL_ERROR_EBADF;
	}

	return GraphicsRenderDeleteEqEvent(eq, id);
}

uint32_t KYTY_SYSV_ABI GraphicsMapComputeQueue(uint32_t pipe_id, uint32_t queue_id, uint32_t* ring_addr, uint32_t ring_size_dw,
                                               uint32_t* read_ptr_addr)
{
	PRINT_NAME();

	printf("\t pipe_id       = %" PRIu32 "\n", pipe_id);
	printf("\t queue_id      = %" PRIu32 "\n", queue_id);
	printf("\t ring_addr     = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(ring_addr));
	printf("\t ring_size_dw  = %" PRIu32 "\n", ring_size_dw);
	printf("\t read_ptr_addr = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(read_ptr_addr));

	uint32_t id = GraphicsRunMapComputeQueue(pipe_id, queue_id, ring_addr, ring_size_dw, read_ptr_addr);

	printf("\t queue         = %" PRIu32 "\n", id);

	return id;
}

void KYTY_SYSV_ABI GraphicsUnmapComputeQueue(uint32_t id)
{
	PRINT_NAME();

	printf("\t id = %" PRIu32 "\n", id);

	GraphicsRunUnmapComputeQueue(id);
}

int KYTY_SYSV_ABI GraphicsComputeWaitOnAddress(uint32_t* cmd, uint64_t size, uint32_t* gpu_addr, uint32_t mask, uint32_t func, uint32_t ref)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 6);

	printf("\t cmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size        = %" PRIu64 "\n", size);
	printf("\t gpu_addr    = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(gpu_addr));
	printf("\t mask        = %08" PRIx32 "\n", mask);
	printf("\t func        = %" PRIu32 "\n", func);
	printf("\t ref         = %08" PRIx32 "\n", ref);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_DISPATCH_WAIT_MEM);
	cmd[1] = static_cast<uint32_t>(reinterpret_cast<uint64_t>(gpu_addr) & 0xffffffffu);
	cmd[2] = static_cast<uint32_t>((reinterpret_cast<uint64_t>(gpu_addr) >> 32u) & 0xffffffffu);
	cmd[3] = mask;
	cmd[4] = func;
	cmd[5] = ref;

	return OK;
}

void KYTY_SYSV_ABI GraphicsDingDong(uint32_t ring_id, uint32_t offset_dw)
{
	PRINT_NAME();

	printf("\t ring_id   = %" PRIu32 "\n", ring_id);
	printf("\t offset_dw = %" PRIu32 "\n", offset_dw);

	GraphicsRunDingDong(ring_id, offset_dw);
}

int KYTY_SYSV_ABI GraphicsInsertPushMarker(uint32_t* cmd, uint64_t size, const char* str)
{
	PRINT_NAME();

	auto len = strlen(str) + 1;

	EXIT_NOT_IMPLEMENTED(size * 4 < len + 1);

	printf("\t cmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size        = %" PRIu64 "\n", size);
	printf("\t str         = %s\n", str);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_PUSH_MARKER);

	memcpy(cmd + 1, str, len);

	return OK;
}

int KYTY_SYSV_ABI GraphicsInsertPopMarker(uint32_t* cmd, uint64_t size)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 2);

	printf("\t cmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size        = %" PRIu64 "\n", size);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_POP_MARKER);

	return OK;
}

uint64_t KYTY_SYSV_ABI GraphicsGetGpuCoreClockFrequency()
{
	return LibKernel::KernelGetTscFrequency();
}

int KYTY_SYSV_ABI GraphicsIsUserPaEnabled()
{
	return 0;
}

void* KYTY_SYSV_ABI GraphicsGetTheTessellationFactorRingBufferBaseAddress()
{
	PRINT_NAME();

	auto addr = Loader::VirtualMemory::AllocAligned(0, 0x20000, Loader::VirtualMemory::Mode::ReadWrite, 256);
	Loader::VirtualMemory::Free(addr);
	bool again = Loader::VirtualMemory::AllocFixed(addr, 0x20000, Loader::VirtualMemory::Mode::ReadWrite);
	EXIT_NOT_IMPLEMENTED(!again);
	Loader::VirtualMemory::Free(addr);

	printf("\t addr = %016" PRIx64 "\n", addr);

	return reinterpret_cast<void*>(addr);
}

int KYTY_SYSV_ABI GraphicsRegisterOwner(uint32_t* owner_handle, const char* name)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(owner_handle == nullptr);
	EXIT_NOT_IMPLEMENTED(name == nullptr);

	printf("\t RegisterOwner: %s\n", name);

	GpuMemoryRegisterOwner(owner_handle, name);

	printf("\t handler: %" PRIu32 "\n", *owner_handle);

	return OK;
}

int KYTY_SYSV_ABI GraphicsRegisterResource(uint32_t* resource_handle, uint32_t owner_handle, const void* memory, size_t size,
                                           const char* name, uint32_t type, uint64_t user_data)
{
	PRINT_NAME();

	// EXIT_NOT_IMPLEMENTED(resource_handle == nullptr);
	EXIT_NOT_IMPLEMENTED(memory == nullptr);
	EXIT_NOT_IMPLEMENTED(name == nullptr);

	printf("\t RegisterResource: %s\n", name);
	printf("\t owner_handle:     %" PRIu32 "\n", owner_handle);
	printf("\t addr:             %016" PRIx64 "\n", reinterpret_cast<uint64_t>(memory));
	printf("\t size:             %" PRIu64 "\n", size);
	printf("\t type:             %" PRIu32 "\n", type);
	printf("\t user_data:        %" PRIu64 "\n", user_data);

	uint32_t rhandle = 0;

	GpuMemoryRegisterResource(&rhandle, owner_handle, memory, size, name, type, user_data);

	printf("\t handler: %" PRIu32 "\n", rhandle);

	if (resource_handle != nullptr)
	{
		*resource_handle = rhandle;
	}

	return OK;
}

int KYTY_SYSV_ABI GraphicsUnregisterAllResourcesForOwner(uint32_t owner_handle)
{
	PRINT_NAME();

	printf("\t owner_handle:     %" PRIu32 "\n", owner_handle);

	GpuMemoryUnregisterAllResourcesForOwner(owner_handle);

	return OK;
}

int KYTY_SYSV_ABI GraphicsUnregisterOwnerAndResources(uint32_t owner_handle)
{
	PRINT_NAME();

	printf("\t owner_handle:     %" PRIu32 "\n", owner_handle);

	GpuMemoryUnregisterOwnerAndResources(owner_handle);

	return OK;
}

int KYTY_SYSV_ABI GraphicsUnregisterResource(uint32_t resource_handle)
{
	PRINT_NAME();

	printf("\t resource_handle:     %" PRIu32 "\n", resource_handle);

	GpuMemoryUnregisterResource(resource_handle);

	return OK;
}

} // namespace Gen4

namespace Gen5 {

LIB_NAME("Graphics5", "Graphics5");

struct ShaderRegister
{
	uint32_t offset;
	uint32_t value;
};

struct RegisterDefaultInfo
{
	uint32_t       type;
	ShaderRegister reg[16];
};

struct RegisterDefaults
{
	ShaderRegister** tbl0       = nullptr;
	ShaderRegister** tbl1       = nullptr;
	ShaderRegister** tbl2       = nullptr;
	ShaderRegister** tbl3       = nullptr;
	uint64_t         unknown[2] = {};
	uint32_t*        types      = nullptr;
	uint32_t         count      = 0;
};

struct ShaderSharp
{
	uint16_t offset_dw : 15;
	uint16_t size      : 1;
};

struct ShaderUserData
{
	uint16_t*    direct_resource_offset;
	ShaderSharp* sharp_resource_offset[4];
	uint16_t     eud_size_dw;
	uint16_t     srt_size_dw;
	uint16_t     direct_resource_count;
	uint16_t     sharp_resource_count[4];
};

struct ShaderRegisterRange
{
	uint16_t start;
	uint16_t end;
};

struct ShaderDrawModifier
{
	uint32_t enbl_start_vertex_offset   : 1;
	uint32_t enbl_start_index_offset    : 1;
	uint32_t enbl_start_instance_offset : 1;
	uint32_t enbl_draw_index            : 1;
	uint32_t enbl_user_vgprs            : 1;
	uint32_t render_target_slice_offset : 3;
	uint32_t fuse_draws                 : 1;
	uint32_t compiler_flags             : 23;
	uint32_t is_default                 : 1;
	uint32_t reserved                   : 31;
};

struct ShaderSpecialRegs
{
	ShaderRegister      ge_cntl;
	ShaderRegister      vgt_shader_stages_en;
	uint32_t            dispatch_modifier;
	ShaderRegisterRange user_data_range;
	ShaderDrawModifier  draw_modifier;
	ShaderRegister      vgt_gs_out_prim_type;
	ShaderRegister      ge_user_vgpr_en;
};

struct ShaderSemantic
{
	uint32_t semantic         : 8;
	uint32_t hardware_mapping : 8;
	uint32_t size_in_elements : 4;
	uint32_t is_f16           : 2;
	uint32_t is_flat_shaded   : 1;
	uint32_t is_linear        : 1;
	uint32_t is_custom        : 1;
	uint32_t static_vb_index  : 1;
	uint32_t static_attribute : 1;
	uint32_t reserved         : 1;
	uint32_t default_value    : 2;
	uint32_t default_value_hi : 2;
};

struct Shader
{
	uint32_t             file_header;
	uint32_t             version;
	ShaderUserData*      user_data;
	const volatile void* code;
	ShaderRegister*      cx_registers;
	ShaderRegister*      sh_registers;
	ShaderSpecialRegs*   specials;
	ShaderSemantic*      input_semantics;
	ShaderSemantic*      output_semantics;
	uint32_t             header_size;
	uint32_t             shader_size;
	uint32_t             embedded_constant_buffer_size_dqw;
	uint32_t             target;
	uint32_t             num_input_semantics;
	uint16_t             scratch_size_dw_per_thread;
	uint16_t             num_output_semantics;
	uint16_t             special_sizes_bytes;
	uint8_t              type;
	uint8_t              num_cx_registers;
	uint8_t              num_sh_registers;
};

struct CommandBuffer
{
	using Callback = KYTY_SYSV_ABI bool (*)(CommandBuffer*, uint32_t, void*);

	uint32_t* bottom;
	uint32_t* top;
	uint32_t* cursor_up;
	uint32_t* cursor_down;
	Callback  callback;
	void*     user_data;
	uint32_t  reserved_dw;

	void DbgDump() const
	{
		printf("\t bottom      = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(bottom));
		printf("\t top         = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(top));
		printf("\t cursor_up   = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(cursor_up));
		printf("\t cursor_down = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(cursor_down));
		printf("\t callback    = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(callback));
		printf("\t user_data   = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(user_data));
		printf("\t reserved_dw = %" PRIu32 "\n", reserved_dw);
	}

	[[nodiscard]] KYTY_SYSV_ABI uint32_t GetAvailableSizeDW() const
	{
		auto available = static_cast<uint32_t>(cursor_down - cursor_up);
		return std::max(available, reserved_dw) - reserved_dw;
	}

	KYTY_SYSV_ABI bool ReserveDW(uint32_t num_dw)
	{
		uint32_t remaining = GetAvailableSizeDW();
		if (num_dw > remaining)
		{
			bool result = callback(this, num_dw + reserved_dw, user_data);
			if (!result)
			{
				return false;
			}
			EXIT_NOT_IMPLEMENTED(!(GetAvailableSizeDW() >= num_dw));
		}
		return true;
	}

	KYTY_SYSV_ABI uint32_t* AllocateDW(uint32_t size_dw)
	{
		if (size_dw == 0 || !ReserveDW(size_dw))
		{
			return nullptr;
		}
		auto* ret_ptr = cursor_up;
		cursor_up += size_dw;
		return ret_ptr;
	}
};

static RegisterDefaultInfo g_reg_info0[] = {
    /* 0 */ {0x5f5a3e7b, {{Pm4::VGT_GS_OUT_PRIM_TYPE, 0x00000002}}},
    /* 1 */ {0x105971c2, {{Pm4::GE_CNTL, 0}}},
    /* 2 */ {0x40d49ad1, {{Pm4::GE_USER_VGPR_EN, 0}}},
    /* 3 */ {0x9ebfab10, {{Pm4::PRIMITIVE_TYPE, 0}}},
    /* 4 */ {0x48531062, {{Pm4::SPI_PS_INPUT_CNTL_0, 0}}},
};

static RegisterDefaultInfo g_reg_info1[] = {
    /* 0 - DepthRenderTarget */ {0x67096014,
                                 {{Pm4::DB_Z_INFO, 0},
                                  {Pm4::DB_STENCIL_INFO, 0},
                                  {Pm4::DB_Z_READ_BASE, 0},
                                  {Pm4::DB_STENCIL_READ_BASE, 0},
                                  {Pm4::DB_Z_WRITE_BASE, 0},
                                  {Pm4::DB_STENCIL_WRITE_BASE, 0},
                                  {Pm4::DB_Z_READ_BASE_HI, 0},
                                  {Pm4::DB_STENCIL_READ_BASE_HI, 0},
                                  {Pm4::DB_Z_WRITE_BASE_HI, 0},
                                  {Pm4::DB_STENCIL_WRITE_BASE_HI, 0},
                                  {Pm4::DB_HTILE_DATA_BASE_HI, 0},
                                  {Pm4::DB_DEPTH_VIEW, 0},
                                  {Pm4::DB_HTILE_DATA_BASE, 0},
                                  {Pm4::DB_DEPTH_SIZE_XY, 0},
                                  {Pm4::DB_DEPTH_CLEAR, 0},
                                  {Pm4::DB_STENCIL_CLEAR, 0}}},
    /* 1 - RenderTarget */ {0x38e92c91,
                            {{Pm4::CB_COLOR0_BASE, 0},
                             {Pm4::CB_COLOR0_VIEW, 0},
                             {Pm4::CB_COLOR0_INFO, 0},
                             {Pm4::CB_COLOR0_ATTRIB, 0},
                             {Pm4::CB_COLOR0_DCC_CONTROL, 0},
                             {Pm4::CB_COLOR0_CMASK, 0},
                             {Pm4::CB_COLOR0_FMASK, 0},
                             {Pm4::CB_COLOR0_CLEAR_WORD0, 0},
                             {Pm4::CB_COLOR0_CLEAR_WORD1, 0},
                             {Pm4::CB_COLOR0_DCC_BASE, 0},
                             {Pm4::CB_COLOR0_BASE_EXT, 0},
                             {Pm4::CB_COLOR0_CMASK_BASE_EXT, 0},
                             {Pm4::CB_COLOR0_FMASK_BASE_EXT, 0},
                             {Pm4::CB_COLOR0_DCC_BASE_EXT, 0},
                             {Pm4::CB_COLOR0_ATTRIB2, 0},
                             {Pm4::CB_COLOR0_ATTRIB3, 0}}}};

#define KYTY_ID(id, tbl) ((id)*4 + (tbl))
#define KYTY_INDEX0(id)  g_reg_info0[id].type, KYTY_ID(id, 0), 0
#define KYTY_INDEX1(id)  g_reg_info1[id].type, KYTY_ID(id, 1), 0
#define KYTY_REG0(id)    &g_reg_info0[id].reg[0]
#define KYTY_REG1(id)    &g_reg_info1[id].reg[0]

static ShaderRegister* g_tbl0[]      = {KYTY_REG0(0), KYTY_REG0(1), KYTY_REG0(2), KYTY_REG0(3), KYTY_REG0(4)};
static ShaderRegister* g_tbl1[]      = {KYTY_REG1(0), KYTY_REG1(1)};
static uint32_t        g_tbl_index[] = {KYTY_INDEX0(0), KYTY_INDEX0(1), KYTY_INDEX0(2), KYTY_INDEX0(3),
                                        KYTY_INDEX0(4), KYTY_INDEX1(0), KYTY_INDEX1(1)};

static RegisterDefaults g_reg_defaults = { // @suppress("Invalid arguments")
    g_tbl0, g_tbl1, nullptr, nullptr, {0, 0}, g_tbl_index, sizeof(g_tbl_index) / 12};

int KYTY_SYSV_ABI GraphicsInit(uint32_t* state, uint32_t ver)
{
	PRINT_NAME();

	printf("\t state = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(state));
	printf("\t ver   = %u\n", ver);

	EXIT_NOT_IMPLEMENTED(state == nullptr);
	EXIT_NOT_IMPLEMENTED(ver != 8);

	return OK;
}

void* KYTY_SYSV_ABI GraphicsGetRegisterDefaults2(uint32_t ver)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(ver != 8);
	EXIT_NOT_IMPLEMENTED(offsetof(RegisterDefaults, count) != 0x38);

	// g_reg_defaults.count = 0;

	return &g_reg_defaults;
}

void* KYTY_SYSV_ABI GraphicsGetRegisterDefaults2Internal(uint32_t ver)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(ver != 8);
	EXIT_NOT_IMPLEMENTED(offsetof(RegisterDefaults, count) != 0x38);

	g_reg_defaults.count = 0;

	return &g_reg_defaults;
}

static void dbg_dump_shader(const Shader* h)
{
	printf("\t file_header  = 0x%08" PRIx32 "\n", h->file_header);
	printf("\t version      = 0x%08" PRIx32 "\n", h->version);
	printf("\t user_data    = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(h->user_data));
	if (h->user_data != nullptr)
	{
		printf("\t\t direct_resource_count    = 0x%04" PRIx16 "\n", h->user_data->direct_resource_count);
		printf("\t\t direct_resource_offset   = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(h->user_data->direct_resource_offset));
		for (int i = 0; i < static_cast<int>(h->user_data->direct_resource_count); i++)
		{
			printf("\t\t\t offset[%02d] = %04" PRIx16 "\n", i, h->user_data->direct_resource_offset[i]);
		}
		for (int imm = 0; imm < 4; imm++)
		{
			printf("\t\t sharp_resource_count  [%d] = 0x%04" PRIx16 "\n", imm, h->user_data->sharp_resource_count[imm]);
			printf("\t\t sharp_resource_offset [%d] = 0x%016" PRIx64 "\n", imm,
			       reinterpret_cast<uint64_t>(h->user_data->sharp_resource_offset[imm]));
			for (int i = 0; i < static_cast<int>(h->user_data->sharp_resource_count[imm]); i++)
			{
				printf("\t\t\t offset_dw[%d] = %04" PRIx16 ", size = %" PRIu16 "\n", i,
				       h->user_data->sharp_resource_offset[imm][i].offset_dw, h->user_data->sharp_resource_offset[imm][i].size);
			}
		}
		printf("\t\t eud_size_dw    = 0x%04" PRIx16 "\n", h->user_data->eud_size_dw);
		printf("\t\t srt_size_dw    = 0x%04" PRIx16 "\n", h->user_data->srt_size_dw);
	}
	printf("\t code             = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(h->code));
	printf("\t num_cx_registers = 0x%02" PRIx8 "\n", h->num_cx_registers);
	printf("\t cx_registers     = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(h->cx_registers));
	for (int i = 0; i < static_cast<int>(h->num_cx_registers); i++)
	{
		printf("\t\t cx[%d]: offset = %08" PRIx32 ", value = %08" PRIx32 "\n", i, h->cx_registers[i].offset, h->cx_registers[i].value);
	}
	printf("\t num_sh_registers = 0x%02" PRIx8 "\n", h->num_sh_registers);
	printf("\t sh_registers     = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(h->sh_registers));
	for (int i = 0; i < static_cast<int>(h->num_sh_registers); i++)
	{
		printf("\t\t sh[%d]: offset = %08" PRIx32 ", value = %08" PRIx32 "\n", i, h->sh_registers[i].offset, h->sh_registers[i].value);
	}
	printf("\t specials                          = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(h->specials));
	printf("\t\t ge_cntl:              offset = %08" PRIx32 ", value = %08" PRIx32 "\n", h->specials->ge_cntl.offset,
	       h->specials->ge_cntl.value);
	printf("\t\t vgt_shader_stages_en: offset = %08" PRIx32 ", value = %08" PRIx32 "\n", h->specials->vgt_shader_stages_en.offset,
	       h->specials->vgt_shader_stages_en.value);
	printf("\t\t vgt_gs_out_prim_type: offset = %08" PRIx32 ", value = %08" PRIx32 "\n", h->specials->vgt_gs_out_prim_type.offset,
	       h->specials->vgt_gs_out_prim_type.value);
	printf("\t\t ge_user_vgpr_en:      offset = %08" PRIx32 ", value = %08" PRIx32 "\n", h->specials->ge_user_vgpr_en.offset,
	       h->specials->ge_user_vgpr_en.value);
	printf("\t\t dispatch_modifier = %08" PRIx32 "\n", h->specials->dispatch_modifier);
	printf("\t\t user_data_range: start = %08" PRIx32 ", end = %08" PRIx32 "\n", h->specials->user_data_range.start,
	       h->specials->user_data_range.end);
	printf("\t\t draw_modifier: enbl_start_vertex_offset   = %08" PRIx32 "\n", h->specials->draw_modifier.enbl_start_vertex_offset);
	printf("\t\t draw_modifier: enbl_start_index_offset    = %08" PRIx32 "\n", h->specials->draw_modifier.enbl_start_index_offset);
	printf("\t\t draw_modifier: enbl_start_instance_offset = %08" PRIx32 "\n", h->specials->draw_modifier.enbl_start_instance_offset);
	printf("\t\t draw_modifier: enbl_draw_index            = %08" PRIx32 "\n", h->specials->draw_modifier.enbl_draw_index);
	printf("\t\t draw_modifier: enbl_user_vgprs            = %08" PRIx32 "\n", h->specials->draw_modifier.enbl_user_vgprs);
	printf("\t\t draw_modifier: render_target_slice_offset = %08" PRIx32 "\n", h->specials->draw_modifier.render_target_slice_offset);
	printf("\t\t draw_modifier: fuse_draws                 = %08" PRIx32 "\n", h->specials->draw_modifier.fuse_draws);
	printf("\t\t draw_modifier: compiler_flags             = %08" PRIx32 "\n", h->specials->draw_modifier.compiler_flags);
	printf("\t\t draw_modifier: is_default                 = %08" PRIx32 "\n", h->specials->draw_modifier.is_default);
	printf("\t\t draw_modifier: reserved                   = %08" PRIx32 "\n", h->specials->draw_modifier.reserved);
	printf("\t num_input_semantics               = 0x%08" PRIx32 "\n", h->num_input_semantics);
	printf("\t input_semantics                   = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(h->input_semantics));
	for (int i = 0; i < static_cast<int>(h->num_input_semantics); i++)
	{
		printf("\t\t input_semantics[%d]: semantic         = %08" PRIx32 "\n", i, h->input_semantics[i].semantic);
		printf("\t\t input_semantics[%d]: hardware_mapping = %08" PRIx32 "\n", i, h->input_semantics[i].hardware_mapping);
		printf("\t\t input_semantics[%d]: size_in_elements = %08" PRIx32 "\n", i, h->input_semantics[i].size_in_elements);
		printf("\t\t input_semantics[%d]: is_f16           = %08" PRIx32 "\n", i, h->input_semantics[i].is_f16);
		printf("\t\t input_semantics[%d]: is_flat_shaded   = %08" PRIx32 "\n", i, h->input_semantics[i].is_flat_shaded);
		printf("\t\t input_semantics[%d]: is_linear        = %08" PRIx32 "\n", i, h->input_semantics[i].is_linear);
		printf("\t\t input_semantics[%d]: is_custom        = %08" PRIx32 "\n", i, h->input_semantics[i].is_custom);
		printf("\t\t input_semantics[%d]: static_vb_index  = %08" PRIx32 "\n", i, h->input_semantics[i].static_vb_index);
		printf("\t\t input_semantics[%d]: static_attribute = %08" PRIx32 "\n", i, h->input_semantics[i].static_attribute);
		printf("\t\t input_semantics[%d]: reserved         = %08" PRIx32 "\n", i, h->input_semantics[i].reserved);
		printf("\t\t input_semantics[%d]: default_value    = %08" PRIx32 "\n", i, h->input_semantics[i].default_value);
		printf("\t\t input_semantics[%d]: default_value_hi = %08" PRIx32 "\n", i, h->input_semantics[i].default_value_hi);
	}
	printf("\t num_output_semantics              = 0x%04" PRIx16 "\n", h->num_output_semantics);
	printf("\t output_semantics                  = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(h->output_semantics));
	for (int i = 0; i < static_cast<int>(h->num_output_semantics); i++)
	{
		printf("\t\t output_semantics[%d]: semantic         = %08" PRIx32 "\n", i, h->output_semantics[i].semantic);
		printf("\t\t output_semantics[%d]: hardware_mapping = %08" PRIx32 "\n", i, h->output_semantics[i].hardware_mapping);
		printf("\t\t output_semantics[%d]: size_in_elements = %08" PRIx32 "\n", i, h->output_semantics[i].size_in_elements);
		printf("\t\t output_semantics[%d]: is_f16           = %08" PRIx32 "\n", i, h->output_semantics[i].is_f16);
		printf("\t\t output_semantics[%d]: is_flat_shaded   = %08" PRIx32 "\n", i, h->output_semantics[i].is_flat_shaded);
		printf("\t\t output_semantics[%d]: is_linear        = %08" PRIx32 "\n", i, h->output_semantics[i].is_linear);
		printf("\t\t output_semantics[%d]: is_custom        = %08" PRIx32 "\n", i, h->output_semantics[i].is_custom);
		printf("\t\t output_semantics[%d]: static_vb_index  = %08" PRIx32 "\n", i, h->output_semantics[i].static_vb_index);
		printf("\t\t output_semantics[%d]: static_attribute = %08" PRIx32 "\n", i, h->output_semantics[i].static_attribute);
		printf("\t\t output_semantics[%d]: reserved         = %08" PRIx32 "\n", i, h->output_semantics[i].reserved);
		printf("\t\t output_semantics[%d]: default_value    = %08" PRIx32 "\n", i, h->output_semantics[i].default_value);
		printf("\t\t output_semantics[%d]: default_value_hi = %08" PRIx32 "\n", i, h->output_semantics[i].default_value_hi);
	}
	printf("\t header_size                       = 0x%08" PRIx32 "\n", h->header_size);
	printf("\t shader_size                       = 0x%08" PRIx32 "\n", h->shader_size);
	printf("\t embedded_constant_buffer_size_dqw = 0x%08" PRIx32 "\n", h->embedded_constant_buffer_size_dqw);
	printf("\t target                            = 0x%08" PRIx32 "\n", h->target);
	printf("\t scratch_size_dw_per_thread        = 0x%04" PRIx16 "\n", h->scratch_size_dw_per_thread);
	printf("\t special_sizes_bytes               = 0x%04" PRIx16 "\n", h->special_sizes_bytes);
	printf("\t type                              = 0x%02" PRIx8 "\n", h->type);
}

int KYTY_SYSV_ABI GraphicsCreateShader(Shader** dst, void* header, const volatile void* code)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(dst == nullptr);
	EXIT_NOT_IMPLEMENTED(header == nullptr);
	EXIT_NOT_IMPLEMENTED(code == nullptr);

	printf("\t header = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(header));
	printf("\t code   = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(code));

	auto* h = static_cast<Shader*>(header);

	auto update_addr = [](auto& m)
	{
		if (m != nullptr)
		{
			m = reinterpret_cast<typename std::remove_reference<decltype(m)>::type>(reinterpret_cast<uintptr_t>(m) +
			                                                                        reinterpret_cast<uintptr_t>(&m));
		}
	};

	update_addr(h->cx_registers);
	update_addr(h->sh_registers);
	update_addr(h->user_data);
	update_addr(h->specials);
	update_addr(h->input_semantics);
	update_addr(h->output_semantics);
	update_addr(h->user_data->direct_resource_offset);
	update_addr(h->user_data->sharp_resource_offset[0]);
	update_addr(h->user_data->sharp_resource_offset[1]);
	update_addr(h->user_data->sharp_resource_offset[2]);
	update_addr(h->user_data->sharp_resource_offset[3]);

	h->code = code;

	dbg_dump_shader(h);

	EXIT_NOT_IMPLEMENTED(h->file_header != 0x34333231);
	EXIT_NOT_IMPLEMENTED(h->version != 0x00000018);

	*dst = h;

	return OK;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbResetQueue(CommandBuffer* buf, uint32_t op, uint32_t state)
{
	PRINT_NAME();

	printf("\t op    = 0x%08" PRIx32 "\n", op);
	printf("\t state = 0x%08" PRIx32 "\n", state);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);
	EXIT_NOT_IMPLEMENTED(op != 0x3ff);
	EXIT_NOT_IMPLEMENTED(state != 0);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(2);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(2, Pm4::IT_NOP, Pm4::R_DRAW_RESET);
	cmd[1] = 0;

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbWaitUntilSafeForRendering(CommandBuffer* buf, uint32_t video_out_handle, uint32_t display_buffer_index)
{
	PRINT_NAME();

	printf("\t video_out_handle     = %" PRIu32 "\n", video_out_handle);
	printf("\t display_buffer_index = %" PRIu32 "\n", display_buffer_index);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(3);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(3, Pm4::IT_NOP, Pm4::R_WAIT_FLIP_DONE);
	cmd[1] = video_out_handle;
	cmd[2] = display_buffer_index;

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbSetCxRegistersIndirect(CommandBuffer* buf, const volatile ShaderRegister* regs, uint32_t num_regs)
{
	PRINT_NAME();

	printf("\t regs     = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(regs));
	printf("\t num_regs = %" PRIu32 "\n", num_regs);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(4);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	auto vaddr = reinterpret_cast<uint64_t>(regs);

	cmd[0] = KYTY_PM4(4, Pm4::IT_NOP, Pm4::R_CX_REGS);
	cmd[1] = num_regs;
	cmd[2] = vaddr & 0xffffffffu;
	cmd[3] = (vaddr >> 32u) & 0xffffffffu;

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbSetShRegistersIndirect(CommandBuffer* buf, const volatile ShaderRegister* regs, uint32_t num_regs)
{
	PRINT_NAME();

	printf("\t regs     = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(regs));
	printf("\t num_regs = %" PRIu32 "\n", num_regs);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(4);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	auto vaddr = reinterpret_cast<uint64_t>(regs);

	cmd[0] = KYTY_PM4(4, Pm4::IT_NOP, Pm4::R_SH_REGS);
	cmd[1] = num_regs;
	cmd[2] = vaddr & 0xffffffffu;
	cmd[3] = (vaddr >> 32u) & 0xffffffffu;

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbSetUcRegistersIndirect(CommandBuffer* buf, const volatile ShaderRegister* regs, uint32_t num_regs)
{
	PRINT_NAME();

	printf("\t regs     = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(regs));
	printf("\t num_regs = %" PRIu32 "\n", num_regs);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(4);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	auto vaddr = reinterpret_cast<uint64_t>(regs);

	cmd[0] = KYTY_PM4(4, Pm4::IT_NOP, Pm4::R_UC_REGS);
	cmd[1] = num_regs;
	cmd[2] = vaddr & 0xffffffffu;
	cmd[3] = (vaddr >> 32u) & 0xffffffffu;

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbSetIndexSize(CommandBuffer* buf, uint8_t index_size, uint8_t cache_policy)
{
	PRINT_NAME();

	printf("\t index_size   = 0x%" PRIx8 "\n", index_size);
	printf("\t cache_policy = 0x%" PRIx8 "\n", cache_policy);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);
	EXIT_NOT_IMPLEMENTED(cache_policy != 0);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(2);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(2, Pm4::IT_INDEX_TYPE, 0u);
	cmd[1] = index_size;

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbDrawIndexAuto(CommandBuffer* buf, uint32_t index_count, uint64_t modifier)
{
	PRINT_NAME();

	printf("\t index_count = 0x%" PRIx32 "\n", index_count);
	printf("\t modifier    = 0x%016" PRIx64 "\n", modifier);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);
	EXIT_NOT_IMPLEMENTED(modifier != 0x40000000);

	// auto *m = reinterpret_cast<ShaderDrawModifier*>(&modifier);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(7);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(7, Pm4::IT_NOP, Pm4::R_DRAW_INDEX_AUTO);
	cmd[1] = index_count;
	cmd[2] = 0;

	return cmd;
}

int KYTY_SYSV_ABI GraphicsSetCxRegIndirectPatchSetAddress(uint32_t* cmd, const volatile ShaderRegister* regs)
{
	PRINT_NAME();

	printf("\t cmd  = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t regs = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(regs));

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);
	EXIT_NOT_IMPLEMENTED(regs == nullptr);

	auto vaddr = reinterpret_cast<uint64_t>(regs);

	cmd[2] = vaddr & 0xffffffffu;
	cmd[3] = (vaddr >> 32u) & 0xffffffffu;

	return OK;
}

int KYTY_SYSV_ABI GraphicsSetShRegIndirectPatchSetAddress(uint32_t* cmd, const volatile ShaderRegister* regs)
{
	PRINT_NAME();

	printf("\t cmd  = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t regs = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(regs));

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);
	EXIT_NOT_IMPLEMENTED(regs == nullptr);

	auto vaddr = reinterpret_cast<uint64_t>(regs);

	cmd[2] = vaddr & 0xffffffffu;
	cmd[3] = (vaddr >> 32u) & 0xffffffffu;

	return OK;
}

int KYTY_SYSV_ABI GraphicsSetUcRegIndirectPatchSetAddress(uint32_t* cmd, const volatile ShaderRegister* regs)
{
	PRINT_NAME();

	printf("\t cmd  = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t regs = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(regs));

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);
	EXIT_NOT_IMPLEMENTED(regs == nullptr);

	auto vaddr = reinterpret_cast<uint64_t>(regs);

	cmd[2] = vaddr & 0xffffffffu;
	cmd[3] = (vaddr >> 32u) & 0xffffffffu;

	return OK;
}

int KYTY_SYSV_ABI GraphicsSetCxRegIndirectPatchAddRegisters(uint32_t* cmd, uint32_t num_regs)
{
	PRINT_NAME();

	printf("\t cmd      = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t num_regs = %" PRIu32 "\n", num_regs);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[1] += num_regs;

	return OK;
}

int KYTY_SYSV_ABI GraphicsSetShRegIndirectPatchAddRegisters(uint32_t* cmd, uint32_t num_regs)
{
	PRINT_NAME();

	printf("\t cmd      = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t num_regs = %" PRIu32 "\n", num_regs);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[1] += num_regs;

	return OK;
}

int KYTY_SYSV_ABI GraphicsSetUcRegIndirectPatchAddRegisters(uint32_t* cmd, uint32_t num_regs)
{
	PRINT_NAME();

	printf("\t cmd      = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t num_regs = %" PRIu32 "\n", num_regs);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[1] += num_regs;

	return OK;
}

int KYTY_SYSV_ABI GraphicsCreatePrimState(ShaderRegister* cx_regs, ShaderRegister* uc_regs, const Shader* hs, const Shader* gs,
                                          uint32_t prim_type)
{
	PRINT_NAME();

	printf("\t cx_regs   = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(cx_regs));
	printf("\t uc_regs   = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(uc_regs));
	printf("\t hs        = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(hs));
	printf("\t gs        = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(gs));
	printf("\t prim_type = %" PRIu32 "\n", prim_type);

	EXIT_NOT_IMPLEMENTED(hs != nullptr);
	EXIT_NOT_IMPLEMENTED(gs == nullptr);
	EXIT_NOT_IMPLEMENTED(cx_regs == nullptr);
	EXIT_NOT_IMPLEMENTED(uc_regs == nullptr);

	EXIT_NOT_IMPLEMENTED(gs->type != 2);
	EXIT_NOT_IMPLEMENTED(gs->specials->vgt_shader_stages_en.offset != Pm4::VGT_SHADER_STAGES_EN);
	EXIT_NOT_IMPLEMENTED(gs->specials->vgt_gs_out_prim_type.offset != Pm4::VGT_GS_OUT_PRIM_TYPE);
	EXIT_NOT_IMPLEMENTED(gs->specials->ge_cntl.offset != Pm4::GE_CNTL);
	EXIT_NOT_IMPLEMENTED(gs->specials->ge_user_vgpr_en.offset != Pm4::GE_USER_VGPR_EN);

	cx_regs[0] = gs->specials->vgt_shader_stages_en;
	cx_regs[1] = gs->specials->vgt_gs_out_prim_type;

	uc_regs[0]        = gs->specials->ge_cntl;
	uc_regs[1]        = gs->specials->ge_user_vgpr_en;
	uc_regs[2].offset = Pm4::PRIMITIVE_TYPE;
	uc_regs[2].value  = 0;

	return OK;
}

int KYTY_SYSV_ABI GraphicsCreateInterpolantMapping(ShaderRegister* regs, const Shader* gs, const Shader* ps)
{
	PRINT_NAME();

	printf("\t regs = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(regs));
	printf("\t gs   = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(gs));
	printf("\t ps   = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(ps));

	EXIT_NOT_IMPLEMENTED(gs == nullptr);
	EXIT_NOT_IMPLEMENTED(gs == nullptr && ps == nullptr);
	EXIT_NOT_IMPLEMENTED(regs == nullptr);

	EXIT_NOT_IMPLEMENTED(gs->type != 2);
	EXIT_NOT_IMPLEMENTED(ps != nullptr && ps->type != 1);

	EXIT_NOT_IMPLEMENTED(sizeof(ShaderSemantic) != 4);
	EXIT_NOT_IMPLEMENTED(ps != nullptr && gs->num_output_semantics != ps->num_input_semantics);

	auto* out32 = reinterpret_cast<uint32_t*>(gs->output_semantics);
	auto* in32  = (ps != nullptr ? reinterpret_cast<uint32_t*>(ps->input_semantics) : nullptr);

	for (int i = 0; i < 32; i++)
	{
		regs[i].offset = Pm4::SPI_PS_INPUT_CNTL_0 + i;
		regs[i].value  = 0;

		if (i < static_cast<int>(gs->num_output_semantics))
		{
			EXIT_NOT_IMPLEMENTED(out32[i] != 0x0000000f);
			EXIT_NOT_IMPLEMENTED(in32 != nullptr && out32[i] != in32[i]);
			regs[i].value = i;
		}
	}

	return OK;
}

uint32_t* KYTY_SYSV_ABI GraphicsCbSetShRegisterRangeDirect(CommandBuffer* buf, uint32_t offset, const uint32_t* values, uint32_t num_values)
{
	PRINT_NAME();

	printf("\t buf        = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(buf));
	printf("\t offset     = %" PRIx32 "\n", offset);
	printf("\t values     = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(values));
	printf("\t num_values = %" PRIu32 "\n", num_values);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);
	EXIT_NOT_IMPLEMENTED(num_values == 0);
	EXIT_NOT_IMPLEMENTED(offset == 0);
	EXIT_NOT_IMPLEMENTED(offset > 0x3ffu);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(num_values + 2);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(num_values + 2, Pm4::IT_SET_SH_REG, 0u);
	cmd[1] = offset;

	if (values == nullptr)
	{
		memset(cmd + 2, 0, static_cast<size_t>(num_values) * 4);
	} else
	{
		memcpy(cmd + 2, values, static_cast<size_t>(num_values) * 4);
	}

	return cmd;
}

int KYTY_SYSV_ABI GraphicsGetDataPacketPayloadAddress(uint32_t** addr, uint32_t* cmd, int type)
{
	PRINT_NAME();

	printf("\t addr = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(addr));
	printf("\t cmd  = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t type = %d\n", type);

	EXIT_NOT_IMPLEMENTED(addr == nullptr);
	EXIT_NOT_IMPLEMENTED(cmd == nullptr);
	EXIT_NOT_IMPLEMENTED(type != 1);

	*addr = cmd + 2;

	return OK;
}

} // namespace Gen5

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
