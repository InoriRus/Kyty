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
#include "Emulator/Graphics/Shader.h"
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
	ShaderInit();
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

int KYTY_SYSV_ABI GraphicsSetVsShader(uint32_t* cmd, uint64_t size, const uint32_t* vs_regs, uint32_t shader_modifier)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < sizeof(HW::VsStageRegisters) / 4 + 2);

	printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size            = %" PRIu64 "\n", size);
	printf("\t shader_modifier = %" PRIu32 "\n", shader_modifier);

	printf("\t m_spiShaderPgmLoVs    = %08" PRIx32 "\n", vs_regs[0]);
	printf("\t m_spiShaderPgmHiVs    = %08" PRIx32 "\n", vs_regs[1]);
	printf("\t m_spiShaderPgmRsrc1Vs = %08" PRIx32 "\n", vs_regs[2]);
	printf("\t m_spiShaderPgmRsrc2Vs = %08" PRIx32 "\n", vs_regs[3]);
	printf("\t m_spiVsOutConfig      = %08" PRIx32 "\n", vs_regs[4]);
	printf("\t m_spiShaderPosFormat  = %08" PRIx32 "\n", vs_regs[5]);
	printf("\t m_paClVsOutCntl       = %08" PRIx32 "\n", vs_regs[6]);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_VS);
	cmd[1] = shader_modifier;
	memcpy(&cmd[2], vs_regs, static_cast<size_t>(7) * 4);

	return OK;
}

int KYTY_SYSV_ABI GraphicsUpdateVsShader(uint32_t* cmd, uint64_t size, const uint32_t* vs_regs, uint32_t shader_modifier)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(size < sizeof(HW::VsStageRegisters) / 4 + 2);

	printf("\t cmd_buffer      = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size            = %" PRIu64 "\n", size);
	printf("\t shader_modifier = %" PRIu32 "\n", shader_modifier);

	printf("\t m_spiShaderPgmLoVs    = %08" PRIx32 "\n", vs_regs[0]);
	printf("\t m_spiShaderPgmHiVs    = %08" PRIx32 "\n", vs_regs[1]);
	printf("\t m_spiShaderPgmRsrc1Vs = %08" PRIx32 "\n", vs_regs[2]);
	printf("\t m_spiShaderPgmRsrc2Vs = %08" PRIx32 "\n", vs_regs[3]);
	printf("\t m_spiVsOutConfig      = %08" PRIx32 "\n", vs_regs[4]);
	printf("\t m_spiShaderPosFormat  = %08" PRIx32 "\n", vs_regs[5]);
	printf("\t m_paClVsOutCntl       = %08" PRIx32 "\n", vs_regs[6]);

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_VS_UPDATE);
	cmd[1] = shader_modifier;
	memcpy(&cmd[2], vs_regs, static_cast<size_t>(7) * 4);

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

		printf("\t m_spiShaderPgmLoPs    = %08" PRIx32 "\n", ps_regs[0]);
		printf("\t m_spiShaderPgmHiPs    = %08" PRIx32 "\n", ps_regs[1]);
		printf("\t m_spiShaderPgmRsrc1Ps = %08" PRIx32 "\n", ps_regs[2]);
		printf("\t m_spiShaderPgmRsrc2Ps = %08" PRIx32 "\n", ps_regs[3]);
		printf("\t m_spiShaderZFormat    = %08" PRIx32 "\n", ps_regs[4]);
		printf("\t m_spiShaderColFormat  = %08" PRIx32 "\n", ps_regs[5]);
		printf("\t m_spiPsInputEna       = %08" PRIx32 "\n", ps_regs[6]);
		printf("\t m_spiPsInputAddr      = %08" PRIx32 "\n", ps_regs[7]);
		printf("\t m_spiPsInControl      = %08" PRIx32 "\n", ps_regs[8]);
		printf("\t m_spiBarycCntl        = %08" PRIx32 "\n", ps_regs[9]);
		printf("\t m_dbShaderControl     = %08" PRIx32 "\n", ps_regs[10]);
		printf("\t m_cbShaderMask        = %08" PRIx32 "\n", ps_regs[11]);

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

		printf("\t m_spiShaderPgmLoPs    = %08" PRIx32 "\n", ps_regs[0]);
		printf("\t m_spiShaderPgmHiPs    = %08" PRIx32 "\n", ps_regs[1]);
		printf("\t m_spiShaderPgmRsrc1Ps = %08" PRIx32 "\n", ps_regs[2]);
		printf("\t m_spiShaderPgmRsrc2Ps = %08" PRIx32 "\n", ps_regs[3]);
		printf("\t m_spiShaderZFormat    = %08" PRIx32 "\n", ps_regs[4]);
		printf("\t m_spiShaderColFormat  = %08" PRIx32 "\n", ps_regs[5]);
		printf("\t m_spiPsInputEna       = %08" PRIx32 "\n", ps_regs[6]);
		printf("\t m_spiPsInputAddr      = %08" PRIx32 "\n", ps_regs[7]);
		printf("\t m_spiPsInControl      = %08" PRIx32 "\n", ps_regs[8]);
		printf("\t m_spiBarycCntl        = %08" PRIx32 "\n", ps_regs[9]);
		printf("\t m_dbShaderControl     = %08" PRIx32 "\n", ps_regs[10]);
		printf("\t m_cbShaderMask        = %08" PRIx32 "\n", ps_regs[11]);

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

	printf("\t m_spiShaderPgmLoPs    = %08" PRIx32 "\n", ps_regs[0]);
	printf("\t m_spiShaderPgmHiPs    = %08" PRIx32 "\n", ps_regs[1]);
	printf("\t m_spiShaderPgmRsrc1Ps = %08" PRIx32 "\n", ps_regs[2]);
	printf("\t m_spiShaderPgmRsrc2Ps = %08" PRIx32 "\n", ps_regs[3]);
	printf("\t m_spiShaderZFormat    = %08" PRIx32 "\n", ps_regs[4]);
	printf("\t m_spiShaderColFormat  = %08" PRIx32 "\n", ps_regs[5]);
	printf("\t m_spiPsInputEna       = %08" PRIx32 "\n", ps_regs[6]);
	printf("\t m_spiPsInputAddr      = %08" PRIx32 "\n", ps_regs[7]);
	printf("\t m_spiPsInControl      = %08" PRIx32 "\n", ps_regs[8]);
	printf("\t m_spiBarycCntl        = %08" PRIx32 "\n", ps_regs[9]);
	printf("\t m_dbShaderControl     = %08" PRIx32 "\n", ps_regs[10]);
	printf("\t m_cbShaderMask        = %08" PRIx32 "\n", ps_regs[11]);

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

	printf("\t m_spiShaderPgmLoPs    = %08" PRIx32 "\n", ps_regs[0]);
	printf("\t m_spiShaderPgmHiPs    = %08" PRIx32 "\n", ps_regs[1]);
	printf("\t m_spiShaderPgmRsrc1Ps = %08" PRIx32 "\n", ps_regs[2]);
	printf("\t m_spiShaderPgmRsrc2Ps = %08" PRIx32 "\n", ps_regs[3]);
	printf("\t m_spiShaderZFormat    = %08" PRIx32 "\n", ps_regs[4]);
	printf("\t m_spiShaderColFormat  = %08" PRIx32 "\n", ps_regs[5]);
	printf("\t m_spiPsInputEna       = %08" PRIx32 "\n", ps_regs[6]);
	printf("\t m_spiPsInputAddr      = %08" PRIx32 "\n", ps_regs[7]);
	printf("\t m_spiPsInControl      = %08" PRIx32 "\n", ps_regs[8]);
	printf("\t m_spiBarycCntl        = %08" PRIx32 "\n", ps_regs[9]);
	printf("\t m_dbShaderControl     = %08" PRIx32 "\n", ps_regs[10]);
	printf("\t m_cbShaderMask        = %08" PRIx32 "\n", ps_regs[11]);

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

	printf("\t m_computePgmLo      = %08" PRIx32 "\n", cs_regs[0]);
	printf("\t m_computePgmHi      = %08" PRIx32 "\n", cs_regs[1]);
	printf("\t m_computePgmRsrc1   = %08" PRIx32 "\n", cs_regs[2]);
	printf("\t m_computePgmRsrc2   = %08" PRIx32 "\n", cs_regs[3]);
	printf("\t m_computeNumThreadX = %08" PRIx32 "\n", cs_regs[4]);
	printf("\t m_computeNumThreadY = %08" PRIx32 "\n", cs_regs[5]);
	printf("\t m_computeNumThreadZ = %08" PRIx32 "\n", cs_regs[6]);

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

	printf("\t cmd_buffer  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(cmd));
	printf("\t size        = %" PRIu64 "\n", size);
	printf("\t index_count = %" PRIu32 "\n", index_count);
	printf("\t index_addr  = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(index_addr));
	printf("\t flags       = %08" PRIx32 "\n", flags);
	printf("\t type        = %" PRIu32 "\n", type);

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

	cmd[0] = KYTY_PM4(size, Pm4::IT_NOP, Pm4::R_WAIT_MEM_32);
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

struct Label
{
	volatile uint64_t m_value;
	uint64_t          m_reserved[3];
};

static RegisterDefaultInfo g_cx_reg_info1[] = {
    /* 0 */ {0xE24F806D, {{Pm4::CB_COLOR_CONTROL, 0x00cc0010}}},
    /* 1 */ {0xF6C28182, {{Pm4::CB_DCC_CONTROL, 0x00000000}}},
    /* 2 */ {0x6F6E55A5, {{Pm4::CB_RMI_GL2_CACHE_CONTROL, 0x00000000}}},
    /* 3 */ {0x0BC65DA4, {{Pm4::CB_SHADER_MASK, 0x00000000}}},
    /* 4 */ {0x9E5AD592, {{Pm4::CB_TARGET_MASK, 0x00000000}}},
    /* 5 */ {0xBB513B98, {{Pm4::DB_ALPHA_TO_MASK, 0x0000aa00}}},
    /* 6 */ {0xAB64B23B, {{Pm4::DB_COUNT_CONTROL, 0x00000000}}},
    /* 7 */ {0x53C39964, {{Pm4::DB_DEPTH_CONTROL, 0x00000000}}},
    /* 8 */ {0x01396B11, {{Pm4::DB_EQAA, 0x00000000}}},
    /* 9 */ {0x7D42019A, {{Pm4::DB_RENDER_CONTROL, 0x00000000}}},
    /* 10 */ {0x3548F523, {{Pm4::PS_SHADER_SAMPLE_EXCLUSION_MASK, 0x00000000}}},
    /* 11 */ {0xF43AD28A, {{Pm4::DB_RMI_L2_CACHE_CONTROL, 0x00000000}}},
    /* 12 */ {0x6DE4C312, {{Pm4::DB_SHADER_CONTROL, 0x00000000}}},
    /* 13 */ {0x00A77AE0, {{Pm4::DB_SRESULTS_COMPARE_STATE0, 0x00000000}}},
    /* 14 */ {0x00A779B7, {{Pm4::DB_SRESULTS_COMPARE_STATE1, 0x00000000}}},
    /* 15 */ {0x5100100C, {{Pm4::DB_STENCILREFMASK, 0x00000000}}},
    /* 16 */ {0x59958BBA, {{Pm4::DB_STENCILREFMASK_BF, 0x00000000}}},
    /* 17 */ {0x0C06F17C, {{Pm4::DB_STENCIL_CONTROL, 0x00000000}}},
    /* 18 */ {0x6F104B72, {{Pm4::GE_MAX_OUTPUT_PER_SUBGROUP, 0x00000000}}},
    /* 19 */ {0x25C70D9C, {{Pm4::PA_CL_CLIP_CNTL, 0x00000000}}},
    /* 20 */ {0x3881201E, {{Pm4::PA_CL_OBJPRIM_ID_CNTL, 0x00000000}}},
    /* 21 */ {0x09AFDDAF, {{Pm4::PA_CL_VTE_CNTL, 0x0000043f}}},
    /* 22 */ {0x367D63CF, {{Pm4::PA_SC_AA_CONFIG, 0x00000000}}},
    /* 23 */ {0x43707DB8, {{Pm4::PA_SC_CLIPRECT_RULE, 0x0000ffff}}},
    /* 24 */ {0xF6AE26BA, {{Pm4::PA_SC_CONSERVATIVE_RASTERIZATION_CNTL, 0x00000000}}},
    /* 25 */ {0x1B917652, {{Pm4::PA_SC_FSR_ENABLE, 0x00000000}}},
    /* 26 */ {0x94B1E4F7, {{Pm4::PA_SC_HORIZ_GRID, 0x00000000}}},
    /* 27 */ {0xE3661B6C, {{Pm4::PA_SC_LEFT_VERT_GRID, 0x00000000}}},
    /* 28 */ {0x1EB8D73A, {{Pm4::PA_SC_MODE_CNTL_0, 0x00000002}}},
    /* 29 */ {0x15051FA3, {{Pm4::PA_SC_MODE_CNTL_1, 0x00000000}}},
    /* 30 */ {0x9C51A7F1, {{Pm4::PA_SC_RIGHT_VERT_GRID, 0x00000000}}},
    /* 31 */ {0xA20EFC70, {{Pm4::PA_SC_WINDOW_OFFSET, 0x00000000}}},
    /* 32 */ {0x0EC09F6E, {{Pm4::PA_STATE_STEREO_X, 0x00000000}}},
    /* 33 */ {0x34A7D6D3, {{Pm4::PA_STEREO_CNTL, 0x00000000}}},
    /* 34 */ {0xCE831B94, {{Pm4::PA_SU_HARDWARE_SCREEN_OFFSET, 0x00000000}}},
    /* 35 */ {0x5CC72A74, {{Pm4::PA_SU_LINE_CNTL, 0x00000008}}},
    /* 36 */ {0x3B77713C, {{Pm4::PA_SU_POINT_MINMAX, 0xffff0000}}},
    /* 37 */ {0x40F64410, {{Pm4::PA_SU_POINT_SIZE, 0x00080008}}},
    /* 38 */ {0x69441268, {{Pm4::PA_SU_POLY_OFFSET_CLAMP, 0x00000000}}},
    /* 39 */ {0x2E418B83, {{Pm4::PA_SU_POLY_OFFSET_DB_FMT_CNTL, 0x000001e9}}},
    /* 40 */ {0xA00D0C8D, {{Pm4::PA_SU_SC_MODE_CNTL, 0x00000240}}},
    /* 41 */ {0xB1289FB3, {{Pm4::PA_SU_SMALL_PRIM_FILTER_CNTL, 0x00000001}}},
    /* 42 */ {0x144832FB, {{Pm4::PA_SU_VTX_CNTL, 0x0000002d}}},
    /* 43 */ {0x9890D9FA, {{Pm4::SPI_TMPRING_SIZE, 0x00000000}}},
    /* 44 */ {0x9016FAF1, {{Pm4::VGT_DRAW_PAYLOAD_CNTL, 0x00000000}}},
    /* 45 */ {0x4B73CE27, {{Pm4::VGT_GS_MAX_VERT_OUT, 0x00000400}}},
    /* 46 */ {0x5F5A3E7B, {{Pm4::VGT_GS_OUT_PRIM_TYPE, 0x00000002}}},
    /* 47 */ {0xD4AF3A51, {{Pm4::VGT_LS_HS_CONFIG, 0x00000000}}},
    /* 48 */ {0x6CF4F543, {{Pm4::VGT_PRIMITIVEID_RESET, 0xffffffff}}},
    /* 49 */ {0x5FB86CCB, {{Pm4::VGT_PRIMITIVEID_EN, 0x00000000}}},
    /* 50 */ {0xEDEFA188, {{Pm4::VGT_REUSE_OFF, 0x00000000}}},
    /* 51 */ {0xD0DE9EE6, {{Pm4::VGT_SHADER_STAGES_EN, 0x00000000}}},
    /* 52 */ {0xC5831803, {{Pm4::VGT_TESS_DISTRIBUTION, 0x88101000}}},
    /* 53 */ {0x8E6DE84B, {{Pm4::VGT_TF_PARAM, 0x00000000}}},
    /* 54 */
    {0xD0771662,
     {
         {Pm4::PA_SC_CENTROID_PRIORITY_0, 0x00000000},
         {Pm4::PA_SC_CENTROID_PRIORITY_1, 0x00000000},
     }},
    /* 55 */ {0x569F7444, {{Pm4::PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0, 0x00000000}}},
    /* 56 */
    {0x5C6637CD,
     {
         {Pm4::PA_SC_AA_MASK_X0Y0_X1Y0, 0xffffffff},
         {Pm4::PA_SC_AA_MASK_X0Y1_X1Y1, 0xffffffff},
     }},
    /* 57 */
    {0xCAE3E690,
     {
         {Pm4::PA_SC_BINNER_CNTL_0, 0x00000002},
         {Pm4::PA_SC_BINNER_CNTL_1, 0x03ff0080},
     }},
    /* 58 */
    {0x43FBD769,
     {
         {Pm4::CB_BLEND_RED, 0x00000000},
         {Pm4::CB_BLEND_BLUE, 0x00000000},
         {Pm4::CB_BLEND_GREEN, 0x00000000},
         {Pm4::CB_BLEND_ALPHA, 0x00000000},
     }},
    /* 59 */ {0xEF550356, {{Pm4::CB_BLEND0_CONTROL, 0x20010001}}},
    /* 60 */
    {0x8F52E279,
     {
         {Pm4::TA_BC_BASE_ADDR, 0x00000000},
         {Pm4::TA_BC_BASE_ADDR_HI, 0x00000000},
     }},
    /* 61 */
    {0x1F2D8149,
     {
         {Pm4::PA_SC_CLIPRECT_0_TL, 0x00000000},
         {Pm4::PA_SC_CLIPRECT_0_BR, 0x20002000},
     }},
    /* 62 */ {0x853D0614, {{Pm4::CX_NOP, 0x00000000}}},
    /* 63 */
    {0x4413C6F9,
     {
         {Pm4::DB_DEPTH_BOUNDS_MIN, 0x00000000},
         {Pm4::DB_DEPTH_BOUNDS_MAX, 0x00000000},
     }},
    /* 64 */
    {0x67096014,
     {
         {Pm4::DB_Z_INFO, 0x80000000},
         {Pm4::DB_STENCIL_INFO, 0x20000000},
         {Pm4::DB_Z_READ_BASE, 0x00000000},
         {Pm4::DB_STENCIL_READ_BASE, 0x00000000},
         {Pm4::DB_Z_WRITE_BASE, 0x00000000},
         {Pm4::DB_STENCIL_WRITE_BASE, 0x00000000},
         {Pm4::DB_Z_READ_BASE_HI, 0x00000000},
         {Pm4::DB_STENCIL_READ_BASE_HI, 0x00000000},
         {Pm4::DB_Z_WRITE_BASE_HI, 0x00000000},
         {Pm4::DB_STENCIL_WRITE_BASE_HI, 0x00000000},
         {Pm4::DB_HTILE_DATA_BASE_HI, 0x00000000},
         {Pm4::DB_DEPTH_VIEW, 0x00000000},
         {Pm4::DB_HTILE_DATA_BASE, 0x00000000},
         {Pm4::DB_DEPTH_SIZE_XY, 0x00000000},
         {Pm4::DB_DEPTH_CLEAR, 0x00000000},
         {Pm4::DB_STENCIL_CLEAR, 0x00000000},
     }},
    /* 65 */
    {0x88F5E915,
     {
         {Pm4::PA_SC_FOV_WINDOW_LR, 0xff00ff00},
         {Pm4::PA_SC_FOV_WINDOW_TB, 0x00000000},
     }},
    /* 66 */
    {0x033F1EFF,
     {
         {Pm4::FSR_RECURSIONS0, 0x00000000},
         {Pm4::FSR_RECURSIONS1, 0x00000000},
     }},
    /* 67 */
    {0x918106BB,
     {
         {Pm4::PA_SC_GENERIC_SCISSOR_TL, 0x80000000},
         {Pm4::PA_SC_GENERIC_SCISSOR_BR, 0x40004000},
     }},
    /* 68 */
    {0x95F0E7AC,
     {
         {Pm4::PA_CL_GB_VERT_CLIP_ADJ, 0x4e7e0000},
         {Pm4::PA_CL_GB_VERT_DISC_ADJ, 0x4e7e0000},
         {Pm4::PA_CL_GB_HORZ_CLIP_ADJ, 0x4e7e0000},
         {Pm4::PA_CL_GB_HORZ_DISC_ADJ, 0x4e7e0000},
     }},
    /* 69 */
    {0xB48CBAB2,
     {
         {Pm4::PA_SU_POLY_OFFSET_BACK_SCALE, 0x00000000},
         {Pm4::PA_SU_POLY_OFFSET_BACK_OFFSET, 0x00000000},
     }},
    /* 70 */
    {0x05BB3BC6,
     {
         {Pm4::PA_SU_POLY_OFFSET_FRONT_SCALE, 0x00000000},
         {Pm4::PA_SU_POLY_OFFSET_FRONT_OFFSET, 0x00000000},
     }},
    /* 71 */
    {0x94FABA07,
     {
         {Pm4::DB_RENDER_OVERRIDE, 0x00000000},
         {Pm4::DB_RENDER_OVERRIDE2, 0x00000000},
     }},
    /* 72 */
    {0x38E92C91,
     {
         {Pm4::CB_COLOR0_BASE, 0x00000000},
         {Pm4::CB_COLOR0_VIEW, 0x00000000},
         {Pm4::CB_COLOR0_INFO, 0x00000000},
         {Pm4::CB_COLOR0_ATTRIB, 0x00000000},
         {Pm4::CB_COLOR0_DCC_CONTROL, 0x00000048},
         {Pm4::CB_COLOR0_CMASK, 0x00000000},
         {Pm4::CB_COLOR0_FMASK, 0x00000000},
         {Pm4::CB_COLOR0_CLEAR_WORD0, 0x00000000},
         {Pm4::CB_COLOR0_CLEAR_WORD1, 0x00000000},
         {Pm4::CB_COLOR0_DCC_BASE, 0x00000000},
         {Pm4::CB_COLOR0_BASE_EXT, 0x00000000},
         {Pm4::CB_COLOR0_CMASK_BASE_EXT, 0x00000000},
         {Pm4::CB_COLOR0_FMASK_BASE_EXT, 0x00000000},
         {Pm4::CB_COLOR0_DCC_BASE_EXT, 0x00000000},
         {Pm4::CB_COLOR0_ATTRIB2, 0x00000000},
         {Pm4::CB_COLOR0_ATTRIB3, 0x0006c000},
     }},
    /* 73 */
    {0x0B177B43,
     {
         {Pm4::PA_SC_SCREEN_SCISSOR_TL, 0x00000000},
         {Pm4::PA_SC_SCREEN_SCISSOR_BR, 0x40004000},
     }},
    /* 74 */ {0x48531062, {{Pm4::SPI_PS_INPUT_CNTL_0, 0x00000000}}},
    /* 75 */
    {0xAAA964B9,
     {
         {Pm4::PA_CL_UCP_0_X, 0x00000000},
         {Pm4::PA_CL_UCP_0_Y, 0x00000000},
         {Pm4::PA_CL_UCP_0_Z, 0x00000000},
         {Pm4::PA_CL_UCP_0_W, 0x00000000},
     }},
    /* 76 */
    {0x7690AF6F,
     {
         {Pm4::PA_CL_VPORT_XSCALE, 0x4e7e0000},
         {Pm4::PA_CL_VPORT_YSCALE, 0x4e7e0000},
         {Pm4::PA_CL_VPORT_ZSCALE, 0x4e7e0000},
         {Pm4::PA_CL_VPORT_XOFFSET, 0x00000000},
         {Pm4::PA_CL_VPORT_YOFFSET, 0x00000000},
         {Pm4::PA_CL_VPORT_ZOFFSET, 0x00000000},
         {Pm4::PA_SC_VPORT_SCISSOR_0_TL, 0x80000000},
         {Pm4::PA_SC_VPORT_SCISSOR_0_BR, 0x40004000},
         {Pm4::PA_SC_VPORT_ZMIN_0, 0x00000000},
         {Pm4::PA_SC_VPORT_ZMAX_0, 0x00000000},
     }},
    /* 77 */
    {0x078D7060,
     {
         {Pm4::PA_SC_WINDOW_SCISSOR_TL, 0x80000000},
         {Pm4::PA_SC_WINDOW_SCISSOR_BR, 0x40004000},
     }},

};

static RegisterDefaultInfo g_sh_reg_info1[] = {
    /* 0 */ {0x5D6E3EC7, {{Pm4::COMPUTE_PGM_RSRC1, 0x00000000}}},
    /* 1 */ {0x57E7079A, {{Pm4::COMPUTE_PGM_RSRC2, 0x00000000}}},
    /* 2 */ {0x7467FAFD, {{Pm4::COMPUTE_PGM_RSRC3, 0x00000000}}},
    /* 3 */ {0x9E826B50, {{Pm4::COMPUTE_RESOURCE_LIMITS, 0x00000000}}},
    /* 4 */ {0xDC484F18, {{Pm4::COMPUTE_TMPRING_SIZE, 0x00000000}}},
    /* 5 */ {0x5DA8BCA3, {{Pm4::SPI_SHADER_PGM_RSRC1_GS, 0x00000000}}},
    /* 6 */ {0x5CA726D8, {{Pm4::SPI_SHADER_PGM_RSRC1_HS, 0x00000000}}},
    /* 7 */ {0x5DD28360, {{Pm4::SPI_SHADER_PGM_RSRC1_PS, 0x00000000}}},
    /* 8 */ {0x57EFA0BE, {{Pm4::SPI_SHADER_PGM_RSRC2_GS, 0x00000000}}},
    /* 9 */ {0x502363D5, {{Pm4::SPI_SHADER_PGM_RSRC2_HS, 0x00000000}}},
    /* 10 */ {0x506D14BD, {{Pm4::SPI_SHADER_PGM_RSRC2_PS, 0x00000000}}},
    /* 11 */ {0xB2609506, {{Pm4::COMPUTE_USER_ACCUM_0, 0x00000000}}},
    /* 12 */
    {0x9E5CFB8A,
     {
         {Pm4::SPI_SHADER_PGM_RSRC3_HS, 0x00000000},
         {Pm4::SPI_SHADER_PGM_RSRC3_GS, 0x00000000},
         {Pm4::SPI_SHADER_PGM_RSRC3_PS, 0x00000000},
     }},
    /* 13 */
    {0xC918DF3E,
     {
         {Pm4::COMPUTE_PGM_LO, 0x00000000},
         {Pm4::COMPUTE_PGM_HI, 0x00000000},
     }},
    /* 14 */
    {0xC9751C9C,
     {
         {Pm4::SPI_SHADER_PGM_LO_ES, 0x00000000},
         {Pm4::SPI_SHADER_PGM_HI_ES, 0x00000000},
     }},
    /* 15 */
    {0xC97EF77A,
     {
         {Pm4::SPI_SHADER_PGM_LO_GS, 0x00000000},
         {Pm4::SPI_SHADER_PGM_HI_GS, 0x00000000},
     }},
    /* 16 */
    {0xC927C6B9,
     {
         {Pm4::SPI_SHADER_PGM_LO_HS, 0x00000000},
         {Pm4::SPI_SHADER_PGM_HI_HS, 0x00000000},
     }},
    /* 17 */
    {0xC92A1EC5,
     {
         {Pm4::SPI_SHADER_PGM_LO_LS, 0x00000000},
         {Pm4::SPI_SHADER_PGM_HI_LS, 0x00000000},
     }},
    /* 18 */
    {0xC9E01B31,
     {
         {Pm4::SPI_SHADER_PGM_LO_PS, 0x00000000},
         {Pm4::SPI_SHADER_PGM_HI_PS, 0x00000000},
     }},
    /* 19 */ {0x50685F29, {{Pm4::SH_NOP, 0x00000000}}},
    /* 20 */ {0xB26219CA, {{Pm4::SPI_SHADER_USER_ACCUM_ESGS_0, 0x00000000}}},
    /* 21 */ {0xB25B6CF9, {{Pm4::SPI_SHADER_USER_ACCUM_LSHS_0, 0x00000000}}},
    /* 22 */ {0xB2F86101, {{Pm4::SPI_SHADER_USER_ACCUM_PS_0, 0x00000000}}},
    /* 23 */
    {0x07E3B155,
     {
         {Pm4::SPI_SHADER_USER_DATA_ADDR_LO_GS, 0x00000000},
         {Pm4::SPI_SHADER_USER_DATA_ADDR_HI_GS, 0x00000000},
     }},
    /* 24 */
    {0x07E383C6,
     {
         {Pm4::SPI_SHADER_USER_DATA_ADDR_LO_HS, 0x00000000},
         {Pm4::SPI_SHADER_USER_DATA_ADDR_HI_HS, 0x00000000},
     }},
    /* 25 */ {0xBDA98653, {{Pm4::COMPUTE_USER_DATA_0, 0x00000000}}},
    /* 26 */ {0xBDBD1D0F, {{Pm4::SPI_SHADER_USER_DATA_GS_0, 0x00000000}}},
    /* 27 */ {0xBD946FD4, {{Pm4::SPI_SHADER_USER_DATA_HS_0, 0x00000000}}},
    /* 28 */ {0xBDF02A4C, {{Pm4::SPI_SHADER_USER_DATA_PS_0, 0x00000000}}},
};

static RegisterDefaultInfo g_uc_reg_info1[] = {
    /* 0 */ {0x19E93E85, {{Pm4::GDS_OA_ADDRESS, 0x00000000}}},
    /* 1 */ {0x3B5C2AF3, {{Pm4::GDS_OA_CNTL, 0x00000000}}},
    /* 2 */ {0x47974A35, {{Pm4::GDS_OA_COUNTER, 0x00000000}}},
    /* 3 */ {0x105971C2, {{Pm4::GE_CNTL, 0x00000000}}},
    /* 4 */ {0x7D137765, {{Pm4::GE_INDX_OFFSET, 0x00000000}}},
    /* 5 */ {0xD187FEBC, {{Pm4::GE_MULTI_PRIM_IB_RESET_EN, 0x00000000}}},
    /* 6 */ {0x12F854AC, {{Pm4::GE_STEREO_CNTL, 0x00000000}}},
    /* 7 */ {0x40D49AD1, {{Pm4::GE_USER_VGPR_EN, 0x00000000}}},
    /* 8 */ {0x8C0923DA, {{Pm4::FSR_EXTEND_SUBPIXEL_ROUNDING, 0x00000000}}},
    /* 9 */ {0xBB8DF494, {{Pm4::TEXTURE_GRADIENT_CONTROL, 0x00000000}}},
    /* 10 */ {0xF6D8A76E, {{Pm4::TEXTURE_GRADIENT_FACTORS, 0x40000040}}},
    /* 11 */ {0x7620F1E9, {{Pm4::VGT_OBJECT_ID, 0x00000000}}},
    /* 12 */ {0x9EBFAB10, {{Pm4::VGT_PRIMITIVE_TYPE, 0x00000000}}},
    /* 13 */
    {0x98A09D0E,
     {
         {Pm4::TA_CS_BC_BASE_ADDR, 0x00000000},
         {Pm4::TA_CS_BC_BASE_ADDR_HI, 0x00000000},
     }},
    /* 14 */
    {0x195D37D2,
     {
         {Pm4::FSR_ALPHA_VALUE0, 0x00000000},
         {Pm4::FSR_ALPHA_VALUE1, 0x00000000},
     }},
    /* 15 */
    {0xF9EC4F85,
     {
         {Pm4::FSR_CONTROL_POINT0, 0x00000000},
         {Pm4::FSR_CONTROL_POINT1, 0x00000000},
         {Pm4::FSR_CONTROL_POINT2, 0x00000000},
         {Pm4::FSR_CONTROL_POINT3, 0x00000000},
     }},
    /* 16 */
    {0x4626B750,
     {
         {Pm4::FSR_WINDOW0, 0x00000000},
         {Pm4::FSR_WINDOW1, 0x00000000},
     }},
    /* 17 */ {0x4CC673A0, {{Pm4::MEMORY_MAPPING_MASK, 0x00000000}}},
    /* 18 */ {0xDE5B3431, {{Pm4::UC_NOP, 0x00000000}}},
    /* 19 */ {0x036AC8A6, {{Pm4::GE_USER_VGPR1, 0x00000000}}}};

static RegisterDefaultInfo g_cx_reg_info2[] = {
    /* 0 */ {0x8FB4EDB5, {{Pm4::DB_DFSM_CONTROL, 0x00000000}}},
    /* 1 */ {0xB994AD29, {{Pm4::DB_HTILE_SURFACE, 0x00000000}}},
    /* 2 */ {0xD427322F, {{Pm4::PA_SC_NGG_MODE_CNTL, 0x00000000}}},
    /* 3 */ {0xF58FEA31, {{Pm4::SPI_INTERP_CONTROL_0, 0x00000000}}},
};

static RegisterDefaultInfo g_sh_reg_info2[] = {
    /* 0 */ {0x6AC156EF, {{Pm4::COMPUTE_DESTINATION_EN_SE0, 0x00000000}}},
    /* 1 */ {0x6AC15610, {{Pm4::COMPUTE_DESTINATION_EN_SE1, 0x00000000}}},
    /* 2 */ {0x6AC15009, {{Pm4::COMPUTE_DESTINATION_EN_SE2, 0x00000000}}},
    /* 3 */ {0x6AC153BA, {{Pm4::COMPUTE_DESTINATION_EN_SE3, 0x00000000}}},
    /* 4 */ {0xBE7DCD73, {{Pm4::COMPUTE_DISPATCH_TUNNEL, 0x00000000}}},
    /* 5 */ {0x0C4B1438, {{Pm4::COMPUTE_SHADER_CHKSUM, 0x00000000}}},
    /* 6 */ {0xDB00D71A, {{Pm4::COMPUTE_START_X, 0x00000000}}},
    /* 7 */ {0xDB00D249, {{Pm4::COMPUTE_START_Y, 0x00000000}}},
    /* 8 */ {0xDB00EC60, {{Pm4::COMPUTE_START_Z, 0x00000000}}},
    /* 9 */ {0x0C4D6FE4, {{Pm4::SPI_SHADER_PGM_CHKSUM_GS, 0x00000000}}},
    /* 10 */ {0x0C4A80EF, {{Pm4::SPI_SHADER_PGM_CHKSUM_HS, 0x00000000}}},
    /* 11 */ {0x0DD283E7, {{Pm4::SPI_SHADER_PGM_CHKSUM_PS, 0x00000000}}},
    /* 12 */ {0xC620E68C, {{Pm4::SPI_SHADER_PGM_RSRC4_GS, 0x00000000}}},
    /* 13 */ {0xC67EFACF, {{Pm4::SPI_SHADER_PGM_RSRC4_HS, 0x00000000}}},
    /* 14 */ {0xD9E6D9F7, {{Pm4::SPI_SHADER_PGM_RSRC4_PS, 0x00000000}}},
};

static RegisterDefaultInfo g_uc_reg_info2[] = {
    /* 0 */ {0x31F34B9F, {{Pm4::VGT_HS_OFFCHIP_PARAM, 0x00000000}}},
    /* 1 */ {0xAC0F9E76, {{Pm4::UC_NOP, 0x00000000}}},
    /* 2 */ {0x929FD95D, {{Pm4::VGT_TF_MEMORY_BASE, 0x00000000}}},
};

#define KYTY_ID(id, tbl)   ((id)*4 + (tbl))
#define KYTY_INDEX_CX1(id) g_cx_reg_info1[id].type, KYTY_ID(id, 0), 0
#define KYTY_INDEX_SH1(id) g_sh_reg_info1[id].type, KYTY_ID(id, 1), 0
#define KYTY_INDEX_UC1(id) g_uc_reg_info1[id].type, KYTY_ID(id, 2), 0
#define KYTY_INDEX_CX2(id) g_cx_reg_info2[id].type, KYTY_ID(id, 0), 0
#define KYTY_INDEX_SH2(id) g_sh_reg_info2[id].type, KYTY_ID(id, 1), 0
#define KYTY_INDEX_UC2(id) g_uc_reg_info2[id].type, KYTY_ID(id, 2), 0
#define KYTY_REG_CX1(id)   &g_cx_reg_info1[id].reg[0]
#define KYTY_REG_SH1(id)   &g_sh_reg_info1[id].reg[0]
#define KYTY_REG_UC1(id)   &g_uc_reg_info1[id].reg[0]
#define KYTY_REG_CX2(id)   &g_cx_reg_info2[id].reg[0]
#define KYTY_REG_SH2(id)   &g_sh_reg_info2[id].reg[0]
#define KYTY_REG_UC2(id)   &g_uc_reg_info2[id].reg[0]

static ShaderRegister* g_tbl_cx1[] = {
    KYTY_REG_CX1(0),  KYTY_REG_CX1(1),  KYTY_REG_CX1(2),  KYTY_REG_CX1(3),  KYTY_REG_CX1(4),  KYTY_REG_CX1(5),  KYTY_REG_CX1(6),
    KYTY_REG_CX1(7),  KYTY_REG_CX1(8),  KYTY_REG_CX1(9),  KYTY_REG_CX1(10), KYTY_REG_CX1(11), KYTY_REG_CX1(12), KYTY_REG_CX1(13),
    KYTY_REG_CX1(14), KYTY_REG_CX1(15), KYTY_REG_CX1(16), KYTY_REG_CX1(17), KYTY_REG_CX1(18), KYTY_REG_CX1(19), KYTY_REG_CX1(20),
    KYTY_REG_CX1(21), KYTY_REG_CX1(22), KYTY_REG_CX1(23), KYTY_REG_CX1(24), KYTY_REG_CX1(25), KYTY_REG_CX1(26), KYTY_REG_CX1(27),
    KYTY_REG_CX1(28), KYTY_REG_CX1(29), KYTY_REG_CX1(30), KYTY_REG_CX1(31), KYTY_REG_CX1(32), KYTY_REG_CX1(33), KYTY_REG_CX1(34),
    KYTY_REG_CX1(35), KYTY_REG_CX1(36), KYTY_REG_CX1(37), KYTY_REG_CX1(38), KYTY_REG_CX1(39), KYTY_REG_CX1(40), KYTY_REG_CX1(41),
    KYTY_REG_CX1(42), KYTY_REG_CX1(43), KYTY_REG_CX1(44), KYTY_REG_CX1(45), KYTY_REG_CX1(46), KYTY_REG_CX1(47), KYTY_REG_CX1(48),
    KYTY_REG_CX1(49), KYTY_REG_CX1(50), KYTY_REG_CX1(51), KYTY_REG_CX1(52), KYTY_REG_CX1(53), KYTY_REG_CX1(54), KYTY_REG_CX1(55),
    KYTY_REG_CX1(56), KYTY_REG_CX1(57), KYTY_REG_CX1(58), KYTY_REG_CX1(59), KYTY_REG_CX1(60), KYTY_REG_CX1(61), KYTY_REG_CX1(62),
    KYTY_REG_CX1(63), KYTY_REG_CX1(64), KYTY_REG_CX1(65), KYTY_REG_CX1(66), KYTY_REG_CX1(67), KYTY_REG_CX1(68), KYTY_REG_CX1(69),
    KYTY_REG_CX1(70), KYTY_REG_CX1(71), KYTY_REG_CX1(72), KYTY_REG_CX1(73), KYTY_REG_CX1(74), KYTY_REG_CX1(75), KYTY_REG_CX1(76),
    KYTY_REG_CX1(77)};

static ShaderRegister* g_tbl_sh1[]    = {KYTY_REG_SH1(0),  KYTY_REG_SH1(1),  KYTY_REG_SH1(2),  KYTY_REG_SH1(3),  KYTY_REG_SH1(4),
                                         KYTY_REG_SH1(5),  KYTY_REG_SH1(6),  KYTY_REG_SH1(7),  KYTY_REG_SH1(8),  KYTY_REG_SH1(9),
                                         KYTY_REG_SH1(10), KYTY_REG_SH1(11), KYTY_REG_SH1(12), KYTY_REG_SH1(13), KYTY_REG_SH1(14),
                                         KYTY_REG_SH1(15), KYTY_REG_SH1(16), KYTY_REG_SH1(17), KYTY_REG_SH1(18), KYTY_REG_SH1(19),
                                         KYTY_REG_SH1(20), KYTY_REG_SH1(21), KYTY_REG_SH1(22), KYTY_REG_SH1(23), KYTY_REG_SH1(24),
                                         KYTY_REG_SH1(25), KYTY_REG_SH1(26), KYTY_REG_SH1(27), KYTY_REG_SH1(28)};
static ShaderRegister* g_tbl_uc1[]    = {KYTY_REG_UC1(0),  KYTY_REG_UC1(1),  KYTY_REG_UC1(2),  KYTY_REG_UC1(3),  KYTY_REG_UC1(4),
                                         KYTY_REG_UC1(5),  KYTY_REG_UC1(6),  KYTY_REG_UC1(7),  KYTY_REG_UC1(8),  KYTY_REG_UC1(9),
                                         KYTY_REG_UC1(10), KYTY_REG_UC1(11), KYTY_REG_UC1(12), KYTY_REG_UC1(13), KYTY_REG_UC1(14),
                                         KYTY_REG_UC1(15), KYTY_REG_UC1(16), KYTY_REG_UC1(17), KYTY_REG_UC1(18), KYTY_REG_UC1(19)};
static uint32_t        g_tbl_index1[] = {
           KYTY_INDEX_CX1(0),  KYTY_INDEX_CX1(1),  KYTY_INDEX_CX1(2),  KYTY_INDEX_CX1(3),  KYTY_INDEX_CX1(4),  KYTY_INDEX_CX1(5),
           KYTY_INDEX_CX1(6),  KYTY_INDEX_CX1(7),  KYTY_INDEX_CX1(8),  KYTY_INDEX_CX1(9),  KYTY_INDEX_CX1(10), KYTY_INDEX_CX1(11),
           KYTY_INDEX_CX1(12), KYTY_INDEX_CX1(13), KYTY_INDEX_CX1(14), KYTY_INDEX_CX1(15), KYTY_INDEX_CX1(16), KYTY_INDEX_CX1(17),
           KYTY_INDEX_CX1(18), KYTY_INDEX_CX1(19), KYTY_INDEX_CX1(20), KYTY_INDEX_CX1(21), KYTY_INDEX_CX1(22), KYTY_INDEX_CX1(23),
           KYTY_INDEX_CX1(24), KYTY_INDEX_CX1(25), KYTY_INDEX_CX1(26), KYTY_INDEX_CX1(27), KYTY_INDEX_CX1(28), KYTY_INDEX_CX1(29),
           KYTY_INDEX_CX1(30), KYTY_INDEX_CX1(31), KYTY_INDEX_CX1(32), KYTY_INDEX_CX1(33), KYTY_INDEX_CX1(34), KYTY_INDEX_CX1(35),
           KYTY_INDEX_CX1(36), KYTY_INDEX_CX1(37), KYTY_INDEX_CX1(38), KYTY_INDEX_CX1(39), KYTY_INDEX_CX1(40), KYTY_INDEX_CX1(41),
           KYTY_INDEX_CX1(42), KYTY_INDEX_CX1(43), KYTY_INDEX_CX1(44), KYTY_INDEX_CX1(45), KYTY_INDEX_CX1(46), KYTY_INDEX_CX1(47),
           KYTY_INDEX_CX1(48), KYTY_INDEX_CX1(49), KYTY_INDEX_CX1(50), KYTY_INDEX_CX1(51), KYTY_INDEX_CX1(52), KYTY_INDEX_CX1(53),
           KYTY_INDEX_CX1(54), KYTY_INDEX_CX1(55), KYTY_INDEX_CX1(56), KYTY_INDEX_CX1(57), KYTY_INDEX_CX1(58), KYTY_INDEX_CX1(59),
           KYTY_INDEX_CX1(60), KYTY_INDEX_CX1(61), KYTY_INDEX_CX1(62), KYTY_INDEX_CX1(63), KYTY_INDEX_CX1(64), KYTY_INDEX_CX1(65),
           KYTY_INDEX_CX1(66), KYTY_INDEX_CX1(67), KYTY_INDEX_CX1(68), KYTY_INDEX_CX1(69), KYTY_INDEX_CX1(70), KYTY_INDEX_CX1(71),
           KYTY_INDEX_CX1(72), KYTY_INDEX_CX1(73), KYTY_INDEX_CX1(74), KYTY_INDEX_CX1(75), KYTY_INDEX_CX1(76), KYTY_INDEX_CX1(77),
           KYTY_INDEX_SH1(0),  KYTY_INDEX_SH1(1),  KYTY_INDEX_SH1(2),  KYTY_INDEX_SH1(3),  KYTY_INDEX_SH1(4),  KYTY_INDEX_SH1(5),
           KYTY_INDEX_SH1(6),  KYTY_INDEX_SH1(7),  KYTY_INDEX_SH1(8),  KYTY_INDEX_SH1(9),  KYTY_INDEX_SH1(10), KYTY_INDEX_SH1(11),
           KYTY_INDEX_SH1(12), KYTY_INDEX_SH1(13), KYTY_INDEX_SH1(14), KYTY_INDEX_SH1(15), KYTY_INDEX_SH1(16), KYTY_INDEX_SH1(17),
           KYTY_INDEX_SH1(18), KYTY_INDEX_SH1(19), KYTY_INDEX_SH1(20), KYTY_INDEX_SH1(21), KYTY_INDEX_SH1(22), KYTY_INDEX_SH1(23),
           KYTY_INDEX_SH1(24), KYTY_INDEX_SH1(25), KYTY_INDEX_SH1(26), KYTY_INDEX_SH1(27), KYTY_INDEX_SH1(28), KYTY_INDEX_UC1(0),
           KYTY_INDEX_UC1(1),  KYTY_INDEX_UC1(2),  KYTY_INDEX_UC1(3),  KYTY_INDEX_UC1(4),  KYTY_INDEX_UC1(5),  KYTY_INDEX_UC1(6),
           KYTY_INDEX_UC1(7),  KYTY_INDEX_UC1(8),  KYTY_INDEX_UC1(9),  KYTY_INDEX_UC1(10), KYTY_INDEX_UC1(11), KYTY_INDEX_UC1(12),
           KYTY_INDEX_UC1(13), KYTY_INDEX_UC1(14), KYTY_INDEX_UC1(15), KYTY_INDEX_UC1(16), KYTY_INDEX_UC1(17), KYTY_INDEX_UC1(18),
           KYTY_INDEX_UC1(19)};

static ShaderRegister* g_tbl_cx2[]    = {KYTY_REG_CX2(0), KYTY_REG_CX2(1), KYTY_REG_CX2(2), KYTY_REG_CX2(3)};
static ShaderRegister* g_tbl_sh2[]    = {KYTY_REG_SH2(0),  KYTY_REG_SH2(1),  KYTY_REG_SH2(2),  KYTY_REG_SH2(3),  KYTY_REG_SH2(4),
                                         KYTY_REG_SH2(5),  KYTY_REG_SH2(6),  KYTY_REG_SH2(7),  KYTY_REG_SH2(8),  KYTY_REG_SH2(9),
                                         KYTY_REG_SH2(10), KYTY_REG_SH2(11), KYTY_REG_SH2(12), KYTY_REG_SH2(13), KYTY_REG_SH2(14)};
static ShaderRegister* g_tbl_uc2[]    = {KYTY_REG_UC2(0), KYTY_REG_UC2(1), KYTY_REG_UC2(2)};
static uint32_t        g_tbl_index2[] = {KYTY_INDEX_CX2(0),  KYTY_INDEX_CX2(1),  KYTY_INDEX_CX2(2),  KYTY_INDEX_CX2(3),  KYTY_INDEX_SH2(0),
                                         KYTY_INDEX_SH2(1),  KYTY_INDEX_SH2(2),  KYTY_INDEX_SH2(3),  KYTY_INDEX_SH2(4),  KYTY_INDEX_SH2(5),
                                         KYTY_INDEX_SH2(6),  KYTY_INDEX_SH2(7),  KYTY_INDEX_SH2(8),  KYTY_INDEX_SH2(9),  KYTY_INDEX_SH2(10),
                                         KYTY_INDEX_SH2(11), KYTY_INDEX_SH2(12), KYTY_INDEX_SH2(13), KYTY_INDEX_SH2(14), KYTY_INDEX_UC2(0),
                                         KYTY_INDEX_UC2(1),  KYTY_INDEX_UC2(2)};

static RegisterDefaults g_reg_defaults1 = { // @suppress("Invalid arguments")
    g_tbl_cx1, g_tbl_sh1, g_tbl_uc1, nullptr, {0, 0}, g_tbl_index1, sizeof(g_tbl_index1) / 12};
static RegisterDefaults g_reg_defaults2 = { // @suppress("Invalid arguments")
    g_tbl_cx2, g_tbl_sh2, g_tbl_uc2, nullptr, {0, 0}, g_tbl_index2, sizeof(g_tbl_index2) / 12};

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

	return &g_reg_defaults1;
}

void* KYTY_SYSV_ABI GraphicsGetRegisterDefaults2Internal(uint32_t ver)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(ver != 8);
	EXIT_NOT_IMPLEMENTED(offsetof(RegisterDefaults, count) != 0x38);

	return &g_reg_defaults2;
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

	EXIT_NOT_IMPLEMENTED(h->file_header != 0x34333231);
	EXIT_NOT_IMPLEMENTED(h->version != 0x00000018);

	auto base = reinterpret_cast<uint64_t>(code);

	printf("\t base   = 0x%016" PRIx64 "\n", base);

	ShaderMappedData map;
	map.user_data           = h->user_data;
	map.input_semantics     = h->input_semantics;
	map.num_input_semantics = h->num_input_semantics;

	ShaderMapUserData(base, map);

	EXIT_NOT_IMPLEMENTED((base & 0xFFFF0000000000FFull) != 0);

	if (h->type == 2 && h->num_sh_registers >= 2 && h->sh_registers[0].offset == Pm4::SPI_SHADER_PGM_LO_ES &&
	    h->sh_registers[1].offset == Pm4::SPI_SHADER_PGM_HI_ES)
	{
		h->sh_registers[0].offset = Pm4::SPI_SHADER_PGM_LO_ES;
		h->sh_registers[0].value  = ((base >> 8u) & 0xffffffffu);
		h->sh_registers[1].offset = Pm4::SPI_SHADER_PGM_HI_ES;
		h->sh_registers[1].value  = ((base >> 40u) & 0x000000ffu);
	} else if (h->type == 1 && h->num_sh_registers >= 2 && h->sh_registers[0].offset == Pm4::SPI_SHADER_PGM_LO_PS &&
	           h->sh_registers[1].offset == Pm4::SPI_SHADER_PGM_HI_PS)
	{
		h->sh_registers[0].offset = Pm4::SPI_SHADER_PGM_LO_PS;
		h->sh_registers[0].value  = ((base >> 8u) & 0xffffffffu);
		h->sh_registers[1].offset = Pm4::SPI_SHADER_PGM_HI_PS;
		h->sh_registers[1].value  = ((base >> 40u) & 0x000000ffu);
	} else
	{
		EXIT("invalid shader\n");
	}

	*dst = h;

	dbg_dump_shader(h);

	return OK;
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
	uc_regs[2].offset = Pm4::VGT_PRIMITIVE_TYPE;
	uc_regs[2].value  = prim_type;

	return OK;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
	EXIT_NOT_IMPLEMENTED(ps != nullptr && ps->num_output_semantics != 0);

	for (uint16_t i = 0; i < 32; i++)
	{
		regs[i].offset = Pm4::SPI_PS_INPUT_CNTL_0 + i;
		regs[i].value  = 0;

		if (i < static_cast<int>(gs->num_output_semantics))
		{
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].semantic != 15 + i);
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].hardware_mapping != i);
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].size_in_elements != 0);
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].is_f16 != 0);
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].is_flat_shaded != 0);
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].is_linear != 0);
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].is_custom != 0);
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].static_vb_index != 0);
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].static_attribute != 0);
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].default_value != 0);
			EXIT_NOT_IMPLEMENTED(gs->output_semantics[i].default_value_hi != 0);
			bool flat = false;
			if (ps != nullptr)
			{
				EXIT_NOT_IMPLEMENTED(ps->input_semantics[i].semantic != gs->output_semantics[i].semantic);
				EXIT_NOT_IMPLEMENTED(ps->input_semantics[i].hardware_mapping != 0);
				EXIT_NOT_IMPLEMENTED(ps->input_semantics[i].size_in_elements != 0);
				EXIT_NOT_IMPLEMENTED(ps->input_semantics[i].is_f16 != 0);
				EXIT_NOT_IMPLEMENTED(ps->input_semantics[i].is_linear != 0);
				EXIT_NOT_IMPLEMENTED(ps->input_semantics[i].is_custom != 0);
				EXIT_NOT_IMPLEMENTED(ps->input_semantics[i].static_vb_index != 0);
				EXIT_NOT_IMPLEMENTED(ps->input_semantics[i].static_attribute != 0);
				EXIT_NOT_IMPLEMENTED(ps->input_semantics[i].default_value != 0);
				EXIT_NOT_IMPLEMENTED(ps->input_semantics[i].default_value_hi != 0);
				flat = ps->input_semantics[i].is_flat_shaded != 0;
			}
			regs[i].value = i | (flat ? 0x400u : 0u);
		}
	}

	return OK;
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

int KYTY_SYSV_ABI GraphicsSuspendPoint()
{
	PRINT_NAME();

	GraphicsRunDone();

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

	auto* marker = buf->AllocateDW(2);
	marker[0]    = KYTY_PM4(2, Pm4::IT_NOP, Pm4::R_ZERO);
	marker[1]    = 0x6875000d;

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

uint32_t* KYTY_SYSV_ABI GraphicsCbReleaseMem(CommandBuffer* buf, uint8_t action, uint16_t gcr_cntl, uint8_t dst, uint8_t cache_policy,
                                             const volatile Label* address, uint8_t data_sel, uint64_t data, uint16_t gds_offset,
                                             uint16_t gds_size, uint8_t interrupt, uint32_t interrupt_ctx_id)
{
	PRINT_NAME();

	printf("\t action           = 0x%02" PRIx8 "\n", action);
	printf("\t gcr_cntl         = 0x%04" PRIx16 "\n", gcr_cntl);
	printf("\t dst              = %" PRIu8 "\n", dst);
	printf("\t cache_policy     = 0x%02" PRIx8 "\n", cache_policy);
	printf("\t address          = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(address));
	printf("\t data_sel         = 0x%02" PRIx8 "\n", data_sel);
	printf("\t data             = 0x%016" PRIx64 "\n", data);
	printf("\t gds_offset       = %" PRIu16 "\n", gds_offset);
	printf("\t gds_size         = %" PRIu16 "\n", gds_size);
	printf("\t interrupt        = 0x%02" PRIx8 "\n", interrupt);
	printf("\t interrupt_ctx_id = %" PRIu32 "\n", interrupt_ctx_id);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);
	EXIT_NOT_IMPLEMENTED(dst != 1);
	EXIT_NOT_IMPLEMENTED(data_sel != 2 && data_sel != 3);
	EXIT_NOT_IMPLEMENTED(gds_offset != 0);
	EXIT_NOT_IMPLEMENTED(gds_size != 1);
	EXIT_NOT_IMPLEMENTED(interrupt != 0);
	EXIT_NOT_IMPLEMENTED(interrupt_ctx_id != 0);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(7);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(7, Pm4::IT_NOP, Pm4::R_RELEASE_MEM);
	cmd[1] = action | (static_cast<uint32_t>(cache_policy) << 8u);
	cmd[2] = gcr_cntl | (static_cast<uint32_t>(data_sel) << 16u);
	cmd[3] = static_cast<uint32_t>(reinterpret_cast<uint64_t>(address) & 0xffffffffu);
	cmd[4] = static_cast<uint32_t>((reinterpret_cast<uint64_t>(address) >> 32u) & 0xffffffffu);
	cmd[5] = static_cast<uint32_t>(data & 0xffffffffu);
	cmd[6] = static_cast<uint32_t>((data >> 32u) & 0xffffffffu);

	return cmd;
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

	auto* cmd = buf->AllocateDW(7);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(7, Pm4::IT_NOP, Pm4::R_WAIT_FLIP_DONE);
	cmd[1] = video_out_handle;
	cmd[2] = display_buffer_index;
	cmd[3] = 0;
	cmd[4] = 0;
	cmd[5] = 0;
	cmd[6] = 0;

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbSetShRegisterDirect(CommandBuffer* buf, ShaderRegister reg)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(buf == nullptr);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(3);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(3, Pm4::IT_SET_SH_REG, 0u);
	cmd[1] = reg.offset;
	cmd[2] = reg.value;

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

	cmd[0] = KYTY_PM4(4, Pm4::IT_NOP, Pm4::R_CX_REGS_INDIRECT);
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

	cmd[0] = KYTY_PM4(4, Pm4::IT_NOP, Pm4::R_SH_REGS_INDIRECT);
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

	cmd[0] = KYTY_PM4(4, Pm4::IT_NOP, Pm4::R_UC_REGS_INDIRECT);
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

uint32_t* KYTY_SYSV_ABI GraphicsDcbEventWrite(CommandBuffer* buf, uint8_t event_type, const volatile void* address)
{
	PRINT_NAME();

	printf("\t event_type = 0x%02" PRIx8 "\n", event_type);
	printf("\t address    = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(address));

	EXIT_NOT_IMPLEMENTED(buf == nullptr);
	EXIT_NOT_IMPLEMENTED(address != nullptr);
	EXIT_NOT_IMPLEMENTED(event_type > 0x3fu);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(2);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	uint32_t event_index = 0;

	cmd[0] = KYTY_PM4(2, Pm4::IT_EVENT_WRITE, 0u);
	cmd[1] = (event_index << 8u) | event_type;

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbAcquireMem(CommandBuffer* buf, uint8_t engine, uint32_t cb_db_op, uint32_t gcr_cntl,
                                              const volatile void* base, uint64_t size_bytes, uint32_t poll_cycles)
{
	PRINT_NAME();

	printf("\t engine      = 0x%02" PRIx8 "\n", engine);
	printf("\t cb_db_op    = 0x%08" PRIx32 "\n", cb_db_op);
	printf("\t gcr_cntl    = 0x%08" PRIx32 "\n", gcr_cntl);
	printf("\t base        = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(base));
	printf("\t size_bytes  = 0x%016" PRIx64 "\n", size_bytes);
	printf("\t poll_cycles = %" PRIu32 "\n", poll_cycles);

	bool no_size = (static_cast<int64_t>(size_bytes) == -1);
	auto vaddr   = reinterpret_cast<uint64_t>(base);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);
	EXIT_NOT_IMPLEMENTED(!no_size && (size_bytes && 0xffu) != 0);
	EXIT_NOT_IMPLEMENTED(!no_size && (size_bytes >> 40u) != 0);
	EXIT_NOT_IMPLEMENTED((vaddr && 0xffu) != 0);
	EXIT_NOT_IMPLEMENTED((vaddr >> 40u) != 0);
	EXIT_NOT_IMPLEMENTED(engine > 1);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(8);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(8, Pm4::IT_NOP, Pm4::R_ACQUIRE_MEM);
	cmd[1] = (static_cast<uint32_t>(engine) << 31u) | cb_db_op;
	cmd[2] = (no_size ? 0 : size_bytes >> 8u);
	cmd[3] = 0;
	cmd[4] = vaddr >> 8u;
	cmd[5] = 0;
	cmd[6] = poll_cycles / 40;
	cmd[7] = gcr_cntl;

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbWriteData(CommandBuffer* buf, uint8_t dst, uint8_t cache_policy, uint64_t address_or_offset,
                                             const void* data, uint32_t num_dwords, uint8_t increment, uint8_t write_confirm)
{
	PRINT_NAME();

	printf("\t dst               = 0x%02" PRIx8 "\n", dst);
	printf("\t cache_policy      = 0x%02" PRIx8 "\n", cache_policy);
	printf("\t address_or_offset = 0x%016" PRIx64 "\n", address_or_offset);
	printf("\t data              = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(data));
	printf("\t num_dwords        = %" PRIu32 "\n", num_dwords);
	printf("\t increment         = 0x%02" PRIx8 "\n", increment);
	printf("\t write_confirm     = 0x%02" PRIx8 "\n", write_confirm);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);
	EXIT_NOT_IMPLEMENTED((4 + num_dwords - 2u) > 0x3fffu);
	EXIT_NOT_IMPLEMENTED(data == nullptr);
	EXIT_NOT_IMPLEMENTED(address_or_offset == 0);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(4 + num_dwords);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(4 + num_dwords, Pm4::IT_NOP, Pm4::R_WRITE_DATA);
	cmd[1] = dst | (static_cast<uint32_t>(cache_policy) << 8u) | (static_cast<uint32_t>(increment) << 16u) |
	         (static_cast<uint32_t>(write_confirm) << 24u);
	cmd[2] = address_or_offset & 0xffffffffu;
	cmd[3] = (address_or_offset >> 32u) & 0xffffffffu;

	memcpy(cmd + 4, data, static_cast<size_t>(num_dwords) * 4);

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbWaitRegMem(CommandBuffer* buf, uint8_t size, uint8_t compare_function, uint8_t op, uint8_t cache_policy,
                                              const volatile void* address, uint64_t reference, uint64_t mask, uint32_t poll_cycles)
{
	PRINT_NAME();

	printf("\t size             = 0x%02" PRIx8 "\n", size);
	printf("\t compare_function = 0x%02" PRIx8 "\n", compare_function);
	printf("\t op               = 0x%02" PRIx8 "\n", op);
	printf("\t cache_policy     = 0x%02" PRIx8 "\n", cache_policy);
	printf("\t address          = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(address));
	printf("\t reference        = 0x%016" PRIx64 "\n", reference);
	printf("\t mask             = 0x%016" PRIx64 "\n", mask);
	printf("\t poll_cycles      = %" PRIu32 "\n", poll_cycles);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);
	EXIT_NOT_IMPLEMENTED(size != 1);
	EXIT_NOT_IMPLEMENTED(op != 1);
	EXIT_NOT_IMPLEMENTED(cache_policy != 0);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(9);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(9, Pm4::IT_NOP, Pm4::R_WAIT_MEM_64);
	cmd[1] = static_cast<uint32_t>(reinterpret_cast<uint64_t>(address) & 0xffffffffu);
	cmd[2] = static_cast<uint32_t>((reinterpret_cast<uint64_t>(address) >> 32u) & 0xffffffffu);
	cmd[3] = static_cast<uint32_t>(reinterpret_cast<uint64_t>(mask) & 0xffffffffu);
	cmd[4] = static_cast<uint32_t>((reinterpret_cast<uint64_t>(mask) >> 32u) & 0xffffffffu);
	cmd[5] = static_cast<uint32_t>(reinterpret_cast<uint64_t>(reference) & 0xffffffffu);
	cmd[6] = static_cast<uint32_t>((reinterpret_cast<uint64_t>(reference) >> 32u) & 0xffffffffu);
	cmd[7] = compare_function;
	cmd[8] = poll_cycles / 40;

	return cmd;
}

uint32_t* KYTY_SYSV_ABI GraphicsDcbSetFlip(CommandBuffer* buf, uint32_t video_out_handle, int32_t display_buffer_index, uint32_t flip_mode,
                                           int64_t flip_arg)
{
	PRINT_NAME();

	printf("\t video_out_handle     = %" PRIu32 "\n", video_out_handle);
	printf("\t display_buffer_index = %" PRId32 "\n", display_buffer_index);
	printf("\t flip_mode            = %" PRIu32 "\n", flip_mode);
	printf("\t flip_arg             = %" PRId64 "\n", flip_arg);

	EXIT_NOT_IMPLEMENTED(buf == nullptr);

	buf->DbgDump();

	auto* cmd = buf->AllocateDW(6);

	EXIT_NOT_IMPLEMENTED(cmd == nullptr);

	cmd[0] = KYTY_PM4(6, Pm4::IT_NOP, Pm4::R_FLIP);
	cmd[1] = video_out_handle;
	cmd[2] = display_buffer_index;
	cmd[3] = flip_mode;
	cmd[4] = static_cast<uint32_t>(static_cast<uint64_t>(flip_arg) & 0xffffffffu);
	cmd[5] = static_cast<uint32_t>((static_cast<uint64_t>(flip_arg) >> 32u) & 0xffffffffu);

	return cmd;
}

} // namespace Gen5

namespace Gen5Driver {

LIB_NAME("Graphics5Driver", "Graphics5Driver");

struct Packet
{
	uint32_t* addr;
	uint32_t  dw_num;
	uint8_t   pad[4];
};

int KYTY_SYSV_ABI GraphicsDriverSubmitDcb(const Packet* packet)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(packet == nullptr);
	EXIT_NOT_IMPLEMENTED(packet->pad[0] != 0);

	printf("\t addr   = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(packet->addr));
	printf("\t dw_num = 0x%08" PRIx32 "\n", packet->dw_num);

	GraphicsDbgDumpDcb("d", packet->dw_num, packet->addr);

	GraphicsRunSubmit(packet->addr, packet->dw_num, nullptr, 0);

	return OK;
}

} // namespace Gen5Driver

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
