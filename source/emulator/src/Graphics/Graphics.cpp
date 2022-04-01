#include "Emulator/Graphics/Graphics.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/String.h"

#include "Emulator/Config.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/GraphicsRun.h"
#include "Emulator/Graphics/HardwareContext.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"
#include "Emulator/Graphics/Objects/Label.h"
#include "Emulator/Graphics/Pm4.h"
#include "Emulator/Graphics/Tile.h"
#include "Emulator/Graphics/VideoOut.h"
#include "Emulator/Graphics/Window.h"
#include "Emulator/Kernel/Pthread.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#include <atomic>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

LIB_NAME("GraphicsDriver", "GraphicsDriver");

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
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Graphics) {}

KYTY_SUBSYSTEM_DESTROY(Graphics) {}

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
		memcpy(&cmd[1], ps_regs, 12 * 4);
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
		memcpy(&cmd[1], ps_regs, 12 * 4);
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
	memcpy(&cmd[1], ps_regs, 12 * 4);

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
	memcpy(&cmd[2], cs_regs, 7 * 4);

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

	printf("\tcmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\tsize        = %" PRIu64 "\n", size);
	printf("\tindex_count = %" PRIu32 "\n", index_count);
	printf("\tflags       = %08" PRIx32 "\n", flags);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_DRAW_INDEX_AUTO);
	cmd[1] = index_count;
	cmd[2] = flags;

	return OK;
}

int KYTY_SYSV_ABI GraphicsInsertWaitFlipDone(uint32_t* cmd, uint64_t size, uint32_t video_out_handle, uint32_t display_buffer_index)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < 3);

	printf("\tcmd_buffer           = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\tsize                 = %" PRIu64 "\n", size);
	printf("\tvideo_out_handle     = %" PRIu32 "\n", video_out_handle);
	printf("\tdisplay_buffer_index = %" PRIu32 "\n", display_buffer_index);

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

	GpuMemoryFlush(WindowGetGraphicContext());
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

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
