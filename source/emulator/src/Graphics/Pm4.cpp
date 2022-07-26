#include "Emulator/Graphics/Pm4.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics::Pm4 {

static const char* g_names[256]          = {};
static const char* g_r_names[Pm4::R_NUM] = {};
static bool        g_names_initialized   = false;

static void init_names()
{
	if (!g_names_initialized)
	{
		for (auto& n: g_names)
		{
			n = "<unknown>";
		}

		for (auto& n: g_r_names)
		{
			n = "<unknown>";
		}

		g_r_names[R_ZERO]             = "R_ZERO";
		g_r_names[R_VS]               = "R_VS";
		g_r_names[R_PS]               = "R_PS";
		g_r_names[R_DRAW_INDEX]       = "R_DRAW_INDEX";
		g_r_names[R_DRAW_INDEX_AUTO]  = "R_DRAW_INDEX_AUTO";
		g_r_names[R_DRAW_RESET]       = "R_DRAW_RESET";
		g_r_names[R_WAIT_FLIP_DONE]   = "R_WAIT_FLIP_DONE";
		g_r_names[R_CS]               = "R_CS";
		g_r_names[R_DISPATCH_DIRECT]  = "R_DISPATCH_DIRECT";
		g_r_names[R_DISPATCH_RESET]   = "R_DISPATCH_RESET";
		g_r_names[R_WAIT_MEM_32]      = "R_WAIT_MEM_32";
		g_r_names[R_PUSH_MARKER]      = "R_PUSH_MARKER";
		g_r_names[R_POP_MARKER]       = "R_POP_MARKER";
		g_r_names[R_VS_EMBEDDED]      = "R_VS_EMBEDDED";
		g_r_names[R_PS_EMBEDDED]      = "R_PS_EMBEDDED";
		g_r_names[R_VS_UPDATE]        = "R_VS_UPDATE";
		g_r_names[R_PS_UPDATE]        = "R_PS_UPDATE";
		g_r_names[R_CX_REGS_INDIRECT] = "R_CX_REGS_INDIRECT";
		g_r_names[R_SH_REGS_INDIRECT] = "R_SH_REGS_INDIRECT";
		g_r_names[R_UC_REGS_INDIRECT] = "R_UC_REGS_INDIRECT";
		g_r_names[R_ACQUIRE_MEM]      = "R_ACQUIRE_MEM";
		g_r_names[R_WRITE_DATA]       = "R_WRITE_DATA";
		g_r_names[R_WAIT_MEM_64]      = "R_WAIT_MEM_64";
		g_r_names[R_FLIP]             = "R_FLIP";
		g_r_names[R_RELEASE_MEM]      = "R_RELEASE_MEM";

		g_names[IT_NOP]                       = "IT_NOP";
		g_names[IT_SET_BASE]                  = "IT_SET_BASE";
		g_names[IT_CLEAR_STATE]               = "IT_CLEAR_STATE";
		g_names[IT_INDEX_BUFFER_SIZE]         = "IT_INDEX_BUFFER_SIZE";
		g_names[IT_DISPATCH_DIRECT]           = "IT_DISPATCH_DIRECT";
		g_names[IT_DISPATCH_INDIRECT]         = "IT_DISPATCH_INDIRECT";
		g_names[IT_SET_PREDICATION]           = "IT_SET_PREDICATION";
		g_names[IT_COND_EXEC]                 = "IT_COND_EXEC";
		g_names[IT_DRAW_INDIRECT]             = "IT_DRAW_INDIRECT";
		g_names[IT_DRAW_INDEX_INDIRECT]       = "IT_DRAW_INDEX_INDIRECT";
		g_names[IT_INDEX_BASE]                = "IT_INDEX_BASE";
		g_names[IT_DRAW_INDEX_2]              = "IT_DRAW_INDEX_2";
		g_names[IT_CONTEXT_CONTROL]           = "IT_CONTEXT_CONTROL";
		g_names[IT_INDEX_TYPE]                = "IT_INDEX_TYPE";
		g_names[IT_DRAW_INDIRECT_MULTI]       = "IT_DRAW_INDIRECT_MULTI";
		g_names[IT_DRAW_INDEX_AUTO]           = "IT_DRAW_INDEX_AUTO";
		g_names[IT_NUM_INSTANCES]             = "IT_NUM_INSTANCES";
		g_names[IT_INDIRECT_BUFFER_CNST]      = "IT_INDIRECT_BUFFER_CNST";
		g_names[IT_DRAW_INDEX_OFFSET_2]       = "IT_DRAW_INDEX_OFFSET_2";
		g_names[IT_WRITE_DATA]                = "IT_WRITE_DATA";
		g_names[IT_MEM_SEMAPHORE]             = "IT_MEM_SEMAPHORE";
		g_names[IT_DRAW_INDEX_INDIRECT_MULTI] = "IT_DRAW_INDEX_INDIRECT_MULTI";
		g_names[IT_WAIT_REG_MEM]              = "IT_WAIT_REG_MEM";
		g_names[IT_INDIRECT_BUFFER]           = "IT_INDIRECT_BUFFER";
		g_names[IT_COPY_DATA]                 = "IT_COPY_DATA";
		g_names[IT_CP_DMA]                    = "IT_CP_DMA";
		g_names[IT_PFP_SYNC_ME]               = "IT_PFP_SYNC_ME";
		g_names[IT_SURFACE_SYNC]              = "IT_SURFACE_SYNC";
		g_names[IT_EVENT_WRITE]               = "IT_EVENT_WRITE";
		g_names[IT_EVENT_WRITE_EOP]           = "IT_EVENT_WRITE_EOP";
		g_names[IT_EVENT_WRITE_EOS]           = "IT_EVENT_WRITE_EOS";
		g_names[IT_RELEASE_MEM]               = "IT_RELEASE_MEM";
		g_names[IT_DMA_DATA]                  = "IT_DMA_DATA";
		g_names[IT_ACQUIRE_MEM]               = "IT_ACQUIRE_MEM";
		g_names[IT_REWIND]                    = "IT_REWIND";
		g_names[IT_SET_CONFIG_REG]            = "IT_SET_CONFIG_REG";
		g_names[IT_SET_CONTEXT_REG]           = "IT_SET_CONTEXT_REG";
		g_names[IT_SET_SH_REG]                = "IT_SET_SH_REG";
		g_names[IT_SET_QUEUE_REG]             = "IT_SET_QUEUE_REG";
		g_names[IT_SET_UCONFIG_REG]           = "IT_SET_UCONFIG_REG";
		g_names[IT_WRITE_CONST_RAM]           = "IT_WRITE_CONST_RAM";
		g_names[IT_DUMP_CONST_RAM]            = "IT_DUMP_CONST_RAM";
		g_names[IT_INCREMENT_CE_COUNTER]      = "IT_INCREMENT_CE_COUNTER";
		g_names[IT_INCREMENT_DE_COUNTER]      = "IT_INCREMENT_DE_COUNTER";
		g_names[IT_WAIT_ON_CE_COUNTER]        = "IT_WAIT_ON_CE_COUNTER";
		g_names[IT_WAIT_ON_DE_COUNTER_DIFF]   = "IT_WAIT_ON_DE_COUNTER_DIFF";
		g_names[IT_DISPATCH_DRAW_PREAMBLE]    = "IT_DISPATCH_DRAW_PREAMBLE";
		g_names[IT_DISPATCH_DRAW]             = "IT_DISPATCH_DRAW";

		g_names_initialized = true;
	}
}

void DumpPm4PacketStream(Core::File* file, uint32_t* cmd_buffer, uint32_t start_dw, uint32_t num_dw)
{
	init_names();

	// db_dump();

	file->Printf("----- Buffer --- dwords: 0x%05" PRIx32 ", offset : %u, addr: %016" PRIx64 " ----- \n", num_dw, start_dw,
	             reinterpret_cast<uint64_t>(cmd_buffer));

	auto* cmd = cmd_buffer + start_dw;
	auto  dw  = num_dw;
	for (;;)
	{
		if (dw == 0)
		{
			break;
		}

		EXIT_NOT_IMPLEMENTED(dw < 2);
		EXIT_NOT_IMPLEMENTED(dw > num_dw);

		auto cmd_id = *cmd++;

		file->Printf("%05" PRIx32 " | 0x%08" PRIx32 " | ", start_dw, cmd_id);

		uint32_t len = 0;

		if ((cmd_id & 0xC0000000u) == 0xC0000000u)
		{
			bool sh_gx = (cmd_id & 0x2u) == 0;
			len        = ((cmd_id >> 16u) & 0x3fffu) + 1;
			uint8_t op = ((cmd_id >> 8u) & 0xffu);
			auto    r  = KYTY_PM4_R(cmd_id);

			file->Printf("%s %s(OP:0x%02" PRIx8 ") SH:%s CNT:%u\n", g_names[op], (op == IT_NOP ? g_r_names[r] : ""), op,
			             sh_gx ? "GX" : "CX", len);

			for (uint32_t i = 0; i < len; i++)
			{
				file->Printf("      | 0x%08" PRIx32 " | \n", cmd[i]);
			}

			if (op == IT_NOP && (r == R_CX_REGS_INDIRECT || r == R_SH_REGS_INDIRECT || r == R_UC_REGS_INDIRECT) && len == 3)
			{
				auto*    indirect_buffer = reinterpret_cast<uint32_t*>(cmd[1] | (static_cast<uint64_t>(cmd[2]) << 32u));
				uint32_t indirect_num_dw = cmd[0];

				for (uint32_t i = 0; i < indirect_num_dw; i++, indirect_buffer += 2)
				{
					file->Printf("      |            | offset = 0x%08" PRIx32 ", value = 0x%08" PRIx32 "\n", indirect_buffer[0],
					             indirect_buffer[1]);
				}
			}
		} else
		{
			file->Printf("?????\n");
		}

		cmd += len;
		dw -= len + 1;
		start_dw += len + 1;
	}
}

} // namespace Kyty::Libs::Graphics::Pm4

#endif // KYTY_EMU_ENABLED
