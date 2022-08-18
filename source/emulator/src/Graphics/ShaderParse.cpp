#include "Emulator/Graphics/ShaderParse.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"

#include "Emulator/Config.h"
#include "Emulator/Graphics/Shader.h"

#ifdef KYTY_EMU_ENABLED

#define KYTY_SHADER_PARSER_ARGS                                                                                                            \
	[[maybe_unused]] uint32_t pc, [[maybe_unused]] const uint32_t *src, [[maybe_unused]] const uint32_t *buffer,                           \
	    [[maybe_unused]] ShaderCode *dst, [[maybe_unused]] bool next_gen
#define KYTY_SHADER_PARSER(f) static uint32_t f(KYTY_SHADER_PARSER_ARGS)
#define KYTY_CP_OP_PARSER_ARGS                                                                                                             \
	[[maybe_unused]] CommandProcessor *cp, [[maybe_unused]] uint32_t cmd_id, [[maybe_unused]] const uint32_t *buffer,                      \
	    [[maybe_unused]] uint32_t dw, [[maybe_unused]] uint32_t num_dw
#define KYTY_CP_OP_PARSER(f) static uint32_t f(KYTY_CP_OP_PARSER_ARGS)

#define KYTY_TYPE_STR(s) [[maybe_unused]] static const char* type_str = s;
#define KYTY_NI(i)                                                                                                                         \
	printf("%s", dst->DbgDump().c_str());                                                                                                  \
	EXIT("unknown %s instruction %s, opcode = 0x%" PRIx32 " at addr 0x%08" PRIx32 " (hash0 = 0x%08" PRIx32 ", crc32 = 0x%08" PRIx32 ")\n", \
	     type_str, i, opcode, pc, dst->GetHash0(), dst->GetCrc32());
#define KYTY_UNKNOWN_OP()                                                                                                                  \
	printf("%s", dst->DbgDump().c_str());                                                                                                  \
	EXIT("unknown %s opcode: 0x%" PRIx32 " at addr 0x%08" PRIx32 " (hash0 = 0x%08" PRIx32 ", crc32 = 0x%08" PRIx32 ")\n", type_str,        \
	     opcode, pc, dst->GetHash0(), dst->GetCrc32());

namespace Kyty::Libs::Graphics {

static ShaderOperand operand_parse(uint32_t code)
{
	ShaderOperand ret;

	ret.size = 1;

	if (code >= 0 && code <= 103)
	{
		ret.type        = ShaderOperandType::Sgpr;
		ret.register_id = static_cast<int>(code);
	} else if (code >= 128 && code <= 192)
	{
		ret.type       = ShaderOperandType::IntegerInlineConstant;
		ret.constant.i = static_cast<int>(code) - 128;
		ret.size       = 0;
	} else if (code >= 193 && code <= 208)
	{
		ret.type       = ShaderOperandType::IntegerInlineConstant;
		ret.constant.i = 192 - static_cast<int>(code);
		ret.size       = 0;
	} else if (code >= 240 && code <= 247)
	{
		static const float fv[] = {0.5f, -0.5f, 1.0f, -1.0f, 2.0f, -2.0f, 4.0f, -4.0f};
		ret.type                = ShaderOperandType::FloatInlineConstant;
		ret.constant.f          = fv[static_cast<int>(code) - 240];
		ret.size                = 0;
	} else if (code >= 256)
	{
		ret.type        = ShaderOperandType::Vgpr;
		ret.register_id = static_cast<int>(code) - 256;
	} else
	{
		switch (code)
		{
			case 106: ret.type = ShaderOperandType::VccLo; break;
			case 107: ret.type = ShaderOperandType::VccHi; break;
			case 124: ret.type = ShaderOperandType::M0; break;
			case 125: ret.type = ShaderOperandType::Null; break;
			case 126: ret.type = ShaderOperandType::ExecLo; break;
			case 127: ret.type = ShaderOperandType::ExecHi; break;
			case 252: ret.type = ShaderOperandType::ExecZ; break;
			case 255:
				ret.type = ShaderOperandType::LiteralConstant;
				ret.size = 0;
				break;
			default: EXIT("unknown operand: %u\n", code);
		}
	}

	return ret;
}

KYTY_SHADER_PARSER(shader_parse_sopc)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("sopc");

	uint32_t ssrc1  = (buffer[0] >> 8u) & 0xffu;
	uint32_t ssrc0  = (buffer[0] >> 0u) & 0xffu;
	uint32_t opcode = (buffer[0] >> 16u) & 0x7fu;

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.src[0]  = operand_parse(ssrc0);
	inst.src[1]  = operand_parse(ssrc1);
	inst.src_num = 2;

	uint32_t size = 1;

	if (inst.src[0].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[0].constant.u = buffer[size];
		size++;
	}

	if (inst.src[1].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[1].constant.u = buffer[size];
		size++;
	}

	inst.format = ShaderInstructionFormat::Ssrc0Ssrc1;

	switch (opcode)
	{
		case 0x00: inst.type = ShaderInstructionType::SCmpEqI32; break;
		case 0x01: inst.type = ShaderInstructionType::SCmpLgI32; break;
		case 0x02: inst.type = ShaderInstructionType::SCmpGtI32; break;
		case 0x03: inst.type = ShaderInstructionType::SCmpGeI32; break;
		case 0x04: inst.type = ShaderInstructionType::SCmpLtI32; break;
		case 0x05: inst.type = ShaderInstructionType::SCmpLeI32; break;
		case 0x06: inst.type = ShaderInstructionType::SCmpEqU32; break;
		case 0x07: inst.type = ShaderInstructionType::SCmpLgU32; break;
		case 0x08: inst.type = ShaderInstructionType::SCmpGtU32; break;
		case 0x09: inst.type = ShaderInstructionType::SCmpGeU32; break;
		case 0x0a: inst.type = ShaderInstructionType::SCmpLtU32; break;
		case 0x0b: inst.type = ShaderInstructionType::SCmpLeU32; break;
		case 0xC: KYTY_NI("s_bitcmp0_b32"); break;
		case 0xD: KYTY_NI("s_bitcmp1_b32"); break;
		case 0xE: KYTY_NI("s_bitcmp0_b64"); break;
		case 0xF: KYTY_NI("s_bitcmp1_b64"); break;
		case 0x10: KYTY_NI("s_setvskip"); break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_sopk)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("sopk");

	uint32_t opcode = (buffer[0] >> 23u) & 0x1fu;
	auto     imm    = static_cast<int16_t>(buffer[0] >> 0u & 0xffffu);
	uint32_t sdst   = (buffer[0] >> 16u) & 0x7fu;

	ShaderInstruction inst;
	inst.pc  = pc;
	inst.dst = operand_parse(sdst);

	inst.format            = ShaderInstructionFormat::SVdstSVsrc0;
	inst.src[0].type       = ShaderOperandType::IntegerInlineConstant;
	inst.src[0].constant.i = imm;
	inst.src_num           = 1;

	switch (opcode)
	{
		case 0x00: inst.type = ShaderInstructionType::SMovkI32; break;

		case 0x02: KYTY_NI("s_cmovk_i32"); break;
		case 0x03: KYTY_NI("s_cmpk_eq_i32"); break;
		case 0x04: KYTY_NI("s_cmpk_lg_i32"); break;
		case 0x05: KYTY_NI("s_cmpk_gt_i32"); break;
		case 0x06: KYTY_NI("s_cmpk_ge_i32"); break;
		case 0x07: KYTY_NI("s_cmpk_lt_i32"); break;
		case 0x08: KYTY_NI("s_cmpk_le_i32"); break;
		case 0x09: KYTY_NI("s_cmpk_eq_u32"); break;
		case 0x0A: KYTY_NI("s_cmpk_lg_u32"); break;
		case 0x0B: KYTY_NI("s_cmpk_gt_u32"); break;
		case 0x0C: KYTY_NI("s_cmpk_ge_u32"); break;
		case 0x0D: KYTY_NI("s_cmpk_lt_u32"); break;
		case 0x0E: KYTY_NI("s_cmpk_le_u32"); break;
		case 0x0F: KYTY_NI("s_addk_i32"); break;
		case 0x10: inst.type = ShaderInstructionType::SMulkI32; break;
		case 0x11: KYTY_NI("s_cbranch_i_fork"); break;
		case 0x12: KYTY_NI("s_getreg_b32"); break;
		case 0x13: KYTY_NI("s_setreg_b32"); break;
		case 0x14: KYTY_NI("s_getreg_regrd_b32"); break;
		case 0x15: KYTY_NI("s_setreg_imm32_b32"); break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return 1;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_sopp)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("sopp");

	uint32_t opcode = (buffer[0] >> 16u) & 0x7fu;
	uint32_t simm   = (buffer[0] >> 0u) & 0xffffu;

	ShaderInstruction inst;
	inst.pc = pc;

	inst.format            = ShaderInstructionFormat::Label;
	inst.src[0].type       = ShaderOperandType::LiteralConstant;
	inst.src[0].constant.i = static_cast<int16_t>(simm) * 4;
	inst.src_num           = 1;

	switch (opcode)
	{
		case 0x01:
			inst.type    = ShaderInstructionType::SEndpgm;
			inst.format  = ShaderInstructionFormat::Empty;
			inst.src_num = 0;
			break;
		case 0x02: inst.type = ShaderInstructionType::SBranch; break;
		case 0x04: inst.type = ShaderInstructionType::SCbranchScc0; break;
		case 0x05: inst.type = ShaderInstructionType::SCbranchScc1; break;
		case 0x06: inst.type = ShaderInstructionType::SCbranchVccz; break;
		case 0x07: inst.type = ShaderInstructionType::SCbranchVccnz; break;
		case 0x08: inst.type = ShaderInstructionType::SCbranchExecz; break;
		case 0x0c:
			inst.type              = ShaderInstructionType::SWaitcnt;
			inst.format            = ShaderInstructionFormat::Imm;
			inst.src[0].type       = ShaderOperandType::LiteralConstant;
			inst.src[0].constant.u = simm;
			inst.src_num           = 1;
			break;
		case 0x10:
			inst.type              = ShaderInstructionType::SSendmsg;
			inst.format            = ShaderInstructionFormat::Imm;
			inst.src[0].type       = ShaderOperandType::LiteralConstant;
			inst.src[0].constant.u = simm;
			inst.src_num           = 1;
			break;
		case 0x20:
			inst.type              = ShaderInstructionType::SInstPrefetch;
			inst.format            = ShaderInstructionFormat::Imm;
			inst.src[0].type       = ShaderOperandType::LiteralConstant;
			inst.src[0].constant.u = simm;
			inst.src_num           = 1;
			break;

		case 0x0: KYTY_NI("s_nop"); break;
		case 0x9: KYTY_NI("s_cbranch_execnz"); break;
		case 0xA: KYTY_NI("s_barrier"); break;
		case 0xB: KYTY_NI("s_setkill"); break;
		case 0xD: KYTY_NI("s_sethalt"); break;
		case 0xE: KYTY_NI("s_sleep"); break;
		case 0xF: KYTY_NI("s_setprio"); break;
		case 0x11: KYTY_NI("s_sendmsghalt"); break;
		case 0x12: KYTY_NI("s_trap"); break;
		case 0x13: KYTY_NI("s_icache_inv"); break;
		case 0x14: KYTY_NI("s_incperflevel"); break;
		case 0x15: KYTY_NI("s_decperflevel"); break;
		case 0x16: KYTY_NI("s_ttracedata"); break;
		case 0x17: KYTY_NI("s_cbranch_cdbgsys"); break;
		case 0x18: KYTY_NI("s_cbranch_cdbguser"); break;
		case 0x19: KYTY_NI("s_cbranch_cdbgsys_or_user"); break;
		case 0x1A: KYTY_NI("s_cbranch_cdbgsys_and_user"); break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	if (inst.type == ShaderInstructionType::SCbranchScc0 || inst.type == ShaderInstructionType::SCbranchScc1 ||
	    inst.type == ShaderInstructionType::SCbranchVccz || inst.type == ShaderInstructionType::SCbranchVccnz ||
	    inst.type == ShaderInstructionType::SCbranchExecz || inst.type == ShaderInstructionType::SBranch)
	{
		dst->GetLabels().Add(ShaderLabel(inst));

		if (inst.type != ShaderInstructionType::SBranch)
		{
			dst->GetIndirectLabels().Add(ShaderLabel(inst.pc + 4, inst.pc));
		}
	}

	return 1;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_sop1)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("sop1");

	uint32_t opcode = (buffer[0] >> 8u) & 0xffu;
	uint32_t ssrc0  = (buffer[0] >> 0u) & 0xffu;
	uint32_t sdst   = (buffer[0] >> 16u) & 0x7fu;

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.src[0]  = operand_parse(ssrc0);
	inst.src_num = 1;
	inst.dst     = operand_parse(sdst);

	uint32_t size = 1;

	if (inst.src[0].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[0].constant.u = buffer[size];
		size++;
	}

	switch (opcode)
	{
		case 0x03:
			inst.type   = ShaderInstructionType::SMovB32;
			inst.format = ShaderInstructionFormat::SVdstSVsrc0;
			break;
		case 0x04:
			inst.type        = ShaderInstructionType::SMovB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			break;
		case 0x05: KYTY_NI("s_cmov_b32"); break;
		case 0x06: KYTY_NI("s_cmov_b64"); break;
		case 0x07: KYTY_NI("s_not_b32"); break;
		case 0x08: KYTY_NI("s_not_b64"); break;
		case 0x09: KYTY_NI("s_wqm_b32"); break;
		case 0x0a:
			inst.type        = ShaderInstructionType::SWqmB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			break;
		case 0x0B: KYTY_NI("s_brev_b32"); break;
		case 0x0C: KYTY_NI("s_brev_b64"); break;
		case 0x0D: KYTY_NI("s_bcnt0_i32_b32"); break;
		case 0x0E: KYTY_NI("s_bcnt0_i32_b64"); break;
		case 0x0F: KYTY_NI("s_bcnt1_i32_b32"); break;
		case 0x10: KYTY_NI("s_bcnt1_i32_b64"); break;
		case 0x11: KYTY_NI("s_ff0_i32_b32"); break;
		case 0x12: KYTY_NI("s_ff0_i32_b64"); break;
		case 0x13: KYTY_NI("s_ff1_i32_b32"); break;
		case 0x14: KYTY_NI("s_ff1_i32_b64"); break;
		case 0x15: KYTY_NI("s_flbit_i32_b32"); break;
		case 0x16: KYTY_NI("s_flbit_i32_b64"); break;
		case 0x17: KYTY_NI("s_flbit_i32"); break;
		case 0x18: KYTY_NI("s_flbit_i32_i64"); break;
		case 0x19: KYTY_NI("s_sext_i32_i8"); break;
		case 0x1A: KYTY_NI("s_sext_i32_i16"); break;
		case 0x1B: KYTY_NI("s_bitset0_b32"); break;
		case 0x1C: KYTY_NI("s_bitset0_b64"); break;
		case 0x1D: KYTY_NI("s_bitset1_b32"); break;
		case 0x1E: KYTY_NI("s_bitset1_b64"); break;
		case 0x1F: KYTY_NI("s_getpc_b64"); break;
		case 0x20:
			inst.type        = ShaderInstructionType::SSetpcB64;
			inst.format      = ShaderInstructionFormat::Saddr;
			inst.src[0].size = 2;
			break;
		case 0x21:
			inst.type        = ShaderInstructionType::SSwappcB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02;
			inst.src[0].size = 2;
			inst.dst.size    = 2;
			break;
		case 0x22: KYTY_NI("s_rfe_b64"); break;
		case 0x24:
			inst.type        = ShaderInstructionType::SAndSaveexecB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			break;
		case 0x25: KYTY_NI("s_or_saveexec_b64"); break;
		case 0x26: KYTY_NI("s_xor_saveexec_b64"); break;
		case 0x27: KYTY_NI("s_andn2_saveexec_b64"); break;
		case 0x28: KYTY_NI("s_orn2_saveexec_b64"); break;
		case 0x29: KYTY_NI("s_nand_saveexec_b64"); break;
		case 0x2A: KYTY_NI("s_nor_saveexec_b64"); break;
		case 0x2B: KYTY_NI("s_xnor_saveexec_b64"); break;
		case 0x2C: KYTY_NI("s_quadmask_b32"); break;
		case 0x2D: KYTY_NI("s_quadmask_b64"); break;
		case 0x2E: KYTY_NI("s_movrels_b32"); break;
		case 0x2F: KYTY_NI("s_movrels_b64"); break;
		case 0x30: KYTY_NI("s_movreld_b32"); break;
		case 0x31: KYTY_NI("s_movreld_b64"); break;
		case 0x32: KYTY_NI("s_cbranch_join"); break;
		case 0x33: KYTY_NI("s_mov_regrd_b32"); break;
		case 0x34: KYTY_NI("s_abs_i32"); break;
		case 0x35: KYTY_NI("s_mov_fed_b32"); break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_sop2)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("sop2");

	uint32_t opcode = (buffer[0] >> 23u) & 0x7fu;

	switch (opcode)
	{
		case 0x7d: return shader_parse_sop1(pc, src, buffer, dst, next_gen); break;
		case 0x7e: return shader_parse_sopc(pc, src, buffer, dst, next_gen); break;
		case 0x7f: return shader_parse_sopp(pc, src, buffer, dst, next_gen); break;
		default: break;
	}

	if (opcode >= 0x60)
	{
		return shader_parse_sopk(pc, src, buffer, dst, next_gen);
	}

	uint32_t ssrc1 = (buffer[0] >> 8u) & 0xffu;
	uint32_t ssrc0 = (buffer[0] >> 0u) & 0xffu;
	uint32_t sdst  = (buffer[0] >> 16u) & 0x7fu;

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.src[0]  = operand_parse(ssrc0);
	inst.src[1]  = operand_parse(ssrc1);
	inst.src_num = 2;
	inst.dst     = operand_parse(sdst);

	uint32_t size = 1;

	if (inst.src[0].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[0].constant.u = buffer[size];
		size++;
	}

	if (inst.src[1].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[1].constant.u = buffer[size];
		size++;
	}

	inst.format = ShaderInstructionFormat::SVdstSVsrc0SVsrc1;

	switch (opcode)
	{
		case 0x00: inst.type = ShaderInstructionType::SAddU32; break;
		case 0x01: KYTY_NI("s_sub_u32"); break;
		case 0x02: inst.type = ShaderInstructionType::SAddI32; break;
		case 0x03: inst.type = ShaderInstructionType::SSubI32; break;
		case 0x04: inst.type = ShaderInstructionType::SAddcU32; break;
		case 0x05: KYTY_NI("s_subb_u32"); break;
		case 0x06: KYTY_NI("s_min_i32"); break;
		case 0x07: KYTY_NI("s_min_u32"); break;
		case 0x08: KYTY_NI("s_max_i32"); break;
		case 0x09: KYTY_NI("s_max_u32"); break;
		case 0x0a: inst.type = ShaderInstructionType::SCselectB32; break;
		case 0x0b:
			inst.type        = ShaderInstructionType::SCselectB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc12;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			inst.src[1].size = 2;
			break;
		case 0x0e: inst.type = ShaderInstructionType::SAndB32; break;
		case 0x0f:
			inst.type        = ShaderInstructionType::SAndB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc12;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			inst.src[1].size = 2;
			break;
		case 0x10: inst.type = ShaderInstructionType::SOrB32; break;
		case 0x11:
			inst.type        = ShaderInstructionType::SOrB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc12;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			inst.src[1].size = 2;
			break;
		case 0x12: KYTY_NI("s_xor_b32"); break;
		case 0x13:
			inst.type        = ShaderInstructionType::SXorB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc12;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			inst.src[1].size = 2;
			break;
		case 0x14: KYTY_NI("s_andn2_b32"); break;
		case 0x15:
			inst.type        = ShaderInstructionType::SAndn2B64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc12;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			inst.src[1].size = 2;
			break;
		case 0x16: KYTY_NI("s_orn2_b32"); break;
		case 0x17:
			inst.type        = ShaderInstructionType::SOrn2B64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc12;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			inst.src[1].size = 2;
			break;
		case 0x18: KYTY_NI("s_nand_b32"); break;
		case 0x19:
			inst.type        = ShaderInstructionType::SNandB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc12;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			inst.src[1].size = 2;
			break;
		case 0x1A: KYTY_NI("s_nor_b32"); break;
		case 0x1b:
			inst.type        = ShaderInstructionType::SNorB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc12;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			inst.src[1].size = 2;
			break;
		case 0x1C: KYTY_NI("s_xnor_b32"); break;
		case 0x1d:
			inst.type        = ShaderInstructionType::SXnorB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc12;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			inst.src[1].size = 2;
			break;
		case 0x1e: inst.type = ShaderInstructionType::SLshlB32; break;
		case 0x1f:
			inst.type        = ShaderInstructionType::SLshlB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc1;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			break;
		case 0x20: inst.type = ShaderInstructionType::SLshrB32; break;
		case 0x21:
			inst.type        = ShaderInstructionType::SLshrB64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc1;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			break;
		case 0x22: KYTY_NI("s_ashr_i32"); break;
		case 0x23: KYTY_NI("s_ashr_i64"); break;
		case 0x24: inst.type = ShaderInstructionType::SBfmB32; break;
		case 0x25: KYTY_NI("s_bfm_b64"); break;
		case 0x26: inst.type = ShaderInstructionType::SMulI32; break;
		case 0x27: inst.type = ShaderInstructionType::SBfeU32; break;
		case 0x28: KYTY_NI("s_bfe_i32"); break;
		case 0x29:
			inst.type        = ShaderInstructionType::SBfeU64;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc1;
			inst.dst.size    = 2;
			inst.src[0].size = 2;
			break;
		case 0x2A: KYTY_NI("s_bfe_i64"); break;
		case 0x2B: KYTY_NI("s_cbranch_g_fork"); break;
		case 0x2C: KYTY_NI("s_absdiff_i32"); break;
		case 0x31:
			EXIT_NOT_IMPLEMENTED(!next_gen);
			inst.type = ShaderInstructionType::SLshl4AddU32;
			break;
		case 0x32: KYTY_NI("s_pack_ll_b32_b16"); break;
		case 0x33: KYTY_NI("s_pack_lh_b32_b16"); break;
		case 0x34: KYTY_NI("s_pack_hh_b32_b16"); break;
		case 0x35: inst.type = ShaderInstructionType::SMulHiU32; break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity,readability-function-size,google-readability-function-size,hicpp-function-size)
KYTY_SHADER_PARSER(shader_parse_vopc)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("vopc");

	uint32_t opcode = (buffer[0] >> 17u) & 0xffu;
	uint32_t src0   = (buffer[0] >> 0u) & 0x1ffu;
	uint32_t vsrc1  = (buffer[0] >> 9u) & 0xffu;

	bool sdwa = (src0 == 249);

	uint32_t size = (sdwa ? 2 : 1);

	src0               = (sdwa ? (buffer[1] >> 0u) & 0xffu : src0);
	uint32_t sdst      = (sdwa ? (buffer[1] >> 8u) & 0x7fu : 0);
	uint32_t sd        = (sdwa ? (buffer[1] >> 15u) & 0x1u : 0);
	uint32_t src0_sel  = (sdwa ? (buffer[1] >> 16u) & 0x7u : 6);
	uint32_t src0_sext = (sdwa ? (buffer[1] >> 19u) & 0x1u : 0);
	uint32_t src0_neg  = (sdwa ? (buffer[1] >> 20u) & 0x1u : 0);
	uint32_t src0_abs  = (sdwa ? (buffer[1] >> 21u) & 0x1u : 0);
	uint32_t s0        = (sdwa ? (buffer[1] >> 23u) & 0x1u : 1);
	uint32_t src1_sel  = (sdwa ? (buffer[1] >> 24u) & 0x7u : 6);
	uint32_t src1_sext = (sdwa ? (buffer[1] >> 27u) & 0x1u : 0);
	uint32_t src1_neg  = (sdwa ? (buffer[1] >> 28u) & 0x1u : 0);
	uint32_t src1_abs  = (sdwa ? (buffer[1] >> 29u) & 0x1u : 0);
	uint32_t s1        = (sdwa ? (buffer[1] >> 31u) & 0x1u : 0);

	EXIT_NOT_IMPLEMENTED(src0_sel != 6);
	EXIT_NOT_IMPLEMENTED(src0_sext != 0);
	EXIT_NOT_IMPLEMENTED(src0_neg != 0);
	EXIT_NOT_IMPLEMENTED(src0_abs != 0);
	EXIT_NOT_IMPLEMENTED(src1_sel != 6);
	EXIT_NOT_IMPLEMENTED(src1_sext != 0);
	EXIT_NOT_IMPLEMENTED(src1_neg != 0);
	EXIT_NOT_IMPLEMENTED(src1_abs != 0);

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.src[0]  = operand_parse(src0 + (s0 == 0 ? 256 : 0));
	inst.src[1]  = operand_parse(vsrc1 + (s1 == 0 ? 256 : 0));
	inst.src_num = 2;

	if (inst.src[0].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[0].constant.u = buffer[size];
		size++;
	}

	inst.format = ShaderInstructionFormat::SmaskVsrc0Vsrc1;
	if (sd == 0)
	{
		inst.dst.type = ShaderOperandType::VccLo;
	} else
	{
		inst.dst = operand_parse(sdst);
	}
	inst.dst.size = 2;

	switch (opcode)
	{
		case 0x00: inst.type = ShaderInstructionType::VCmpFF32; break;
		case 0x01: inst.type = ShaderInstructionType::VCmpLtF32; break;
		case 0x02: inst.type = ShaderInstructionType::VCmpEqF32; break;
		case 0x03: inst.type = ShaderInstructionType::VCmpLeF32; break;
		case 0x04: inst.type = ShaderInstructionType::VCmpGtF32; break;
		case 0x05: inst.type = ShaderInstructionType::VCmpLgF32; break;
		case 0x06: inst.type = ShaderInstructionType::VCmpGeF32; break;
		case 0x07: inst.type = ShaderInstructionType::VCmpOF32; break;
		case 0x08: inst.type = ShaderInstructionType::VCmpUF32; break;
		case 0x09: inst.type = ShaderInstructionType::VCmpNgeF32; break;
		case 0x0a: inst.type = ShaderInstructionType::VCmpNlgF32; break;
		case 0x0b: inst.type = ShaderInstructionType::VCmpNgtF32; break;
		case 0x0c: inst.type = ShaderInstructionType::VCmpNleF32; break;
		case 0x0d: inst.type = ShaderInstructionType::VCmpNeqF32; break;
		case 0x0e: inst.type = ShaderInstructionType::VCmpNltF32; break;
		case 0x0f: inst.type = ShaderInstructionType::VCmpTruF32; break;
		case 0x10: KYTY_NI("v_cmpx_f_f32"); break;
		case 0x11: inst.type = ShaderInstructionType::VCmpxLtF32; break;
		case 0x12: KYTY_NI("v_cmpx_eq_f32"); break;
		case 0x13: KYTY_NI("v_cmpx_le_f32"); break;
		case 0x14: inst.type = ShaderInstructionType::VCmpxGtF32; break;
		case 0x15: KYTY_NI("v_cmpx_lg_f32"); break;
		case 0x16: KYTY_NI("v_cmpx_ge_f32"); break;
		case 0x17: KYTY_NI("v_cmpx_o_f32"); break;
		case 0x18: KYTY_NI("v_cmpx_u_f32"); break;
		case 0x19: KYTY_NI("v_cmpx_nge_f32"); break;
		case 0x1A: KYTY_NI("v_cmpx_nlg_f32"); break;
		case 0x1B: KYTY_NI("v_cmpx_ngt_f32"); break;
		case 0x1C: KYTY_NI("v_cmpx_nle_f32"); break;
		case 0x1d: inst.type = ShaderInstructionType::VCmpxNeqF32; break;
		case 0x1E: KYTY_NI("v_cmpx_nlt_f32"); break;
		case 0x1F: KYTY_NI("v_cmpx_tru_f32"); break;
		case 0x20: KYTY_NI("v_cmp_f_f64"); break;
		case 0x21: KYTY_NI("v_cmp_lt_f64"); break;
		case 0x22: KYTY_NI("v_cmp_eq_f64"); break;
		case 0x23: KYTY_NI("v_cmp_le_f64"); break;
		case 0x24: KYTY_NI("v_cmp_gt_f64"); break;
		case 0x25: KYTY_NI("v_cmp_lg_f64"); break;
		case 0x26: KYTY_NI("v_cmp_ge_f64"); break;
		case 0x27: KYTY_NI("v_cmp_o_f64"); break;
		case 0x28: KYTY_NI("v_cmp_u_f64"); break;
		case 0x29: KYTY_NI("v_cmp_nge_f64"); break;
		case 0x2A: KYTY_NI("v_cmp_nlg_f64"); break;
		case 0x2B: KYTY_NI("v_cmp_ngt_f64"); break;
		case 0x2C: KYTY_NI("v_cmp_nle_f64"); break;
		case 0x2D: KYTY_NI("v_cmp_neq_f64"); break;
		case 0x2E: KYTY_NI("v_cmp_nlt_f64"); break;
		case 0x2F: KYTY_NI("v_cmp_tru_f64"); break;
		case 0x30: KYTY_NI("v_cmpx_f_f64"); break;
		case 0x31: KYTY_NI("v_cmpx_lt_f64"); break;
		case 0x32: KYTY_NI("v_cmpx_eq_f64"); break;
		case 0x33: KYTY_NI("v_cmpx_le_f64"); break;
		case 0x34: KYTY_NI("v_cmpx_gt_f64"); break;
		case 0x35: KYTY_NI("v_cmpx_lg_f64"); break;
		case 0x36: KYTY_NI("v_cmpx_ge_f64"); break;
		case 0x37: KYTY_NI("v_cmpx_o_f64"); break;
		case 0x38: KYTY_NI("v_cmpx_u_f64"); break;
		case 0x39: KYTY_NI("v_cmpx_nge_f64"); break;
		case 0x3A: KYTY_NI("v_cmpx_nlg_f64"); break;
		case 0x3B: KYTY_NI("v_cmpx_ngt_f64"); break;
		case 0x3C: KYTY_NI("v_cmpx_nle_f64"); break;
		case 0x3D: KYTY_NI("v_cmpx_neq_f64"); break;
		case 0x3E: KYTY_NI("v_cmpx_nlt_f64"); break;
		case 0x3F: KYTY_NI("v_cmpx_tru_f64"); break;
		case 0x40: KYTY_NI("v_cmps_f_f32"); break;
		case 0x41: KYTY_NI("v_cmps_lt_f32"); break;
		case 0x42: KYTY_NI("v_cmps_eq_f32"); break;
		case 0x43: KYTY_NI("v_cmps_le_f32"); break;
		case 0x44: KYTY_NI("v_cmps_gt_f32"); break;
		case 0x45: KYTY_NI("v_cmps_lg_f32"); break;
		case 0x46: KYTY_NI("v_cmps_ge_f32"); break;
		case 0x47: KYTY_NI("v_cmps_o_f32"); break;
		case 0x48: KYTY_NI("v_cmps_u_f32"); break;
		case 0x49: KYTY_NI("v_cmps_nge_f32"); break;
		case 0x4A: KYTY_NI("v_cmps_nlg_f32"); break;
		case 0x4B: KYTY_NI("v_cmps_ngt_f32"); break;
		case 0x4C: KYTY_NI("v_cmps_nle_f32"); break;
		case 0x4D: KYTY_NI("v_cmps_neq_f32"); break;
		case 0x4E: KYTY_NI("v_cmps_nlt_f32"); break;
		case 0x4F: KYTY_NI("v_cmps_tru_f32"); break;
		case 0x50: KYTY_NI("v_cmpsx_f_f32"); break;
		case 0x51: KYTY_NI("v_cmpsx_lt_f32"); break;
		case 0x52: KYTY_NI("v_cmpsx_eq_f32"); break;
		case 0x53: KYTY_NI("v_cmpsx_le_f32"); break;
		case 0x54: KYTY_NI("v_cmpsx_gt_f32"); break;
		case 0x55: KYTY_NI("v_cmpsx_lg_f32"); break;
		case 0x56: KYTY_NI("v_cmpsx_ge_f32"); break;
		case 0x57: KYTY_NI("v_cmpsx_o_f32"); break;
		case 0x58: KYTY_NI("v_cmpsx_u_f32"); break;
		case 0x59: KYTY_NI("v_cmpsx_nge_f32"); break;
		case 0x5A: KYTY_NI("v_cmpsx_nlg_f32"); break;
		case 0x5B: KYTY_NI("v_cmpsx_ngt_f32"); break;
		case 0x5C: KYTY_NI("v_cmpsx_nle_f32"); break;
		case 0x5D: KYTY_NI("v_cmpsx_neq_f32"); break;
		case 0x5E: KYTY_NI("v_cmpsx_nlt_f32"); break;
		case 0x5F: KYTY_NI("v_cmpsx_tru_f32"); break;
		case 0x60: KYTY_NI("v_cmps_f_f64"); break;
		case 0x61: KYTY_NI("v_cmps_lt_f64"); break;
		case 0x62: KYTY_NI("v_cmps_eq_f64"); break;
		case 0x63: KYTY_NI("v_cmps_le_f64"); break;
		case 0x64: KYTY_NI("v_cmps_gt_f64"); break;
		case 0x65: KYTY_NI("v_cmps_lg_f64"); break;
		case 0x66: KYTY_NI("v_cmps_ge_f64"); break;
		case 0x67: KYTY_NI("v_cmps_o_f64"); break;
		case 0x68: KYTY_NI("v_cmps_u_f64"); break;
		case 0x69: KYTY_NI("v_cmps_nge_f64"); break;
		case 0x6A: KYTY_NI("v_cmps_nlg_f64"); break;
		case 0x6B: KYTY_NI("v_cmps_ngt_f64"); break;
		case 0x6C: KYTY_NI("v_cmps_nle_f64"); break;
		case 0x6D: KYTY_NI("v_cmps_neq_f64"); break;
		case 0x6E: KYTY_NI("v_cmps_nlt_f64"); break;
		case 0x6F: KYTY_NI("v_cmps_tru_f64"); break;
		case 0x70: KYTY_NI("v_cmpsx_f_f64"); break;
		case 0x71: KYTY_NI("v_cmpsx_lt_f64"); break;
		case 0x72: KYTY_NI("v_cmpsx_eq_f64"); break;
		case 0x73: KYTY_NI("v_cmpsx_le_f64"); break;
		case 0x74: KYTY_NI("v_cmpsx_gt_f64"); break;
		case 0x75: KYTY_NI("v_cmpsx_lg_f64"); break;
		case 0x76: KYTY_NI("v_cmpsx_ge_f64"); break;
		case 0x77: KYTY_NI("v_cmpsx_o_f64"); break;
		case 0x78: KYTY_NI("v_cmpsx_u_f64"); break;
		case 0x79: KYTY_NI("v_cmpsx_nge_f64"); break;
		case 0x7A: KYTY_NI("v_cmpsx_nlg_f64"); break;
		case 0x7B: KYTY_NI("v_cmpsx_ngt_f64"); break;
		case 0x7C: KYTY_NI("v_cmpsx_nle_f64"); break;
		case 0x7D: KYTY_NI("v_cmpsx_neq_f64"); break;
		case 0x7E: KYTY_NI("v_cmpsx_nlt_f64"); break;
		case 0x7F: KYTY_NI("v_cmpsx_tru_f64"); break;
		case 0x80: inst.type = ShaderInstructionType::VCmpFI32; break;
		case 0x81: inst.type = ShaderInstructionType::VCmpLtI32; break;
		case 0x82: inst.type = ShaderInstructionType::VCmpEqI32; break;
		case 0x83: inst.type = ShaderInstructionType::VCmpLeI32; break;
		case 0x84: inst.type = ShaderInstructionType::VCmpGtI32; break;
		case 0x85: inst.type = ShaderInstructionType::VCmpNeI32; break;
		case 0x86: inst.type = ShaderInstructionType::VCmpGeI32; break;
		case 0x87: inst.type = ShaderInstructionType::VCmpTI32; break;
		case 0x88: KYTY_NI("v_cmp_class_f32"); break;
		case 0x89: KYTY_NI("v_cmp_lt_i16"); break;
		case 0x8A: KYTY_NI("v_cmp_eq_i16"); break;
		case 0x8B: KYTY_NI("v_cmp_le_i16"); break;
		case 0x8C: KYTY_NI("v_cmp_gt_i16"); break;
		case 0x8D: KYTY_NI("v_cmp_ne_i16"); break;
		case 0x8E: KYTY_NI("v_cmp_ge_i16"); break;
		case 0x8F: KYTY_NI("v_cmp_class_f16"); break;
		case 0x90: KYTY_NI("v_cmpx_f_i32"); break;
		case 0x91: KYTY_NI("v_cmpx_lt_i32"); break;
		case 0x92: KYTY_NI("v_cmpx_eq_i32"); break;
		case 0x93: KYTY_NI("v_cmpx_le_i32"); break;
		case 0x94: KYTY_NI("v_cmpx_gt_i32"); break;
		case 0x95: KYTY_NI("v_cmpx_ne_i32"); break;
		case 0x96: KYTY_NI("v_cmpx_ge_i32"); break;
		case 0x97: KYTY_NI("v_cmpx_t_i32"); break;
		case 0x98: KYTY_NI("v_cmpx_class_f32"); break;
		case 0x99: KYTY_NI("v_cmpx_lt_i16"); break;
		case 0x9A: KYTY_NI("v_cmpx_eq_i16"); break;
		case 0x9B: KYTY_NI("v_cmpx_le_i16"); break;
		case 0x9C: KYTY_NI("v_cmpx_gt_i16"); break;
		case 0x9D: KYTY_NI("v_cmpx_ne_i16"); break;
		case 0x9E: KYTY_NI("v_cmpx_ge_i16"); break;
		case 0x9F: KYTY_NI("v_cmpx_class_f16"); break;
		case 0xA0: KYTY_NI("v_cmp_f_i64"); break;
		case 0xA1: KYTY_NI("v_cmp_lt_i64"); break;
		case 0xA2: KYTY_NI("v_cmp_eq_i64"); break;
		case 0xA3: KYTY_NI("v_cmp_le_i64"); break;
		case 0xA4: KYTY_NI("v_cmp_gt_i64"); break;
		case 0xA5: KYTY_NI("v_cmp_ne_i64"); break;
		case 0xA6: KYTY_NI("v_cmp_ge_i64"); break;
		case 0xA7: KYTY_NI("v_cmp_t_i64"); break;
		case 0xA8: KYTY_NI("v_cmp_class_f64"); break;
		case 0xA9: KYTY_NI("v_cmp_lt_u16"); break;
		case 0xAA: KYTY_NI("v_cmp_eq_u16"); break;
		case 0xAB: KYTY_NI("v_cmp_le_u16"); break;
		case 0xAC: KYTY_NI("v_cmp_gt_u16"); break;
		case 0xAD: KYTY_NI("v_cmp_ne_u16"); break;
		case 0xAE: KYTY_NI("v_cmp_ge_u16"); break;
		case 0xB0: KYTY_NI("v_cmpx_f_i64"); break;
		case 0xB1: KYTY_NI("v_cmpx_lt_i64"); break;
		case 0xB2: KYTY_NI("v_cmpx_eq_i64"); break;
		case 0xB3: KYTY_NI("v_cmpx_le_i64"); break;
		case 0xB4: KYTY_NI("v_cmpx_gt_i64"); break;
		case 0xB5: KYTY_NI("v_cmpx_ne_i64"); break;
		case 0xB6: KYTY_NI("v_cmpx_ge_i64"); break;
		case 0xB7: KYTY_NI("v_cmpx_t_i64"); break;
		case 0xB8: KYTY_NI("v_cmpx_class_f64"); break;
		case 0xB9: KYTY_NI("v_cmpx_lt_u16"); break;
		case 0xBA: KYTY_NI("v_cmpx_eq_u16"); break;
		case 0xBB: KYTY_NI("v_cmpx_le_u16"); break;
		case 0xBC: KYTY_NI("v_cmpx_gt_u16"); break;
		case 0xBD: KYTY_NI("v_cmpx_ne_u16"); break;
		case 0xBE: KYTY_NI("v_cmpx_ge_u16"); break;
		case 0xc0: inst.type = ShaderInstructionType::VCmpFU32; break;
		case 0xc1: inst.type = ShaderInstructionType::VCmpLtU32; break;
		case 0xc2: inst.type = ShaderInstructionType::VCmpEqU32; break;
		case 0xc3: inst.type = ShaderInstructionType::VCmpLeU32; break;
		case 0xc4: inst.type = ShaderInstructionType::VCmpGtU32; break;
		case 0xc5: inst.type = ShaderInstructionType::VCmpNeU32; break;
		case 0xc6: inst.type = ShaderInstructionType::VCmpGeU32; break;
		case 0xc7: inst.type = ShaderInstructionType::VCmpTU32; break;
		case 0xC8: KYTY_NI("v_cmp_f_f16"); break;
		case 0xC9: KYTY_NI("v_cmp_lt_f16"); break;
		case 0xCA: KYTY_NI("v_cmp_eq_f16"); break;
		case 0xCB: KYTY_NI("v_cmp_le_f16"); break;
		case 0xCC: KYTY_NI("v_cmp_gt_f16"); break;
		case 0xCD: KYTY_NI("v_cmp_lg_f16"); break;
		case 0xCE: KYTY_NI("v_cmp_ge_f16"); break;
		case 0xCF: KYTY_NI("v_cmp_o_f16"); break;
		case 0xD0: KYTY_NI("v_cmpx_f_u32"); break;
		case 0xD1: KYTY_NI("v_cmpx_lt_u32"); break;
		case 0xd2: inst.type = ShaderInstructionType::VCmpxEqU32; break;
		case 0xD3: KYTY_NI("v_cmpx_le_u32"); break;
		case 0xd4: inst.type = ShaderInstructionType::VCmpxGtU32; break;
		case 0xd5: inst.type = ShaderInstructionType::VCmpxNeU32; break;
		case 0xd6: inst.type = ShaderInstructionType::VCmpxGeU32; break;
		case 0xD7: KYTY_NI("v_cmpx_t_u32"); break;
		case 0xD8: KYTY_NI("v_cmpx_f_f16"); break;
		case 0xD9: KYTY_NI("v_cmpx_lt_f16"); break;
		case 0xDA: KYTY_NI("v_cmpx_eq_f16"); break;
		case 0xDB: KYTY_NI("v_cmpx_le_f16"); break;
		case 0xDC: KYTY_NI("v_cmpx_gt_f16"); break;
		case 0xDD: KYTY_NI("v_cmpx_lg_f16"); break;
		case 0xDE: KYTY_NI("v_cmpx_ge_f16"); break;
		case 0xDF: KYTY_NI("v_cmpx_o_f16"); break;
		case 0xE0: KYTY_NI("v_cmp_f_u64"); break;
		case 0xE1: KYTY_NI("v_cmp_lt_u64"); break;
		case 0xE2: KYTY_NI("v_cmp_eq_u64"); break;
		case 0xE3: KYTY_NI("v_cmp_le_u64"); break;
		case 0xE4: KYTY_NI("v_cmp_gt_u64"); break;
		case 0xE5: KYTY_NI("v_cmp_ne_u64"); break;
		case 0xE6: KYTY_NI("v_cmp_ge_u64"); break;
		case 0xE7: KYTY_NI("v_cmp_t_u64"); break;
		case 0xE8: KYTY_NI("v_cmp_u_f16"); break;
		case 0xE9: KYTY_NI("v_cmp_nge_f16"); break;
		case 0xEA: KYTY_NI("v_cmp_nlg_f16"); break;
		case 0xEB: KYTY_NI("v_cmp_ngt_f16"); break;
		case 0xEC: KYTY_NI("v_cmp_nle_f16"); break;
		case 0xED: KYTY_NI("v_cmp_neq_f16"); break;
		case 0xEE: KYTY_NI("v_cmp_nlt_f16"); break;
		case 0xEF: KYTY_NI("v_cmp_tru_f16"); break;
		case 0xF0: KYTY_NI("v_cmpx_f_u64"); break;
		case 0xF1: KYTY_NI("v_cmpx_lt_u64"); break;
		case 0xF2: KYTY_NI("v_cmpx_eq_u64"); break;
		case 0xF3: KYTY_NI("v_cmpx_le_u64"); break;
		case 0xF4: KYTY_NI("v_cmpx_gt_u64"); break;
		case 0xF5: KYTY_NI("v_cmpx_ne_u64"); break;
		case 0xF6: KYTY_NI("v_cmpx_ge_u64"); break;
		case 0xF7: KYTY_NI("v_cmpx_t_u64"); break;
		case 0xF8: KYTY_NI("v_cmpx_u_f16"); break;
		case 0xF9: KYTY_NI("v_cmpx_nge_f16"); break;
		case 0xFA: KYTY_NI("v_cmpx_nlg_f16"); break;
		case 0xFB: KYTY_NI("v_cmpx_ngt_f16"); break;
		case 0xFC: KYTY_NI("v_cmpx_nle_f16"); break;
		case 0xFD: KYTY_NI("v_cmpx_neq_f16"); break;
		case 0xFE: KYTY_NI("v_cmpx_nlt_f16"); break;
		case 0xFF: KYTY_NI("v_cmpx_tru_f16"); break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_vop1)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("vop1");

	uint32_t vdst   = (buffer[0] >> 17u) & 0xffu;
	uint32_t src0   = (buffer[0] >> 0u) & 0x1ffu;
	uint32_t opcode = (buffer[0] >> 9u) & 0xffu;

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.src[0]  = operand_parse(src0);
	inst.dst     = operand_parse(vdst + 256);
	inst.src_num = 1;

	uint32_t size = 1;

	if (inst.src[0].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[0].constant.u = buffer[size];
		size++;
	}

	inst.format = ShaderInstructionFormat::SVdstSVsrc0;

	switch (opcode)
	{
		case 0x00: KYTY_NI("v_nop"); break;
		case 0x01: inst.type = ShaderInstructionType::VMovB32; break;
		case 0x02: KYTY_NI("v_readfirstlane_b32"); break;
		case 0x03: KYTY_NI("v_cvt_i32_f64"); break;
		case 0x04: KYTY_NI("v_cvt_f64_i32"); break;
		case 0x05: inst.type = ShaderInstructionType::VCvtF32I32; break;
		case 0x06: inst.type = ShaderInstructionType::VCvtF32U32; break;
		case 0x07: inst.type = ShaderInstructionType::VCvtU32F32; break;
		case 0x08: KYTY_NI("v_cvt_i32_f32"); break;
		case 0x09: KYTY_NI("v_mov_fed_b32"); break;
		case 0x0A: KYTY_NI("v_cvt_f16_f32"); break;
		case 0x0b: inst.type = ShaderInstructionType::VCvtF32F16; break;
		case 0x0C: KYTY_NI("v_cvt_rpi_i32_f32"); break;
		case 0x0D: KYTY_NI("v_cvt_flr_i32_f32"); break;
		case 0x0E: KYTY_NI("v_cvt_off_f32_i4"); break;
		case 0x0F: KYTY_NI("v_cvt_f32_f64"); break;
		case 0x10: KYTY_NI("v_cvt_f64_f32"); break;
		case 0x11: inst.type = ShaderInstructionType::VCvtF32Ubyte0; break;
		case 0x12: inst.type = ShaderInstructionType::VCvtF32Ubyte1; break;
		case 0x13: inst.type = ShaderInstructionType::VCvtF32Ubyte2; break;
		case 0x14: inst.type = ShaderInstructionType::VCvtF32Ubyte3; break;
		case 0x15: KYTY_NI("v_cvt_u32_f64"); break;
		case 0x16: KYTY_NI("v_cvt_f64_u32"); break;
		case 0x17: KYTY_NI("v_trunc_f64"); break;
		case 0x18: KYTY_NI("v_ceil_f64"); break;
		case 0x19: KYTY_NI("v_rndne_f64"); break;
		case 0x1A: KYTY_NI("v_floor_f64"); break;
		case 0x20: inst.type = ShaderInstructionType::VFractF32; break;
		case 0x21: inst.type = ShaderInstructionType::VTruncF32; break;
		case 0x22: inst.type = ShaderInstructionType::VCeilF32; break;
		case 0x23: inst.type = ShaderInstructionType::VRndneF32; break;
		case 0x24: inst.type = ShaderInstructionType::VFloorF32; break;
		case 0x25: inst.type = ShaderInstructionType::VExpF32; break;
		case 0x26: KYTY_NI("v_log_clamp_f32"); break;
		case 0x27: inst.type = ShaderInstructionType::VLogF32; break;
		case 0x28: KYTY_NI("v_rcp_clamp_f32"); break;
		case 0x29: KYTY_NI("v_rcp_legacy_f32"); break;
		case 0x2a: inst.type = ShaderInstructionType::VRcpF32; break;
		case 0x2B: KYTY_NI("v_rcp_iflag_f32"); break;
		case 0x2C: KYTY_NI("v_rsq_clamp_f32"); break;
		case 0x2D: KYTY_NI("v_rsq_legacy_f32"); break;
		case 0x2e: inst.type = ShaderInstructionType::VRsqF32; break;
		case 0x2F: KYTY_NI("v_rcp_f64"); break;
		case 0x30: KYTY_NI("v_rcp_clamp_f64"); break;
		case 0x31: KYTY_NI("v_rsq_f64"); break;
		case 0x32: KYTY_NI("v_rsq_clamp_f64"); break;
		case 0x33: inst.type = ShaderInstructionType::VSqrtF32; break;
		case 0x34: KYTY_NI("v_sqrt_f64"); break;
		case 0x35: inst.type = ShaderInstructionType::VSinF32; break;
		case 0x36: inst.type = ShaderInstructionType::VCosF32; break;
		case 0x37: inst.type = ShaderInstructionType::VNotB32; break;
		case 0x38: inst.type = ShaderInstructionType::VBfrevB32; break;
		case 0x39: KYTY_NI("v_ffbh_u32"); break;
		case 0x3A: KYTY_NI("v_ffbl_b32"); break;
		case 0x3B: KYTY_NI("v_ffbh_i32"); break;
		case 0x3C: KYTY_NI("v_frexp_exp_i32_f64"); break;
		case 0x3D: KYTY_NI("v_frexp_mant_f64"); break;
		case 0x3E: KYTY_NI("v_fract_f64"); break;
		case 0x3F: KYTY_NI("v_frexp_exp_i32_f32"); break;
		case 0x40: KYTY_NI("v_frexp_mant_f32"); break;
		case 0x41: KYTY_NI("v_clrexcp"); break;
		case 0x42: KYTY_NI("v_movreld_b32"); break;
		case 0x43: KYTY_NI("v_movrels_b32"); break;
		case 0x44: KYTY_NI("v_movrelsd_b32"); break;
		case 0x45: KYTY_NI("v_log_legacy_f32"); break;
		case 0x46: KYTY_NI("v_exp_legacy_f32"); break;
		case 0x50: KYTY_NI("v_cvt_f16_u16"); break;
		case 0x51: KYTY_NI("v_cvt_f16_i16"); break;
		case 0x52: KYTY_NI("v_cvt_u16_f16"); break;
		case 0x53: KYTY_NI("v_cvt_i16_f16"); break;
		case 0x54: KYTY_NI("v_rcp_f16"); break;
		case 0x55: KYTY_NI("v_sqrt_f16"); break;
		case 0x56: KYTY_NI("v_rsq_f16"); break;
		case 0x57: KYTY_NI("v_log_f16"); break;
		case 0x58: KYTY_NI("v_exp_f16"); break;
		case 0x59: KYTY_NI("v_frexp_mant_f16"); break;
		case 0x5A: KYTY_NI("v_frexp_exp_i16_f16"); break;
		case 0x5B: KYTY_NI("v_floor_f16"); break;
		case 0x5C: KYTY_NI("v_ceil_f16"); break;
		case 0x5D: KYTY_NI("v_trunc_f16"); break;
		case 0x5E: KYTY_NI("v_rndne_f16"); break;
		case 0x5F: KYTY_NI("v_fract_f16"); break;
		case 0x60: KYTY_NI("v_sin_f16"); break;
		case 0x61: KYTY_NI("v_cos_f16"); break;
		case 0x62: KYTY_NI("v_sat_pk_u8_i16"); break;
		case 0x63: KYTY_NI("v_cvt_norm_i16_f16"); break;
		case 0x64: KYTY_NI("v_cvt_norm_u16_f16"); break;
		case 0x65: KYTY_NI("v_swap_b32"); break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_vop2)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("vop2");

	uint32_t opcode = (buffer[0] >> 25u) & 0x3fu;

	switch (opcode)
	{
		case 0x3e: return shader_parse_vopc(pc, src, buffer, dst, next_gen); break;
		case 0x3f: return shader_parse_vop1(pc, src, buffer, dst, next_gen); break;
		default: break;
	}

	uint32_t vdst  = (buffer[0] >> 17u) & 0xffu;
	uint32_t src0  = (buffer[0] >> 0u) & 0x1ffu;
	uint32_t vsrc1 = (buffer[0] >> 9u) & 0xffu;

	bool sdwa = (src0 == 249);

	uint32_t size = (sdwa ? 2 : 1);

	src0               = (sdwa ? (buffer[1] >> 0u) & 0xffu : src0);
	uint32_t dst_sel   = (sdwa ? (buffer[1] >> 8u) & 0x7u : 6);
	uint32_t dst_u     = (sdwa ? (buffer[1] >> 11u) & 0x3u : 2);
	uint32_t clmp      = (sdwa ? (buffer[1] >> 13u) & 0x1u : 0);
	uint32_t omod      = (sdwa ? (buffer[1] >> 14u) & 0x3u : 0);
	uint32_t src0_sel  = (sdwa ? (buffer[1] >> 16u) & 0x7u : 6);
	uint32_t src0_sext = (sdwa ? (buffer[1] >> 19u) & 0x1u : 0);
	uint32_t src0_neg  = (sdwa ? (buffer[1] >> 20u) & 0x1u : 0);
	uint32_t src0_abs  = (sdwa ? (buffer[1] >> 21u) & 0x1u : 0);
	uint32_t s0        = (sdwa ? (buffer[1] >> 23u) & 0x1u : 1);
	uint32_t src1_sel  = (sdwa ? (buffer[1] >> 24u) & 0x7u : 6);
	uint32_t src1_sext = (sdwa ? (buffer[1] >> 27u) & 0x1u : 0);
	uint32_t src1_neg  = (sdwa ? (buffer[1] >> 28u) & 0x1u : 0);
	uint32_t src1_abs  = (sdwa ? (buffer[1] >> 29u) & 0x1u : 0);
	uint32_t s1        = (sdwa ? (buffer[1] >> 31u) & 0x1u : 0);

	EXIT_NOT_IMPLEMENTED(dst_sel != 6);
	EXIT_NOT_IMPLEMENTED(sdwa && dst_sel == 6 && dst_u != 0);
	EXIT_NOT_IMPLEMENTED(omod != 0);
	EXIT_NOT_IMPLEMENTED(src0_sel != 6);
	EXIT_NOT_IMPLEMENTED(src0_sext != 0);
	EXIT_NOT_IMPLEMENTED(src0_neg != 0);
	EXIT_NOT_IMPLEMENTED(src1_sel != 6);
	EXIT_NOT_IMPLEMENTED(src1_sext != 0);
	EXIT_NOT_IMPLEMENTED(src1_neg != 0);

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.src[0]  = operand_parse(src0 + (s0 == 0 ? 256 : 0));
	inst.src[1]  = operand_parse(vsrc1 + (s1 == 0 ? 256 : 0));
	inst.dst     = operand_parse(vdst + 256);
	inst.src_num = 2;

	switch (omod)
	{
		case 0: inst.dst.multiplier = 1.0f; break;
		case 1: inst.dst.multiplier = 2.0f; break;
		case 2: inst.dst.multiplier = 4.0f; break;
		case 3: inst.dst.multiplier = 0.5f; break;
		default: break;
	}

	if (inst.src[0].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[0].constant.u = buffer[size];
		size++;
	}

	inst.src[0].absolute = (src0_abs != 0);
	inst.src[1].absolute = (src1_abs != 0);

	inst.dst.clamp = (clmp != 0);

	inst.format = ShaderInstructionFormat::SVdstSVsrc0SVsrc1;

	switch (opcode)
	{
		case 0x00:
			EXIT_NOT_IMPLEMENTED(next_gen);
			inst.type        = ShaderInstructionType::VCndmaskB32;
			inst.format      = ShaderInstructionFormat::VdstVsrc0Vsrc1Smask2;
			inst.src[2].type = ShaderOperandType::VccLo;
			inst.src[2].size = 2;
			inst.src_num     = 3;
			break;
		case 0x01:
			if (next_gen)
			{
				inst.type        = ShaderInstructionType::VCndmaskB32;
				inst.format      = ShaderInstructionFormat::VdstVsrc0Vsrc1Smask2;
				inst.src[2].type = ShaderOperandType::VccLo;
				inst.src[2].size = 2;
				inst.src_num     = 3;
			} else
			{
				KYTY_NI("v_readlane_b32");
			};
			break;
		case 0x02:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_writelane_b32");
			};
			break;
		case 0x03: inst.type = ShaderInstructionType::VAddF32; break;
		case 0x04: inst.type = ShaderInstructionType::VSubF32; break;
		case 0x05: inst.type = ShaderInstructionType::VSubrevF32; break;
		case 0x06:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_mac_legacy_f32")
			};
			break;
		case 0x07: KYTY_NI("v_mul_legacy_f32"); break;
		case 0x08: inst.type = ShaderInstructionType::VMulF32; break;
		case 0x09: KYTY_NI("v_mul_i32_i24"); break;
		case 0x0A: KYTY_NI("v_mul_hi_i32_i24"); break;
		case 0x0C: KYTY_NI("v_mul_hi_u32_u24"); break;
		case 0x0D:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_min_legacy_f32")
			};
			break;
		case 0x0E: KYTY_NI("v_max_legacy_f32"); break;
		case 0x0b: inst.type = ShaderInstructionType::VMulU32U24; break;
		case 0x0f: inst.type = ShaderInstructionType::VMinF32; break;
		case 0x10: inst.type = ShaderInstructionType::VMaxF32; break;
		case 0x11: KYTY_NI("v_min_i32"); break;
		case 0x12: KYTY_NI("v_max_i32"); break;
		case 0x13: KYTY_NI("v_min_u32"); break;
		case 0x14: KYTY_NI("v_max_u32"); break;
		case 0x15: inst.type = ShaderInstructionType::VLshrB32; break;
		case 0x16: inst.type = ShaderInstructionType::VLshrrevB32; break;
		case 0x17: inst.type = ShaderInstructionType::VAshrI32; break;
		case 0x18: inst.type = ShaderInstructionType::VAshrrevI32; break;
		case 0x19: inst.type = ShaderInstructionType::VLshlB32; break;
		case 0x1a: inst.type = ShaderInstructionType::VLshlrevB32; break;
		case 0x1b: inst.type = ShaderInstructionType::VAndB32; break;
		case 0x1c: inst.type = ShaderInstructionType::VOrB32; break;
		case 0x1d: inst.type = ShaderInstructionType::VXorB32; break;
		case 0x1E:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				inst.type = ShaderInstructionType::VBfmB32;
			};
			break;
		case 0x1f: inst.type = ShaderInstructionType::VMacF32; break;
		case 0x20:
			inst.type              = ShaderInstructionType::VMadmkF32;
			inst.format            = ShaderInstructionFormat::VdstVsrc0Vsrc1Vsrc2;
			inst.src_num           = 3;
			inst.src[2]            = inst.src[1];
			inst.src[1].type       = ShaderOperandType::LiteralConstant;
			inst.src[1].constant.u = buffer[size];
			inst.src[1].size       = 0;
			size++;
			break;
		case 0x21:
			inst.type              = ShaderInstructionType::VMadakF32;
			inst.format            = ShaderInstructionFormat::VdstVsrc0Vsrc1Vsrc2;
			inst.src_num           = 3;
			inst.src[2].type       = ShaderOperandType::LiteralConstant;
			inst.src[2].constant.u = buffer[size];
			inst.src[2].size       = 0;
			size++;
			break;
		case 0x22: inst.type = ShaderInstructionType::VBcntU32B32; break;
		case 0x23: inst.type = ShaderInstructionType::VMbcntLoU32B32; break;
		case 0x24: inst.type = ShaderInstructionType::VMbcntHiU32B32; break;
		case 0x25:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				inst.type      = ShaderInstructionType::VAddI32;
				inst.format    = ShaderInstructionFormat::VdstSdst2Vsrc0Vsrc1;
				inst.dst2.type = ShaderOperandType::VccLo;
				inst.dst2.size = 2;
			};
			break;
		case 0x26:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				inst.type      = ShaderInstructionType::VSubI32;
				inst.format    = ShaderInstructionFormat::VdstSdst2Vsrc0Vsrc1;
				inst.dst2.type = ShaderOperandType::VccLo;
				inst.dst2.size = 2;
			};
			break;
		case 0x27:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				inst.type      = ShaderInstructionType::VSubrevI32;
				inst.format    = ShaderInstructionFormat::VdstSdst2Vsrc0Vsrc1;
				inst.dst2.type = ShaderOperandType::VccLo;
				inst.dst2.size = 2;
			};
			break;
		case 0x28:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_addc_u32")
			};
			break;
		case 0x29:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_subb_u32")
			};
			break;
		case 0x2A:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_subbrev_u32")
			};
			break;
		case 0x2B:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_ldexp_f32")
			};
			break;
		case 0x2C:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_cvt_pkaccum_u8_f32")
			};
			break;
		case 0x2D:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_cvt_pknorm_i16_f32")
			};
			break;
		case 0x2E: KYTY_NI("v_cvt_pknorm_u16_f32"); break;
		case 0x2f: inst.type = ShaderInstructionType::VCvtPkrtzF16F32; break;
		case 0x30: KYTY_NI("v_cvt_pk_u16_u32"); break;
		case 0x31: KYTY_NI("v_cvt_pk_i16_i32"); break;
		case 0x32: KYTY_NI("v_add_f16"); break;
		case 0x33: KYTY_NI("v_sub_f16"); break;
		case 0x34: KYTY_NI("v_subrev_f16"); break;
		case 0x35: KYTY_NI("v_mul_f16"); break;
		case 0x36:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_mac_f16")
			};
			break;
		case 0x37:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_madmk_f16")
			};
			break;
		case 0x38:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_madak_f16")
			};
			break;
		case 0x39: KYTY_NI("v_max_f16"); break;
		case 0x3A: KYTY_NI("v_min_f16"); break;
		case 0x3B: KYTY_NI("v_ldexp_f16"); break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity,readability-function-size,google-readability-function-size,hicpp-function-size)
KYTY_SHADER_PARSER(shader_parse_vop3)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("vop3");

	uint32_t opcode = (next_gen ? (buffer[0] >> 16u) & 0x3ffu : (buffer[0] >> 17u) & 0x1ffu);
	uint32_t clamp  = (next_gen ? (buffer[0] >> 15u) & 0x1u : (buffer[0] >> 11u) & 0x1u);
	uint32_t op_sel = (next_gen ? (buffer[0] >> 11u) & 0xfu : 0);
	uint32_t abs    = (buffer[0] >> 8u) & 0x7u;
	uint32_t vdst   = (buffer[0] >> 0u) & 0xffu;
	uint32_t sdst   = (buffer[0] >> 8u) & 0x7fu;
	uint32_t neg    = (buffer[1] >> 29u) & 0x7u;
	uint32_t omod   = (buffer[1] >> 27u) & 0x3u;
	uint32_t src0   = (buffer[1] >> 0u) & 0x1ffu;
	uint32_t src1   = (buffer[1] >> 9u) & 0x1ffu;
	uint32_t src2   = (buffer[1] >> 18u) & 0x1ffu;

	EXIT_NOT_IMPLEMENTED(op_sel != 0);

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.src[0]  = operand_parse(src0);
	inst.src[1]  = operand_parse(src1);
	inst.src[2]  = operand_parse(src2);
	inst.src_num = 3;
	inst.dst     = operand_parse(vdst + 256);

	switch (omod)
	{
		case 0: inst.dst.multiplier = 1.0f; break;
		case 1: inst.dst.multiplier = 2.0f; break;
		case 2: inst.dst.multiplier = 4.0f; break;
		case 3: inst.dst.multiplier = 0.5f; break;
		default: break;
	}

	if ((neg & 0x1u) != 0)
	{
		inst.src[0].negate = true;
	}
	if ((neg & 0x2u) != 0)
	{
		inst.src[1].negate = true;
	}
	if ((neg & 0x4u) != 0)
	{
		inst.src[2].negate = true;
	}

	uint32_t size = 2;

	if (inst.src[0].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[0].constant.u = buffer[size];
		size++;
	}

	if (inst.src[1].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[1].constant.u = buffer[size];
		size++;
	}

	if (inst.src[2].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[2].constant.u = buffer[size];
		size++;
	}

	inst.format = ShaderInstructionFormat::VdstVsrc0Vsrc1Vsrc2;

	if (opcode >= 0 && opcode <= 0xff)
	{
		/* VOPC using VOP3 encoding */
		inst.format   = ShaderInstructionFormat::SmaskVsrc0Vsrc1;
		inst.src_num  = 2;
		inst.dst      = operand_parse(vdst);
		inst.dst.size = 2;
	}

	if (opcode >= 0x100 && opcode <= 0x13d)
	{
		/* VOP2 using VOP3 encoding */
		inst.format  = ShaderInstructionFormat::SVdstSVsrc0SVsrc1;
		inst.src_num = 2;
	}

	if (opcode >= 0x180 && opcode <= 0x1e8)
	{
		/* VOP1 using VOP3 encoding */
		inst.format  = ShaderInstructionFormat::SVdstSVsrc0;
		inst.src_num = 1;
	}

	switch (opcode)
	{
		/* VOPC using VOP3 encoding */
		case 0x00: inst.type = ShaderInstructionType::VCmpFF32; break;
		case 0x01: inst.type = ShaderInstructionType::VCmpLtF32; break;
		case 0x02: inst.type = ShaderInstructionType::VCmpEqF32; break;
		case 0x03: inst.type = ShaderInstructionType::VCmpLeF32; break;
		case 0x04: inst.type = ShaderInstructionType::VCmpGtF32; break;
		case 0x05: inst.type = ShaderInstructionType::VCmpLgF32; break;
		case 0x06: inst.type = ShaderInstructionType::VCmpGeF32; break;
		case 0x07: inst.type = ShaderInstructionType::VCmpOF32; break;
		case 0x08: inst.type = ShaderInstructionType::VCmpUF32; break;
		case 0x09: inst.type = ShaderInstructionType::VCmpNgeF32; break;
		case 0x0a: inst.type = ShaderInstructionType::VCmpNlgF32; break;
		case 0x0b: inst.type = ShaderInstructionType::VCmpNgtF32; break;
		case 0x0c: inst.type = ShaderInstructionType::VCmpNleF32; break;
		case 0x0d: inst.type = ShaderInstructionType::VCmpNeqF32; break;
		case 0x0e: inst.type = ShaderInstructionType::VCmpNltF32; break;
		case 0x0f: inst.type = ShaderInstructionType::VCmpTruF32; break;
		case 0x10: KYTY_NI("v_cmpx_f_f32"); break;
		case 0x11: KYTY_NI("v_cmpx_lt_f32"); break;
		case 0x12: KYTY_NI("v_cmpx_eq_f32"); break;
		case 0x13: KYTY_NI("v_cmpx_le_f32"); break;
		case 0x14: KYTY_NI("v_cmpx_gt_f32"); break;
		case 0x15: KYTY_NI("v_cmpx_lg_f32"); break;
		case 0x16: KYTY_NI("v_cmpx_ge_f32"); break;
		case 0x17: KYTY_NI("v_cmpx_o_f32"); break;
		case 0x18: KYTY_NI("v_cmpx_u_f32"); break;
		case 0x19: KYTY_NI("v_cmpx_nge_f32"); break;
		case 0x1A: KYTY_NI("v_cmpx_nlg_f32"); break;
		case 0x1B: KYTY_NI("v_cmpx_ngt_f32"); break;
		case 0x1C: KYTY_NI("v_cmpx_nle_f32"); break;
		case 0x1d: inst.type = ShaderInstructionType::VCmpxNeqF32; break;
		case 0x1E: KYTY_NI("v_cmpx_nlt_f32"); break;
		case 0x1F: KYTY_NI("v_cmpx_tru_f32"); break;
		case 0x20: KYTY_NI("v_cmp_f_f64"); break;
		case 0x21: KYTY_NI("v_cmp_lt_f64"); break;
		case 0x22: KYTY_NI("v_cmp_eq_f64"); break;
		case 0x23: KYTY_NI("v_cmp_le_f64"); break;
		case 0x24: KYTY_NI("v_cmp_gt_f64"); break;
		case 0x25: KYTY_NI("v_cmp_lg_f64"); break;
		case 0x26: KYTY_NI("v_cmp_ge_f64"); break;
		case 0x27: KYTY_NI("v_cmp_o_f64"); break;
		case 0x28: KYTY_NI("v_cmp_u_f64"); break;
		case 0x29: KYTY_NI("v_cmp_nge_f64"); break;
		case 0x2A: KYTY_NI("v_cmp_nlg_f64"); break;
		case 0x2B: KYTY_NI("v_cmp_ngt_f64"); break;
		case 0x2C: KYTY_NI("v_cmp_nle_f64"); break;
		case 0x2D: KYTY_NI("v_cmp_neq_f64"); break;
		case 0x2E: KYTY_NI("v_cmp_nlt_f64"); break;
		case 0x2F: KYTY_NI("v_cmp_tru_f64"); break;
		case 0x30: KYTY_NI("v_cmpx_f_f64"); break;
		case 0x31: KYTY_NI("v_cmpx_lt_f64"); break;
		case 0x32: KYTY_NI("v_cmpx_eq_f64"); break;
		case 0x33: KYTY_NI("v_cmpx_le_f64"); break;
		case 0x34: KYTY_NI("v_cmpx_gt_f64"); break;
		case 0x35: KYTY_NI("v_cmpx_lg_f64"); break;
		case 0x36: KYTY_NI("v_cmpx_ge_f64"); break;
		case 0x37: KYTY_NI("v_cmpx_o_f64"); break;
		case 0x38: KYTY_NI("v_cmpx_u_f64"); break;
		case 0x39: KYTY_NI("v_cmpx_nge_f64"); break;
		case 0x3A: KYTY_NI("v_cmpx_nlg_f64"); break;
		case 0x3B: KYTY_NI("v_cmpx_ngt_f64"); break;
		case 0x3C: KYTY_NI("v_cmpx_nle_f64"); break;
		case 0x3D: KYTY_NI("v_cmpx_neq_f64"); break;
		case 0x3E: KYTY_NI("v_cmpx_nlt_f64"); break;
		case 0x3F: KYTY_NI("v_cmpx_tru_f64"); break;
		case 0x40: KYTY_NI("v_cmps_f_f32"); break;
		case 0x41: KYTY_NI("v_cmps_lt_f32"); break;
		case 0x42: KYTY_NI("v_cmps_eq_f32"); break;
		case 0x43: KYTY_NI("v_cmps_le_f32"); break;
		case 0x44: KYTY_NI("v_cmps_gt_f32"); break;
		case 0x45: KYTY_NI("v_cmps_lg_f32"); break;
		case 0x46: KYTY_NI("v_cmps_ge_f32"); break;
		case 0x47: KYTY_NI("v_cmps_o_f32"); break;
		case 0x48: KYTY_NI("v_cmps_u_f32"); break;
		case 0x49: KYTY_NI("v_cmps_nge_f32"); break;
		case 0x4A: KYTY_NI("v_cmps_nlg_f32"); break;
		case 0x4B: KYTY_NI("v_cmps_ngt_f32"); break;
		case 0x4C: KYTY_NI("v_cmps_nle_f32"); break;
		case 0x4D: KYTY_NI("v_cmps_neq_f32"); break;
		case 0x4E: KYTY_NI("v_cmps_nlt_f32"); break;
		case 0x4F: KYTY_NI("v_cmps_tru_f32"); break;
		case 0x50: KYTY_NI("v_cmpsx_f_f32"); break;
		case 0x51: KYTY_NI("v_cmpsx_lt_f32"); break;
		case 0x52: KYTY_NI("v_cmpsx_eq_f32"); break;
		case 0x53: KYTY_NI("v_cmpsx_le_f32"); break;
		case 0x54: KYTY_NI("v_cmpsx_gt_f32"); break;
		case 0x55: KYTY_NI("v_cmpsx_lg_f32"); break;
		case 0x56: KYTY_NI("v_cmpsx_ge_f32"); break;
		case 0x57: KYTY_NI("v_cmpsx_o_f32"); break;
		case 0x58: KYTY_NI("v_cmpsx_u_f32"); break;
		case 0x59: KYTY_NI("v_cmpsx_nge_f32"); break;
		case 0x5A: KYTY_NI("v_cmpsx_nlg_f32"); break;
		case 0x5B: KYTY_NI("v_cmpsx_ngt_f32"); break;
		case 0x5C: KYTY_NI("v_cmpsx_nle_f32"); break;
		case 0x5D: KYTY_NI("v_cmpsx_neq_f32"); break;
		case 0x5E: KYTY_NI("v_cmpsx_nlt_f32"); break;
		case 0x5F: KYTY_NI("v_cmpsx_tru_f32"); break;
		case 0x60: KYTY_NI("v_cmps_f_f64"); break;
		case 0x61: KYTY_NI("v_cmps_lt_f64"); break;
		case 0x62: KYTY_NI("v_cmps_eq_f64"); break;
		case 0x63: KYTY_NI("v_cmps_le_f64"); break;
		case 0x64: KYTY_NI("v_cmps_gt_f64"); break;
		case 0x65: KYTY_NI("v_cmps_lg_f64"); break;
		case 0x66: KYTY_NI("v_cmps_ge_f64"); break;
		case 0x67: KYTY_NI("v_cmps_o_f64"); break;
		case 0x68: KYTY_NI("v_cmps_u_f64"); break;
		case 0x69: KYTY_NI("v_cmps_nge_f64"); break;
		case 0x6A: KYTY_NI("v_cmps_nlg_f64"); break;
		case 0x6B: KYTY_NI("v_cmps_ngt_f64"); break;
		case 0x6C: KYTY_NI("v_cmps_nle_f64"); break;
		case 0x6D: KYTY_NI("v_cmps_neq_f64"); break;
		case 0x6E: KYTY_NI("v_cmps_nlt_f64"); break;
		case 0x6F: KYTY_NI("v_cmps_tru_f64"); break;
		case 0x70: KYTY_NI("v_cmpsx_f_f64"); break;
		case 0x71: KYTY_NI("v_cmpsx_lt_f64"); break;
		case 0x72: KYTY_NI("v_cmpsx_eq_f64"); break;
		case 0x73: KYTY_NI("v_cmpsx_le_f64"); break;
		case 0x74: KYTY_NI("v_cmpsx_gt_f64"); break;
		case 0x75: KYTY_NI("v_cmpsx_lg_f64"); break;
		case 0x76: KYTY_NI("v_cmpsx_ge_f64"); break;
		case 0x77: KYTY_NI("v_cmpsx_o_f64"); break;
		case 0x78: KYTY_NI("v_cmpsx_u_f64"); break;
		case 0x79: KYTY_NI("v_cmpsx_nge_f64"); break;
		case 0x7A: KYTY_NI("v_cmpsx_nlg_f64"); break;
		case 0x7B: KYTY_NI("v_cmpsx_ngt_f64"); break;
		case 0x7C: KYTY_NI("v_cmpsx_nle_f64"); break;
		case 0x7D: KYTY_NI("v_cmpsx_neq_f64"); break;
		case 0x7E: KYTY_NI("v_cmpsx_nlt_f64"); break;
		case 0x7F: KYTY_NI("v_cmpsx_tru_f64"); break;
		case 0x80: inst.type = ShaderInstructionType::VCmpFI32; break;
		case 0x81: inst.type = ShaderInstructionType::VCmpLtI32; break;
		case 0x82: inst.type = ShaderInstructionType::VCmpEqI32; break;
		case 0x83: inst.type = ShaderInstructionType::VCmpLeI32; break;
		case 0x84: inst.type = ShaderInstructionType::VCmpGtI32; break;
		case 0x85: inst.type = ShaderInstructionType::VCmpNeI32; break;
		case 0x86: inst.type = ShaderInstructionType::VCmpGeI32; break;
		case 0x87: inst.type = ShaderInstructionType::VCmpTI32; break;
		case 0x88: KYTY_NI("v_cmp_class_f32"); break;
		case 0x89: KYTY_NI("v_cmp_lt_i16"); break;
		case 0x8A: KYTY_NI("v_cmp_eq_i16"); break;
		case 0x8B: KYTY_NI("v_cmp_le_i16"); break;
		case 0x8C: KYTY_NI("v_cmp_gt_i16"); break;
		case 0x8D: KYTY_NI("v_cmp_ne_i16"); break;
		case 0x8E: KYTY_NI("v_cmp_ge_i16"); break;
		case 0x8F: KYTY_NI("v_cmp_class_f16"); break;
		case 0x90: KYTY_NI("v_cmpx_f_i32"); break;
		case 0x91: KYTY_NI("v_cmpx_lt_i32"); break;
		case 0x92: KYTY_NI("v_cmpx_eq_i32"); break;
		case 0x93: KYTY_NI("v_cmpx_le_i32"); break;
		case 0x94: KYTY_NI("v_cmpx_gt_i32"); break;
		case 0x95: KYTY_NI("v_cmpx_ne_i32"); break;
		case 0x96: KYTY_NI("v_cmpx_ge_i32"); break;
		case 0x97: KYTY_NI("v_cmpx_t_i32"); break;
		case 0x98: KYTY_NI("v_cmpx_class_f32"); break;
		case 0x99: KYTY_NI("v_cmpx_lt_i16"); break;
		case 0x9A: KYTY_NI("v_cmpx_eq_i16"); break;
		case 0x9B: KYTY_NI("v_cmpx_le_i16"); break;
		case 0x9C: KYTY_NI("v_cmpx_gt_i16"); break;
		case 0x9D: KYTY_NI("v_cmpx_ne_i16"); break;
		case 0x9E: KYTY_NI("v_cmpx_ge_i16"); break;
		case 0x9F: KYTY_NI("v_cmpx_class_f16"); break;
		case 0xA0: KYTY_NI("v_cmp_f_i64"); break;
		case 0xA1: KYTY_NI("v_cmp_lt_i64"); break;
		case 0xA2: KYTY_NI("v_cmp_eq_i64"); break;
		case 0xA3: KYTY_NI("v_cmp_le_i64"); break;
		case 0xA4: KYTY_NI("v_cmp_gt_i64"); break;
		case 0xA5: KYTY_NI("v_cmp_ne_i64"); break;
		case 0xA6: KYTY_NI("v_cmp_ge_i64"); break;
		case 0xA7: KYTY_NI("v_cmp_t_i64"); break;
		case 0xA8: KYTY_NI("v_cmp_class_f64"); break;
		case 0xA9: KYTY_NI("v_cmp_lt_u16"); break;
		case 0xAA: KYTY_NI("v_cmp_eq_u16"); break;
		case 0xAB: KYTY_NI("v_cmp_le_u16"); break;
		case 0xAC: KYTY_NI("v_cmp_gt_u16"); break;
		case 0xAD: KYTY_NI("v_cmp_ne_u16"); break;
		case 0xAE: KYTY_NI("v_cmp_ge_u16"); break;
		case 0xB0: KYTY_NI("v_cmpx_f_i64"); break;
		case 0xB1: KYTY_NI("v_cmpx_lt_i64"); break;
		case 0xB2: KYTY_NI("v_cmpx_eq_i64"); break;
		case 0xB3: KYTY_NI("v_cmpx_le_i64"); break;
		case 0xB4: KYTY_NI("v_cmpx_gt_i64"); break;
		case 0xB5: KYTY_NI("v_cmpx_ne_i64"); break;
		case 0xB6: KYTY_NI("v_cmpx_ge_i64"); break;
		case 0xB7: KYTY_NI("v_cmpx_t_i64"); break;
		case 0xB8: KYTY_NI("v_cmpx_class_f64"); break;
		case 0xB9: KYTY_NI("v_cmpx_lt_u16"); break;
		case 0xBA: KYTY_NI("v_cmpx_eq_u16"); break;
		case 0xBB: KYTY_NI("v_cmpx_le_u16"); break;
		case 0xBC: KYTY_NI("v_cmpx_gt_u16"); break;
		case 0xBD: KYTY_NI("v_cmpx_ne_u16"); break;
		case 0xBE: KYTY_NI("v_cmpx_ge_u16"); break;
		case 0xc0: inst.type = ShaderInstructionType::VCmpFU32; break;
		case 0xc1: inst.type = ShaderInstructionType::VCmpLtU32; break;
		case 0xc2: inst.type = ShaderInstructionType::VCmpEqU32; break;
		case 0xc3: inst.type = ShaderInstructionType::VCmpLeU32; break;
		case 0xc4: inst.type = ShaderInstructionType::VCmpGtU32; break;
		case 0xc5: inst.type = ShaderInstructionType::VCmpNeU32; break;
		case 0xc6: inst.type = ShaderInstructionType::VCmpGeU32; break;
		case 0xc7: inst.type = ShaderInstructionType::VCmpTU32; break;
		case 0xC8: KYTY_NI("v_cmp_f_f16"); break;
		case 0xC9: KYTY_NI("v_cmp_lt_f16"); break;
		case 0xCA: KYTY_NI("v_cmp_eq_f16"); break;
		case 0xCB: KYTY_NI("v_cmp_le_f16"); break;
		case 0xCC: KYTY_NI("v_cmp_gt_f16"); break;
		case 0xCD: KYTY_NI("v_cmp_lg_f16"); break;
		case 0xCE: KYTY_NI("v_cmp_ge_f16"); break;
		case 0xCF: KYTY_NI("v_cmp_o_f16"); break;
		case 0xD0: KYTY_NI("v_cmpx_f_u32"); break;
		case 0xD1: KYTY_NI("v_cmpx_lt_u32"); break;
		case 0xd2: inst.type = ShaderInstructionType::VCmpxEqU32; break;
		case 0xD3: KYTY_NI("v_cmpx_le_u32"); break;
		case 0xd4: inst.type = ShaderInstructionType::VCmpxGtU32; break;
		case 0xd5: inst.type = ShaderInstructionType::VCmpxNeU32; break;
		case 0xd6: inst.type = ShaderInstructionType::VCmpxGeU32; break;
		case 0xD7: KYTY_NI("v_cmpx_t_u32"); break;
		case 0xD8: KYTY_NI("v_cmpx_f_f16"); break;
		case 0xD9: KYTY_NI("v_cmpx_lt_f16"); break;
		case 0xDA: KYTY_NI("v_cmpx_eq_f16"); break;
		case 0xDB: KYTY_NI("v_cmpx_le_f16"); break;
		case 0xDC: KYTY_NI("v_cmpx_gt_f16"); break;
		case 0xDD: KYTY_NI("v_cmpx_lg_f16"); break;
		case 0xDE: KYTY_NI("v_cmpx_ge_f16"); break;
		case 0xDF: KYTY_NI("v_cmpx_o_f16"); break;
		case 0xE0: KYTY_NI("v_cmp_f_u64"); break;
		case 0xE1: KYTY_NI("v_cmp_lt_u64"); break;
		case 0xE2: KYTY_NI("v_cmp_eq_u64"); break;
		case 0xE3: KYTY_NI("v_cmp_le_u64"); break;
		case 0xE4: KYTY_NI("v_cmp_gt_u64"); break;
		case 0xE5: KYTY_NI("v_cmp_ne_u64"); break;
		case 0xE6: KYTY_NI("v_cmp_ge_u64"); break;
		case 0xE7: KYTY_NI("v_cmp_t_u64"); break;
		case 0xE8: KYTY_NI("v_cmp_u_f16"); break;
		case 0xE9: KYTY_NI("v_cmp_nge_f16"); break;
		case 0xEA: KYTY_NI("v_cmp_nlg_f16"); break;
		case 0xEB: KYTY_NI("v_cmp_ngt_f16"); break;
		case 0xEC: KYTY_NI("v_cmp_nle_f16"); break;
		case 0xED: KYTY_NI("v_cmp_neq_f16"); break;
		case 0xEE: KYTY_NI("v_cmp_nlt_f16"); break;
		case 0xEF: KYTY_NI("v_cmp_tru_f16"); break;
		case 0xF0: KYTY_NI("v_cmpx_f_u64"); break;
		case 0xF1: KYTY_NI("v_cmpx_lt_u64"); break;
		case 0xF2: KYTY_NI("v_cmpx_eq_u64"); break;
		case 0xF3: KYTY_NI("v_cmpx_le_u64"); break;
		case 0xF4: KYTY_NI("v_cmpx_gt_u64"); break;
		case 0xF5: KYTY_NI("v_cmpx_ne_u64"); break;
		case 0xF6: KYTY_NI("v_cmpx_ge_u64"); break;
		case 0xF7: KYTY_NI("v_cmpx_t_u64"); break;
		case 0xF8: KYTY_NI("v_cmpx_u_f16"); break;
		case 0xF9: KYTY_NI("v_cmpx_nge_f16"); break;
		case 0xFA: KYTY_NI("v_cmpx_nlg_f16"); break;
		case 0xFB: KYTY_NI("v_cmpx_ngt_f16"); break;
		case 0xFC: KYTY_NI("v_cmpx_nle_f16"); break;
		case 0xFD: KYTY_NI("v_cmpx_neq_f16"); break;
		case 0xFE: KYTY_NI("v_cmpx_nlt_f16"); break;
		case 0xFF: KYTY_NI("v_cmpx_tru_f16"); break;

		/* VOP2 using VOP3 encoding */
		case 0x100:
			EXIT_NOT_IMPLEMENTED(next_gen);
			inst.type        = ShaderInstructionType::VCndmaskB32;
			inst.format      = ShaderInstructionFormat::VdstVsrc0Vsrc1Smask2;
			inst.src_num     = 3;
			inst.src[2].size = 2;
			break;
		case 0x101:
			if (next_gen)
			{
				inst.type        = ShaderInstructionType::VCndmaskB32;
				inst.format      = ShaderInstructionFormat::VdstVsrc0Vsrc1Smask2;
				inst.src_num     = 3;
				inst.src[2].size = 2;
			} else
			{
				KYTY_NI("v_readlane_b32");
			};
			break;
		case 0x102:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_writelane_b32");
			};
			break;
		case 0x103: inst.type = ShaderInstructionType::VAddF32; break;
		case 0x104: inst.type = ShaderInstructionType::VSubF32; break;
		case 0x105: inst.type = ShaderInstructionType::VSubrevF32; break;
		case 0x106:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_mac_legacy_f32")
			};
			break;
		case 0x107: KYTY_NI("v_mul_legacy_f32"); break;
		case 0x108: inst.type = ShaderInstructionType::VMulF32; break;
		case 0x109: KYTY_NI("v_mul_i32_i24"); break;
		case 0x10A: KYTY_NI("v_mul_hi_i32_i24"); break;
		case 0x10b: inst.type = ShaderInstructionType::VMulU32U24; break;
		case 0x10C: KYTY_NI("v_mul_hi_u32_u24"); break;
		case 0x10D:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_min_legacy_f32")
			};
			break;
		case 0x10E: KYTY_NI("v_max_legacy_f32"); break;
		case 0x10f: inst.type = ShaderInstructionType::VMinF32; break;
		case 0x110: inst.type = ShaderInstructionType::VMaxF32; break;
		case 0x111: KYTY_NI("v_min_i32"); break;
		case 0x112: KYTY_NI("v_max_i32"); break;
		case 0x113: KYTY_NI("v_min_u32"); break;
		case 0x114: KYTY_NI("v_max_u32"); break;
		case 0x115: inst.type = ShaderInstructionType::VLshrB32; break;
		case 0x116: inst.type = ShaderInstructionType::VLshrrevB32; break;
		case 0x117: inst.type = ShaderInstructionType::VAshrI32; break;
		case 0x118: inst.type = ShaderInstructionType::VAshrrevI32; break;
		case 0x119: inst.type = ShaderInstructionType::VLshlB32; break;
		case 0x11a: inst.type = ShaderInstructionType::VLshlrevB32; break;
		case 0x11b: inst.type = ShaderInstructionType::VAndB32; break;
		case 0x11c: inst.type = ShaderInstructionType::VOrB32; break;
		case 0x11d: inst.type = ShaderInstructionType::VXorB32; break;
		case 0x11E:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				inst.type = ShaderInstructionType::VBfmB32;
			};
			break;
		case 0x11f: inst.type = ShaderInstructionType::VMacF32; break;
		case 0x120: KYTY_NI("v_madmk_f32"); break;
		case 0x121: KYTY_NI("v_madak_f32"); break;
		case 0x122: inst.type = ShaderInstructionType::VBcntU32B32; break;
		case 0x123: inst.type = ShaderInstructionType::VMbcntLoU32B32; break;
		case 0x124: inst.type = ShaderInstructionType::VMbcntHiU32B32; break;
		case 0x125:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				inst.type      = ShaderInstructionType::VAddI32;
				inst.format    = ShaderInstructionFormat::VdstSdst2Vsrc0Vsrc1;
				inst.dst2      = operand_parse(sdst);
				inst.dst2.size = 2;
			};
			break;
		case 0x126:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				inst.type      = ShaderInstructionType::VSubI32;
				inst.format    = ShaderInstructionFormat::VdstSdst2Vsrc0Vsrc1;
				inst.dst2      = operand_parse(sdst);
				inst.dst2.size = 2;
			};
			break;
		case 0x127:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				inst.type      = ShaderInstructionType::VSubrevI32;
				inst.format    = ShaderInstructionFormat::VdstSdst2Vsrc0Vsrc1;
				inst.dst2      = operand_parse(sdst);
				inst.dst2.size = 2;
			};
			break;
		case 0x128:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_addc_u32")
			};
			break;
		case 0x129:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_subb_u32")
			};
			break;
		case 0x12A:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_subbrev_u32")
			};
			break;
		case 0x12B:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_ldexp_f32")
			};
			break;
		case 0x12C:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_cvt_pkaccum_u8_f32")
			};
			break;
		case 0x12D:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_cvt_pknorm_i16_f32")
			};
			break;
		case 0x12E: KYTY_NI("v_cvt_pknorm_u16_f32"); break;
		case 0x12f: inst.type = ShaderInstructionType::VCvtPkrtzF16F32; break;
		case 0x130: KYTY_NI("v_cvt_pk_u16_u32"); break;
		case 0x131: KYTY_NI("v_cvt_pk_i16_i32"); break;
		case 0x132: KYTY_NI("v_add_f16"); break;
		case 0x133: KYTY_NI("v_sub_f16"); break;
		case 0x134: KYTY_NI("v_subrev_f16"); break;
		case 0x135: KYTY_NI("v_mul_f16"); break;
		case 0x136:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_mac_f16")
			};
			break;
		case 0x137:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_madmk_f16")
			};
			break;
		case 0x138:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_madak_f16")
			};
			break;
		case 0x139: KYTY_NI("v_max_f16"); break;
		case 0x13A: KYTY_NI("v_min_f16"); break;
		case 0x13B: KYTY_NI("v_ldexp_f16"); break;

		/* VOP3 instructions */
		case 0x140:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_mad_legacy_f32")
			};
			break;
		case 0x141: inst.type = ShaderInstructionType::VMadF32; break;
		case 0x142: KYTY_NI("v_mad_i32_i24"); break;
		case 0x143: inst.type = ShaderInstructionType::VMadU32U24; break;
		case 0x144: KYTY_NI("v_cubeid_f32"); break;
		case 0x145: KYTY_NI("v_cubesc_f32"); break;
		case 0x146: KYTY_NI("v_cubetc_f32"); break;
		case 0x147: KYTY_NI("v_cubema_f32"); break;
		case 0x148: inst.type = ShaderInstructionType::VBfeU32; break;
		case 0x149: KYTY_NI("v_bfe_i32"); break;
		case 0x14A: KYTY_NI("v_bfi_b32"); break;
		case 0x14b: inst.type = ShaderInstructionType::VFmaF32; break;
		case 0x14C: KYTY_NI("v_fma_f64"); break;
		case 0x14D: KYTY_NI("v_lerp_u8"); break;
		case 0x14E: KYTY_NI("v_alignbit_b32"); break;
		case 0x14F: KYTY_NI("v_alignbyte_b32"); break;
		case 0x150: KYTY_NI("v_mullit_f32"); break;
		case 0x151: inst.type = ShaderInstructionType::VMin3F32; break;
		case 0x152: KYTY_NI("v_min3_i32"); break;
		case 0x153: KYTY_NI("v_min3_u32"); break;
		case 0x154: inst.type = ShaderInstructionType::VMax3F32; break;
		case 0x155: KYTY_NI("v_max3_i32"); break;
		case 0x156: KYTY_NI("v_max3_u32"); break;
		case 0x157: inst.type = ShaderInstructionType::VMed3F32; break;
		case 0x158: KYTY_NI("v_med3_i32"); break;
		case 0x159: KYTY_NI("v_med3_u32"); break;
		case 0x15A: KYTY_NI("v_sad_u8"); break;
		case 0x15B: KYTY_NI("v_sad_hi_u8"); break;
		case 0x15C: KYTY_NI("v_sad_u16"); break;
		case 0x15d: inst.type = ShaderInstructionType::VSadU32; break;
		case 0x15E: KYTY_NI("v_cvt_pk_u8_f32"); break;
		case 0x15F: KYTY_NI("v_div_fixup_f32"); break;
		case 0x160: KYTY_NI("v_div_fixup_f64"); break;
		case 0x161: KYTY_NI("v_lshl_b64"); break;
		case 0x162: KYTY_NI("v_lshr_b64"); break;
		case 0x163: KYTY_NI("v_ashr_i64"); break;
		case 0x164: KYTY_NI("v_add_f64"); break;
		case 0x165: KYTY_NI("v_mul_f64"); break;
		case 0x166: KYTY_NI("v_min_f64"); break;
		case 0x167: KYTY_NI("v_max_f64"); break;
		case 0x168: KYTY_NI("v_ldexp_f64"); break;
		case 0x169:
			inst.type    = ShaderInstructionType::VMulLoU32;
			inst.format  = ShaderInstructionFormat::SVdstSVsrc0SVsrc1;
			inst.src_num = 2;
			break;
		case 0x16a:
			inst.type    = ShaderInstructionType::VMulHiU32;
			inst.format  = ShaderInstructionFormat::SVdstSVsrc0SVsrc1;
			inst.src_num = 2;
			break;
		case 0x16b:
			inst.type    = ShaderInstructionType::VMulLoI32;
			inst.format  = ShaderInstructionFormat::SVdstSVsrc0SVsrc1;
			inst.src_num = 2;
			break;
		case 0x16C: KYTY_NI("v_mul_hi_i32"); break;
		case 0x16D: KYTY_NI("v_div_scale_f32"); break;
		case 0x16E: KYTY_NI("v_div_scale_f64"); break;
		case 0x16F: KYTY_NI("v_div_fmas_f32"); break;
		case 0x170: KYTY_NI("v_div_fmas_f64"); break;
		case 0x171: KYTY_NI("v_msad_u8"); break;
		case 0x174: KYTY_NI("v_trig_preop_f64"); break;
		case 0x175: KYTY_NI("v_mqsad_u32_u8"); break;
		case 0x176: KYTY_NI("v_mad_u64_u32"); break;
		case 0x177: KYTY_NI("v_mad_i64_i32"); break;
		case 0x303:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_add_u16")
			};
			break;
		case 0x304:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_sub_u16")
			};
			break;
		case 0x305: KYTY_NI("v_mul_lo_u16"); break;
		case 0x307: KYTY_NI("v_lshrrev_b16"); break;
		case 0x308: KYTY_NI("v_ashrrev_i16"); break;
		case 0x309: KYTY_NI("v_max_u16"); break;
		case 0x30A: KYTY_NI("v_max_i16"); break;
		case 0x30B: KYTY_NI("v_min_u16"); break;
		case 0x30C: KYTY_NI("v_min_i16"); break;
		case 0x30D:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_add_i16")
			};
			break;
		case 0x30E:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_sub_i16")
			};
			break;
		case 0x30F:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_add_u32")
			};
			break;
		case 0x310:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_sub_u32")
			};
			break;
		case 0x311: KYTY_NI("v_pack_b32_f16"); break;
		case 0x312: KYTY_NI("v_cvt_pknorm_i16_f16"); break;
		case 0x313: KYTY_NI("v_cvt_pknorm_u16_f16"); break;
		case 0x314: KYTY_NI("v_lshlrev_b16"); break;
		case 0x340: KYTY_NI("v_mad_u16"); break;
		case 0x341: KYTY_NI("v_mad_f16"); break;
		case 0x342: KYTY_NI("v_interp_p1ll_f16"); break;
		case 0x343: KYTY_NI("v_interp_p1lv_f16"); break;
		case 0x344: KYTY_NI("v_perm_b32"); break;
		case 0x345:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_xad_b32")
			};
			break;
		case 0x346: KYTY_NI("v_lshl_add_u32"); break;
		case 0x347: KYTY_NI("v_add_lshl_u32"); break;
		case 0x34B: KYTY_NI("v_fma_f16"); break;
		case 0x351: KYTY_NI("v_min3_f16"); break;
		case 0x352: KYTY_NI("v_min3_i16"); break;
		case 0x353: KYTY_NI("v_min3_u16"); break;
		case 0x354: KYTY_NI("v_max3_f16"); break;
		case 0x355: KYTY_NI("v_max3_i16"); break;
		case 0x356: KYTY_NI("v_max3_u16"); break;
		case 0x357: KYTY_NI("v_med3_f16"); break;
		case 0x358: KYTY_NI("v_med3_i16"); break;
		case 0x359: KYTY_NI("v_med3_u16"); break;
		case 0x35A: KYTY_NI("v_interp_p2_f16"); break;
		case 0x35E: KYTY_NI("v_mad_i16"); break;
		case 0x35F: KYTY_NI("v_div_fixup_f16"); break;
		case 0x36D: KYTY_NI("v_add3_u32"); break;
		case 0x36F:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_lshl_or_u32")
			};
			break;
		case 0x371:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_and_or_u32")
			};
			break;
		case 0x372:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("v_or3_u32")
			};
			break;
		case 0x373: KYTY_NI("v_mad_u32_u16"); break;
		case 0x375: KYTY_NI("v_mad_i32_i16"); break;

		/* VOP1 using VOP3 encoding */
		case 0x180: KYTY_NI("v_nop"); break;
		case 0x181: KYTY_NI("v_mov_b32"); break;
		case 0x182: KYTY_NI("v_readfirstlane_b32"); break;
		case 0x183: KYTY_NI("v_cvt_i32_f64"); break;
		case 0x184: KYTY_NI("v_cvt_f64_i32"); break;
		case 0x185: KYTY_NI("v_cvt_f32_i32"); break;
		case 0x186: KYTY_NI("v_cvt_f32_u32"); break;
		case 0x187: KYTY_NI("v_cvt_u32_f32"); break;
		case 0x188: KYTY_NI("v_cvt_i32_f32"); break;
		case 0x189: KYTY_NI("v_mov_fed_b32"); break;
		case 0x18A: KYTY_NI("v_cvt_f16_f32"); break;
		case 0x18B: KYTY_NI("v_cvt_f32_f16"); break;
		case 0x18C: KYTY_NI("v_cvt_rpi_i32_f32"); break;
		case 0x18D: KYTY_NI("v_cvt_flr_i32_f32"); break;
		case 0x18E: KYTY_NI("v_cvt_off_f32_i4"); break;
		case 0x18F: KYTY_NI("v_cvt_f32_f64"); break;
		case 0x190: KYTY_NI("v_cvt_f64_f32"); break;
		case 0x191: KYTY_NI("v_cvt_f32_ubyte0"); break;
		case 0x192: KYTY_NI("v_cvt_f32_ubyte1"); break;
		case 0x193: KYTY_NI("v_cvt_f32_ubyte2"); break;
		case 0x194: KYTY_NI("v_cvt_f32_ubyte3"); break;
		case 0x195: KYTY_NI("v_cvt_u32_f64"); break;
		case 0x196: KYTY_NI("v_cvt_f64_u32"); break;
		case 0x197: KYTY_NI("v_trunc_f64"); break;
		case 0x198: KYTY_NI("v_ceil_f64"); break;
		case 0x199: KYTY_NI("v_rndne_f64"); break;
		case 0x19A: KYTY_NI("v_floor_f64"); break;
		case 0x1a0: inst.type = ShaderInstructionType::VFractF32; break;
		case 0x1a1: inst.type = ShaderInstructionType::VTruncF32; break;
		case 0x1a2: inst.type = ShaderInstructionType::VCeilF32; break;
		case 0x1a3: inst.type = ShaderInstructionType::VRndneF32; break;
		case 0x1a4: inst.type = ShaderInstructionType::VFloorF32; break;
		case 0x1a5: inst.type = ShaderInstructionType::VExpF32; break;
		case 0x1A6: KYTY_NI("v_log_clamp_f32"); break;
		case 0x1a7: inst.type = ShaderInstructionType::VLogF32; break;
		case 0x1A8: KYTY_NI("v_rcp_clamp_f32"); break;
		case 0x1A9: KYTY_NI("v_rcp_legacy_f32"); break;
		case 0x1aa: inst.type = ShaderInstructionType::VRcpF32; break;
		case 0x1AB: KYTY_NI("v_rcp_iflag_f32"); break;
		case 0x1AC: KYTY_NI("v_rsq_clamp_f32"); break;
		case 0x1AD: KYTY_NI("v_rsq_legacy_f32"); break;
		case 0x1ae: inst.type = ShaderInstructionType::VRsqF32; break;
		case 0x1AF: KYTY_NI("v_rcp_f64"); break;
		case 0x1B0: KYTY_NI("v_rcp_clamp_f64"); break;
		case 0x1B1: KYTY_NI("v_rsq_f64"); break;
		case 0x1B2: KYTY_NI("v_rsq_clamp_f64"); break;
		case 0x1b3: inst.type = ShaderInstructionType::VSqrtF32; break;
		case 0x1B4: KYTY_NI("v_sqrt_f64"); break;
		case 0x1b5: inst.type = ShaderInstructionType::VSinF32; break;
		case 0x1b6: inst.type = ShaderInstructionType::VCosF32; break;
		case 0x1B7: KYTY_NI("v_not_b32"); break;
		case 0x1B8: KYTY_NI("v_bfrev_b32"); break;
		case 0x1B9: KYTY_NI("v_ffbh_u32"); break;
		case 0x1BA: KYTY_NI("v_ffbl_b32"); break;
		case 0x1BB: KYTY_NI("v_ffbh_i32"); break;
		case 0x1BC: KYTY_NI("v_frexp_exp_i32_f64"); break;
		case 0x1BD: KYTY_NI("v_frexp_mant_f64"); break;
		case 0x1BE: KYTY_NI("v_fract_f64"); break;
		case 0x1BF: KYTY_NI("v_frexp_exp_i32_f32"); break;
		case 0x1C0: KYTY_NI("v_frexp_mant_f32"); break;
		case 0x1C1: KYTY_NI("v_clrexcp"); break;
		case 0x1C2: KYTY_NI("v_movreld_b32"); break;
		case 0x1C3: KYTY_NI("v_movrels_b32"); break;
		case 0x1C4: KYTY_NI("v_movrelsd_b32"); break;
		case 0x1C5: KYTY_NI("v_log_legacy_f32"); break;
		case 0x1C6: KYTY_NI("v_exp_legacy_f32"); break;
		case 0x1D0: KYTY_NI("v_cvt_f16_u16"); break;
		case 0x1D1: KYTY_NI("v_cvt_f16_i16"); break;
		case 0x1D2: KYTY_NI("v_cvt_u16_f16"); break;
		case 0x1D3: KYTY_NI("v_cvt_i16_f16"); break;
		case 0x1D4: KYTY_NI("v_rcp_f16"); break;
		case 0x1D5: KYTY_NI("v_sqrt_f16"); break;
		case 0x1D6: KYTY_NI("v_rsq_f16"); break;
		case 0x1D7: KYTY_NI("v_log_f16"); break;
		case 0x1D8: KYTY_NI("v_exp_f16"); break;
		case 0x1D9: KYTY_NI("v_frexp_mant_f16"); break;
		case 0x1DA: KYTY_NI("v_frexp_exp_i16_f16"); break;
		case 0x1DB: KYTY_NI("v_floor_f16"); break;
		case 0x1DC: KYTY_NI("v_ceil_f16"); break;
		case 0x1DD: KYTY_NI("v_trunc_f16"); break;
		case 0x1DE: KYTY_NI("v_rndne_f16"); break;
		case 0x1DF: KYTY_NI("v_fract_f16"); break;
		case 0x1E0: KYTY_NI("v_sin_f16"); break;
		case 0x1E1: KYTY_NI("v_cos_f16"); break;
		case 0x1E2: KYTY_NI("v_sat_pk_u8_i16"); break;
		case 0x1E3: KYTY_NI("v_cvt_norm_i16_f16"); break;
		case 0x1E4: KYTY_NI("v_cvt_norm_u16_f16"); break;
		case 0x1E5: KYTY_NI("v_swap_b32"); break;

		default: KYTY_UNKNOWN_OP();
	}

	if (inst.dst2.type == ShaderOperandType::Unknown)
	{
		if ((abs & 0x1u) != 0)
		{
			inst.src[0].absolute = true;
		}
		if ((abs & 0x2u) != 0)
		{
			inst.src[1].absolute = true;
		}
		if ((abs & 0x4u) != 0)
		{
			inst.src[2].absolute = true;
		}

		if (!next_gen)
		{
			inst.dst.clamp = (clamp != 0);
		}
	}

	if (next_gen)
	{
		inst.dst.clamp = (clamp != 0);
	}

	dst->GetInstructions().Add(inst);

	return size;
}

KYTY_SHADER_PARSER(shader_parse_exp)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("exp");

	uint32_t vm     = (buffer[0] >> 12u) & 0x1u;
	uint32_t done   = (buffer[0] >> 11u) & 0x1u;
	uint32_t compr  = (buffer[0] >> 10u) & 0x1u;
	uint32_t target = (buffer[0] >> 4u) & 0x3fu;
	uint32_t en     = (buffer[0] >> 0u) & 0xfu;

	uint32_t vsrc0 = (buffer[1] >> 0u) & 0xffu;
	uint32_t vsrc1 = (buffer[1] >> 8u) & 0xffu;
	uint32_t vsrc2 = (buffer[1] >> 16u) & 0xffu;
	uint32_t vsrc3 = (buffer[1] >> 24u) & 0xffu;

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.src[0]  = operand_parse(vsrc0 + 256);
	inst.src[1]  = operand_parse(vsrc1 + 256);
	inst.src[2]  = operand_parse(vsrc2 + 256);
	inst.src[3]  = operand_parse(vsrc3 + 256);
	inst.src_num = 4;

	inst.type = ShaderInstructionType::Exp;

	switch (target)
	{
		case 0x00:
			if (done != 0 && compr != 0 && vm != 0 && en == 0x0)
			{
				inst.format  = ShaderInstructionFormat::Mrt0OffOffComprVmDone;
				inst.src_num = 0;
			} else if (done != 0 && compr != 0 && vm != 0 && en == 0xf)
			{
				inst.format  = ShaderInstructionFormat::Mrt0Vsrc0Vsrc1ComprVmDone;
				inst.src_num = 2;
			} else if (done != 0 && compr == 0 && vm != 0 && en == 0xf)
			{
				inst.format = ShaderInstructionFormat::Mrt0Vsrc0Vsrc1Vsrc2Vsrc3VmDone;
			};
			break;
		case 0x0c:
			if (done != 0 && en == 0xf)
			{
				inst.format = ShaderInstructionFormat::Pos0Vsrc0Vsrc1Vsrc2Vsrc3Done;
			};
			break;
		case 0x14:
			if (done != 0 && en == 0x1)
			{
				inst.format  = ShaderInstructionFormat::PrimVsrc0OffOffOffDone;
				inst.src_num = 1;
			};
			break;

		default: break;
	}

	if (inst.format == ShaderInstructionFormat::Unknown && done == 0 && compr == 0 && vm == 0 && en == 0xf)
	{
		switch (target)
		{
			case 0x20: inst.format = ShaderInstructionFormat::Param0Vsrc0Vsrc1Vsrc2Vsrc3; break;
			case 0x21: inst.format = ShaderInstructionFormat::Param1Vsrc0Vsrc1Vsrc2Vsrc3; break;
			case 0x22: inst.format = ShaderInstructionFormat::Param2Vsrc0Vsrc1Vsrc2Vsrc3; break;
			case 0x23: inst.format = ShaderInstructionFormat::Param3Vsrc0Vsrc1Vsrc2Vsrc3; break;
			case 0x24: inst.format = ShaderInstructionFormat::Param4Vsrc0Vsrc1Vsrc2Vsrc3; break;
			default: break;
		}
	}

	if (inst.format == ShaderInstructionFormat::Unknown)
	{
		printf("%s", dst->DbgDump().c_str());
		EXIT("%s\n"
		     "unknown exp target: 0x%02" PRIx32 " at addr 0x%08" PRIx32 " (hash0 = 0x%08" PRIx32 ", crc32 = 0x%08" PRIx32 ")\n",
		     dst->DbgDump().c_str(), target, pc, dst->GetHash0(), dst->GetCrc32());
	}

	dst->GetInstructions().Add(inst);

	return 2;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_smem)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("smem");

	uint32_t opcode  = (buffer[0] >> 18u) & 0xffu;
	uint32_t glc     = (buffer[0] >> 16u) & 0x1u;
	uint32_t dlc     = (buffer[0] >> 14u) & 0x1u;
	uint32_t sdst    = (buffer[0] >> 6u) & 0x7fu;
	uint32_t sbase   = (buffer[0] >> 0u) & 0x3fu;
	uint32_t soffset = (buffer[1] >> 25u) & 0x7fu;
	auto     offset  = static_cast<int32_t>((buffer[1] >> 0u) & 0x1fffffu);

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.dst     = operand_parse(sdst);
	inst.src_num = 2;
	inst.src[0]  = operand_parse(sbase * 2);
	inst.src[1]  = operand_parse(soffset);

	uint32_t size = 2;

	EXIT_NOT_IMPLEMENTED(glc != 0);
	EXIT_NOT_IMPLEMENTED(dlc != 0);
	EXIT_NOT_IMPLEMENTED(inst.src[0].type == ShaderOperandType::LiteralConstant);
	EXIT_NOT_IMPLEMENTED(inst.src[1].type == ShaderOperandType::LiteralConstant);

	if (inst.src[1].type == ShaderOperandType::Null)
	{
		struct
		{
			int x : 21;
		} s {};

		s.x = offset;

		inst.src[1].type       = ShaderOperandType::IntegerInlineConstant;
		inst.src[1].constant.i = s.x;
		inst.src[1].size       = 0;
	} else
	{
		EXIT_NOT_IMPLEMENTED(offset != 0);
	}

	switch (opcode)
	{
		case 0x00:
			inst.type        = ShaderInstructionType::SLoadDword;
			inst.format      = ShaderInstructionFormat::SdstSbaseSoffset;
			inst.src[0].size = 2;
			inst.dst.size    = 1;
			break;
		case 0x01:
			inst.type        = ShaderInstructionType::SLoadDwordx2;
			inst.format      = ShaderInstructionFormat::Sdst2Ssrc02Ssrc1;
			inst.src[0].size = 2;
			inst.dst.size    = 2;
			break;
		case 0x02:
			inst.type        = ShaderInstructionType::SLoadDwordx4;
			inst.format      = ShaderInstructionFormat::Sdst4SbaseSoffset;
			inst.src[0].size = 2;
			inst.dst.size    = 4;
			break;
		case 0x03: KYTY_NI("s_load_dwordx8"); break;
		case 0x04: KYTY_NI("s_load_dwordx16"); break;
		case 0x08: KYTY_NI("s_buffer_load_dword"); break;
		case 0x09: KYTY_NI("s_buffer_load_dwordx2"); break;
		case 0x0a:
			inst.type        = ShaderInstructionType::SBufferLoadDwordx4;
			inst.format      = ShaderInstructionFormat::Sdst4SvSoffset;
			inst.src[0].size = 4;
			inst.dst.size    = 4;
			break;
		case 0x0B: KYTY_NI("s_buffer_load_dwordx8"); break;
		case 0x0c:
			inst.type        = ShaderInstructionType::SBufferLoadDwordx16;
			inst.format      = ShaderInstructionFormat::Sdst16SvSoffset;
			inst.src[0].size = 4;
			inst.dst.size    = 16;
			break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_smrd)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("smrd");

	uint32_t opcode = (buffer[0] >> 22u) & 0x1fu;
	uint32_t sdst   = (buffer[0] >> 15u) & 0x7fu;
	uint32_t sbase  = (buffer[0] >> 9u) & 0x3fu;
	uint32_t imm    = (buffer[0] >> 8u) & 0x1u;
	uint32_t offset = (buffer[0] >> 0u) & 0xffu;

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.dst     = operand_parse(sdst);
	inst.src_num = 2;
	inst.src[0]  = operand_parse(sbase * 2);

	uint32_t size = 1;

	if (imm == 1)
	{
		inst.src[1].type       = ShaderOperandType::LiteralConstant;
		inst.src[1].constant.u = offset << 2u;
	} else
	{
		inst.src[1] = operand_parse(offset);

		if (inst.src[1].type == ShaderOperandType::LiteralConstant)
		{
			inst.src[1].constant.u = buffer[size];
			size++;
		}
	}

	switch (opcode)
	{
		case 0x00: KYTY_NI("s_load_dword"); break;
		case 0x01: KYTY_NI("s_load_dwordx2"); break;
		case 0x02:
			inst.type        = ShaderInstructionType::SLoadDwordx4;
			inst.format      = ShaderInstructionFormat::Sdst4SbaseSoffset;
			inst.src[0].size = 2;
			inst.dst.size    = 4;
			break;
		case 0x03:
			inst.type        = ShaderInstructionType::SLoadDwordx8;
			inst.format      = ShaderInstructionFormat::Sdst8SbaseSoffset;
			inst.src[0].size = 2;
			inst.dst.size    = 8;
			break;
		case 0x04: KYTY_NI("s_load_dwordx16"); break;
		case 0x08:
			inst.type        = ShaderInstructionType::SBufferLoadDword;
			inst.format      = ShaderInstructionFormat::SdstSvSoffset;
			inst.src[0].size = 4;
			break;
		case 0x09:
			inst.type        = ShaderInstructionType::SBufferLoadDwordx2;
			inst.format      = ShaderInstructionFormat::Sdst2SvSoffset;
			inst.src[0].size = 4;
			inst.dst.size    = 2;
			break;
		case 0x0a:
			inst.type        = ShaderInstructionType::SBufferLoadDwordx4;
			inst.format      = ShaderInstructionFormat::Sdst4SvSoffset;
			inst.src[0].size = 4;
			inst.dst.size    = 4;
			break;
		case 0x0b:
			inst.type        = ShaderInstructionType::SBufferLoadDwordx8;
			inst.format      = ShaderInstructionFormat::Sdst8SvSoffset;
			inst.src[0].size = 4;
			inst.dst.size    = 8;
			break;
		case 0x0c:
			inst.type        = ShaderInstructionType::SBufferLoadDwordx16;
			inst.format      = ShaderInstructionFormat::Sdst16SvSoffset;
			inst.src[0].size = 4;
			inst.dst.size    = 16;
			break;
		case 0x1C: KYTY_NI("s_memrealtime"); break;
		case 0x1D: KYTY_NI("s_dcache_inv_vol"); break;
		case 0x1E: KYTY_NI("s_memtime"); break;
		case 0x1F: KYTY_NI("s_dcache_inv") break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_mubuf)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("mubuf");

	uint32_t opcode = (buffer[0] >> 18u) & 0x1fu;
	uint32_t lds    = (buffer[0] >> 16u) & 0x1u;
	uint32_t glc    = (buffer[0] >> 14u) & 0x1u;
	uint32_t idxen  = (buffer[0] >> 13u) & 0x1u;
	uint32_t offen  = (buffer[0] >> 12u) & 0x1u;
	uint32_t offset = (buffer[0] >> 0u) & 0xfffu;

	uint32_t soffset = (buffer[1] >> 24u) & 0xffu;
	uint32_t tfe     = (buffer[1] >> 23u) & 0x1u;
	uint32_t slc     = (buffer[1] >> 22u) & 0x1u;
	uint32_t srsrc   = (buffer[1] >> 16u) & 0x1fu;
	uint32_t vdata   = (buffer[1] >> 8u) & 0xffu;
	uint32_t vaddr   = (buffer[1] >> 0u) & 0xffu;

	EXIT_NOT_IMPLEMENTED(idxen == 0);
	EXIT_NOT_IMPLEMENTED(offen == 1);
	EXIT_NOT_IMPLEMENTED(offset != 0);
	EXIT_NOT_IMPLEMENTED(glc == 1);
	EXIT_NOT_IMPLEMENTED(slc == 1);
	EXIT_NOT_IMPLEMENTED(lds == 1);
	EXIT_NOT_IMPLEMENTED(tfe == 1);

	uint32_t size = 2;

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.dst     = operand_parse(vdata + 256);
	inst.src_num = 3;
	inst.src[0]  = operand_parse(vaddr + 256);
	inst.src[1]  = operand_parse(srsrc * 4);
	inst.src[2]  = operand_parse(soffset);

	if (inst.src[2].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[2].constant.u = buffer[size];
		size++;
	}

	switch (opcode)
	{
		case 0x00:
			inst.type        = ShaderInstructionType::BufferLoadFormatX;
			inst.format      = ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxen;
			inst.src[1].size = 4;
			break;
		case 0x01:
			inst.type        = ShaderInstructionType::BufferLoadFormatXy;
			inst.format      = ShaderInstructionFormat::Vdata2VaddrSvSoffsIdxen;
			inst.src[1].size = 4;
			inst.dst.size    = 2;
			break;
		case 0x02:
			inst.type        = ShaderInstructionType::BufferLoadFormatXyz;
			inst.format      = ShaderInstructionFormat::Vdata3VaddrSvSoffsIdxen;
			inst.src[1].size = 4;
			inst.dst.size    = 3;
			break;
		case 0x03:
			inst.type        = ShaderInstructionType::BufferLoadFormatXyzw;
			inst.format      = ShaderInstructionFormat::Vdata4VaddrSvSoffsIdxen;
			inst.src[1].size = 4;
			inst.dst.size    = 4;
			break;
		case 0x04:
			inst.type        = ShaderInstructionType::BufferStoreFormatX;
			inst.format      = ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxen;
			inst.src[1].size = 4;
			break;
		case 0x05:
			inst.type        = ShaderInstructionType::BufferStoreFormatXy;
			inst.format      = ShaderInstructionFormat::Vdata2VaddrSvSoffsIdxen;
			inst.src[1].size = 4;
			inst.dst.size    = 2;
			break;
		case 0x06: KYTY_NI("buffer_store_format_xyz"); break;
		case 0x07: KYTY_NI("buffer_store_format_xyzw"); break;
		case 0x08: KYTY_NI("buffer_load_ubyte"); break;
		case 0x09: KYTY_NI("buffer_load_sbyte"); break;
		case 0x0A: KYTY_NI("buffer_load_ushort"); break;
		case 0x0B: KYTY_NI("buffer_load_sshort"); break;
		case 0x0c:
			inst.type        = ShaderInstructionType::BufferLoadDword;
			inst.format      = ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxen;
			inst.src[1].size = 4;
			break;
		case 0x0D: KYTY_NI("buffer_load_dwordx2"); break;
		case 0x0E: KYTY_NI("buffer_load_dwordx4"); break;
		case 0x0F: KYTY_NI("buffer_load_dwordx3"); break;
		case 0x18: KYTY_NI("buffer_store_byte"); break;
		case 0x1A: KYTY_NI("buffer_store_short"); break;
		case 0x1c:
			inst.type        = ShaderInstructionType::BufferStoreDword;
			inst.format      = ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxen;
			inst.src[1].size = 4;
			break;
		case 0x1D: KYTY_NI("buffer_store_dwordx2"); break;
		case 0x1E: KYTY_NI("buffer_store_dwordx4"); break;
		case 0x1F: KYTY_NI("buffer_store_dwordx3"); break;
		case 0x30: KYTY_NI("buffer_atomic_swap"); break;
		case 0x31: KYTY_NI("buffer_atomic_cmpswap"); break;
		case 0x32: KYTY_NI("buffer_atomic_add"); break;
		case 0x33: KYTY_NI("buffer_atomic_sub"); break;
		case 0x34:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("buffer_atomic_rsub")
			};
			break;
		case 0x35: KYTY_NI("buffer_atomic_smin"); break;
		case 0x36: KYTY_NI("buffer_atomic_umin"); break;
		case 0x37: KYTY_NI("buffer_atomic_smax"); break;
		case 0x38: KYTY_NI("buffer_atomic_umax"); break;
		case 0x39: KYTY_NI("buffer_atomic_and"); break;
		case 0x3A: KYTY_NI("buffer_atomic_or"); break;
		case 0x3B: KYTY_NI("buffer_atomic_xor"); break;
		case 0x3C: KYTY_NI("buffer_atomic_inc"); break;
		case 0x3D: KYTY_NI("buffer_atomic_dec"); break;
		case 0x3E: KYTY_NI("buffer_atomic_fcmpswap"); break;
		case 0x3F: KYTY_NI("buffer_atomic_fmin"); break;
		case 0x40: KYTY_NI("buffer_atomic_fmax"); break;
		case 0x50: KYTY_NI("buffer_atomic_swap_x2"); break;
		case 0x51: KYTY_NI("buffer_atomic_cmpswap_x2"); break;
		case 0x52: KYTY_NI("buffer_atomic_add_x2"); break;
		case 0x53: KYTY_NI("buffer_atomic_sub_x2"); break;
		case 0x54: KYTY_NI("buffer_atomic_rsub_x2"); break;
		case 0x55: KYTY_NI("buffer_atomic_smin_x2"); break;
		case 0x56: KYTY_NI("buffer_atomic_umin_x2"); break;
		case 0x57: KYTY_NI("buffer_atomic_smax_x2"); break;
		case 0x58: KYTY_NI("buffer_atomic_umax_x2"); break;
		case 0x59: KYTY_NI("buffer_atomic_and_x2"); break;
		case 0x5A: KYTY_NI("buffer_atomic_or_x2"); break;
		case 0x5B: KYTY_NI("buffer_atomic_xor_x2"); break;
		case 0x5C: KYTY_NI("buffer_atomic_inc_x2"); break;
		case 0x5D: KYTY_NI("buffer_atomic_dec_x2"); break;
		case 0x5E: KYTY_NI("buffer_atomic_fcmpswap_x2"); break;
		case 0x5F: KYTY_NI("buffer_atomic_fmin_x2"); break;
		case 0x60: KYTY_NI("buffer_atomic_fmax_x2"); break;
		case 0x71:
			if (next_gen)
			{
				KYTY_UNKNOWN_OP();
			} else
			{
				KYTY_NI("buffer_wbinvl1")
			};
			break;
		case 0x80: KYTY_NI("buffer_load_format_d16_x"); break;
		case 0x81: KYTY_NI("buffer_load_format_d16_xy"); break;
		case 0x82: KYTY_NI("buffer_load_format_d16_xyz"); break;
		case 0x83: KYTY_NI("buffer_load_format_d16_xyzw"); break;
		case 0x84: KYTY_NI("buffer_store_format_d16_x"); break;
		case 0x85: KYTY_NI("buffer_store_format_d16_xy"); break;
		case 0x86: KYTY_NI("buffer_store_format_d16_xyz"); break;
		case 0x87: KYTY_NI("buffer_store_format_d16_xyzw"); break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_ds)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("ds");

	uint32_t opcode  = (buffer[0] >> 18u) & 0xffu;
	uint32_t gds     = (buffer[0] >> 17u) & 0x1u;
	uint32_t offset0 = (buffer[0] >> 0u) & 0xffu;
	uint32_t offset1 = (buffer[0] >> 8u) & 0xffu;

	uint32_t vdst  = (buffer[1] >> 24u) & 0xffu;
	uint32_t data1 = (buffer[1] >> 16u) & 0xffu;
	uint32_t data0 = (buffer[1] >> 8u) & 0xffu;
	uint32_t addr  = (buffer[1] >> 0u) & 0xffu;

	EXIT_NOT_IMPLEMENTED(addr != 0);
	EXIT_NOT_IMPLEMENTED(data0 != 0);
	EXIT_NOT_IMPLEMENTED(data1 != 0);
	EXIT_NOT_IMPLEMENTED(offset0 != 0);
	EXIT_NOT_IMPLEMENTED(offset1 != 0);
	EXIT_NOT_IMPLEMENTED(gds == 0);

	uint32_t size = 2;

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.dst     = operand_parse(vdst + 256);
	inst.src_num = 0;

	switch (opcode) // NOLINT
	{
		case 0x00: KYTY_NI("ds_add_u32"); break;
		case 0x01: KYTY_NI("ds_sub_u32"); break;
		case 0x02: KYTY_NI("ds_rsub_u32"); break;
		case 0x03: KYTY_NI("ds_inc_u32"); break;
		case 0x04: KYTY_NI("ds_dec_u32"); break;
		case 0x05: KYTY_NI("ds_min_i32"); break;
		case 0x06: KYTY_NI("ds_max_i32"); break;
		case 0x07: KYTY_NI("ds_min_u32"); break;
		case 0x08: KYTY_NI("ds_max_u32"); break;
		case 0x09: KYTY_NI("ds_and_b32"); break;
		case 0x0A: KYTY_NI("ds_or_b32"); break;
		case 0x0B: KYTY_NI("ds_xor_b32"); break;
		case 0x0C: KYTY_NI("ds_mskor_b32"); break;
		case 0x0D: KYTY_NI("ds_write_b32"); break;
		case 0x0E: KYTY_NI("ds_write2_b32"); break;
		case 0x0F: KYTY_NI("ds_write2st64_b32"); break;
		case 0x10: KYTY_NI("ds_cmpst_b32"); break;
		case 0x11: KYTY_NI("ds_cmpst_f32"); break;
		case 0x12: KYTY_NI("ds_min_f32"); break;
		case 0x13: KYTY_NI("ds_max_f32"); break;
		case 0x14: KYTY_NI("ds_nop"); break;
		case 0x18: KYTY_NI("ds_gws_sema_release_all"); break;
		case 0x19: KYTY_NI("ds_gws_init"); break;
		case 0x1A: KYTY_NI("ds_gws_sema_v"); break;
		case 0x1B: KYTY_NI("ds_gws_sema_br"); break;
		case 0x1C: KYTY_NI("ds_gws_sema_p"); break;
		case 0x1D: KYTY_NI("ds_gws_barrier"); break;
		case 0x1E: KYTY_NI("ds_write_b8"); break;
		case 0x1F: KYTY_NI("ds_write_b16"); break;
		case 0x20: KYTY_NI("ds_add_rtn_u32"); break;
		case 0x21: KYTY_NI("ds_sub_rtn_u32"); break;
		case 0x22: KYTY_NI("ds_rsub_rtn_u32"); break;
		case 0x23: KYTY_NI("ds_inc_rtn_u32"); break;
		case 0x24: KYTY_NI("ds_dec_rtn_u32"); break;
		case 0x25: KYTY_NI("ds_min_rtn_i32"); break;
		case 0x26: KYTY_NI("ds_max_rtn_i32"); break;
		case 0x27: KYTY_NI("ds_min_rtn_u32"); break;
		case 0x28: KYTY_NI("ds_max_rtn_u32"); break;
		case 0x29: KYTY_NI("ds_and_rtn_b32"); break;
		case 0x2A: KYTY_NI("ds_or_rtn_b32"); break;
		case 0x2B: KYTY_NI("ds_xor_rtn_b32"); break;
		case 0x2C: KYTY_NI("ds_mskor_rtn_b32"); break;
		case 0x2D: KYTY_NI("ds_wrxchg_rtn_b32"); break;
		case 0x2E: KYTY_NI("ds_wrxchg2_rtn_b32"); break;
		case 0x2F: KYTY_NI("ds_wrxchg2st64_rtn_b32"); break;
		case 0x30: KYTY_NI("ds_cmpst_rtn_b32"); break;
		case 0x31: KYTY_NI("ds_cmpst_rtn_f32"); break;
		case 0x32: KYTY_NI("ds_min_rtn_f32"); break;
		case 0x33: KYTY_NI("ds_max_rtn_f32"); break;
		case 0x34: KYTY_NI("ds_wrap_rtn_b32"); break;
		case 0x35: KYTY_NI("ds_swizzle_b32"); break;
		case 0x36: KYTY_NI("ds_read_b32"); break;
		case 0x37: KYTY_NI("ds_read2_b32"); break;
		case 0x38: KYTY_NI("ds_read2st64_b32"); break;
		case 0x39: KYTY_NI("ds_read_i8"); break;
		case 0x3A: KYTY_NI("ds_read_u8"); break;
		case 0x3B: KYTY_NI("ds_read_i16"); break;
		case 0x3C: KYTY_NI("ds_read_u16"); break;
		case 0x3d:
			inst.type   = ShaderInstructionType::DsConsume;
			inst.format = ShaderInstructionFormat::VdstGds;
			break;
		case 0x3e:
			inst.type   = ShaderInstructionType::DsAppend;
			inst.format = ShaderInstructionFormat::VdstGds;
			break;
		case 0x3F: KYTY_NI("ds_ordered_count"); break;
		case 0x40: KYTY_NI("ds_add_u64"); break;
		case 0x41: KYTY_NI("ds_sub_u64"); break;
		case 0x42: KYTY_NI("ds_rsub_u64"); break;
		case 0x43: KYTY_NI("ds_inc_u64"); break;
		case 0x44: KYTY_NI("ds_dec_u64"); break;
		case 0x45: KYTY_NI("ds_min_i64"); break;
		case 0x46: KYTY_NI("ds_max_i64"); break;
		case 0x47: KYTY_NI("ds_min_u64"); break;
		case 0x48: KYTY_NI("ds_max_u64"); break;
		case 0x49: KYTY_NI("ds_and_b64"); break;
		case 0x4A: KYTY_NI("ds_or_b64"); break;
		case 0x4B: KYTY_NI("ds_xor_b64"); break;
		case 0x4C: KYTY_NI("ds_mskor_b64"); break;
		case 0x4D: KYTY_NI("ds_write_b64"); break;
		case 0x4E: KYTY_NI("ds_write2_b64"); break;
		case 0x4F: KYTY_NI("ds_write2st64_b64"); break;
		case 0x50: KYTY_NI("ds_cmpst_b64"); break;
		case 0x51: KYTY_NI("ds_cmpst_f64"); break;
		case 0x52: KYTY_NI("ds_min_f64"); break;
		case 0x53: KYTY_NI("ds_max_f64"); break;
		case 0x60: KYTY_NI("ds_add_rtn_u64"); break;
		case 0x61: KYTY_NI("ds_sub_rtn_u64"); break;
		case 0x62: KYTY_NI("ds_rsub_rtn_u64"); break;
		case 0x63: KYTY_NI("ds_inc_rtn_u64"); break;
		case 0x64: KYTY_NI("ds_dec_rtn_u64"); break;
		case 0x65: KYTY_NI("ds_min_rtn_i64"); break;
		case 0x66: KYTY_NI("ds_max_rtn_i64"); break;
		case 0x67: KYTY_NI("ds_min_rtn_u64"); break;
		case 0x68: KYTY_NI("ds_max_rtn_u64"); break;
		case 0x69: KYTY_NI("ds_and_rtn_b64"); break;
		case 0x6A: KYTY_NI("ds_or_rtn_b64"); break;
		case 0x6B: KYTY_NI("ds_xor_rtn_b64"); break;
		case 0x6C: KYTY_NI("ds_mskor_rtn_b64"); break;
		case 0x6D: KYTY_NI("ds_wrxchg_rtn_b64"); break;
		case 0x6E: KYTY_NI("ds_wrxchg2_rtn_b64"); break;
		case 0x6F: KYTY_NI("ds_wrxchg2st64_rtn_b64"); break;
		case 0x70: KYTY_NI("ds_cmpst_rtn_b64"); break;
		case 0x71: KYTY_NI("ds_cmpst_rtn_f64"); break;
		case 0x72: KYTY_NI("ds_min_rtn_f64"); break;
		case 0x73: KYTY_NI("ds_max_rtn_f64"); break;
		case 0x76: KYTY_NI("ds_read_b64"); break;
		case 0x77: KYTY_NI("ds_read2_b64"); break;
		case 0x78: KYTY_NI("ds_read2st64_b64"); break;
		case 0x7E: KYTY_NI("ds_condxchg32_rtn_b64"); break;
		case 0x80: KYTY_NI("ds_add_src2_u32"); break;
		case 0x81: KYTY_NI("ds_sub_src2_u32"); break;
		case 0x82: KYTY_NI("ds_rsub_src2_u32"); break;
		case 0x83: KYTY_NI("ds_inc_src2_u32"); break;
		case 0x84: KYTY_NI("ds_dec_src2_u32"); break;
		case 0x85: KYTY_NI("ds_min_src2_i32"); break;
		case 0x86: KYTY_NI("ds_max_src2_i32"); break;
		case 0x87: KYTY_NI("ds_min_src2_u32"); break;
		case 0x88: KYTY_NI("ds_max_src2_u32"); break;
		case 0x89: KYTY_NI("ds_and_src2_b32"); break;
		case 0x8A: KYTY_NI("ds_or_src2_b32"); break;
		case 0x8B: KYTY_NI("ds_xor_src2_b32"); break;
		case 0x8D: KYTY_NI("ds_write_src2_b32"); break;
		case 0x92: KYTY_NI("ds_min_src2_f32"); break;
		case 0x93: KYTY_NI("ds_max_src2_f32"); break;
		case 0xC0: KYTY_NI("ds_add_src2_u64"); break;
		case 0xC1: KYTY_NI("ds_sub_src2_u64"); break;
		case 0xC2: KYTY_NI("ds_rsub_src2_u64"); break;
		case 0xC3: KYTY_NI("ds_inc_src2_u64"); break;
		case 0xC4: KYTY_NI("ds_dec_src2_u64"); break;
		case 0xC5: KYTY_NI("ds_min_src2_i64"); break;
		case 0xC6: KYTY_NI("ds_max_src2_i64"); break;
		case 0xC7: KYTY_NI("ds_min_src2_u64"); break;
		case 0xC8: KYTY_NI("ds_max_src2_u64"); break;
		case 0xC9: KYTY_NI("ds_and_src2_b64"); break;
		case 0xCA: KYTY_NI("ds_or_src2_b64"); break;
		case 0xCB: KYTY_NI("ds_xor_src2_b64"); break;
		case 0xCD: KYTY_NI("ds_write_src2_b64"); break;
		case 0xD2: KYTY_NI("ds_min_src2_f64"); break;
		case 0xD3: KYTY_NI("ds_max_src2_f64"); break;
		case 0xDE: KYTY_NI("ds_write_b96"); break;
		case 0xDF: KYTY_NI("ds_write_b128"); break;
		case 0xFD: KYTY_NI("ds_condxchg32_rtn_b128"); break;
		case 0xFE: KYTY_NI("ds_read_b96"); break;
		case 0xFF: KYTY_NI("ds_read_b128"); break;

		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_mimg)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("mimg");

	uint32_t slc    = (buffer[0] >> 25u) & 0x1u;
	uint32_t opcode = (buffer[0] >> 18u) & 0x7fu;
	uint32_t lwe    = (buffer[0] >> 17u) & 0x1u;
	uint32_t tff    = (buffer[0] >> 16u) & 0x1u;
	uint32_t r128   = (buffer[0] >> 15u) & 0x1u;
	uint32_t da     = (buffer[0] >> 14u) & 0x1u;
	uint32_t glc    = (buffer[0] >> 13u) & 0x1u;
	uint32_t unrm   = (buffer[0] >> 12u) & 0x1u;
	uint32_t dmask  = (buffer[0] >> 8u) & 0xfu;

	uint32_t ssamp = (buffer[1] >> 21u) & 0x1fu; // S#
	uint32_t srsrc = (buffer[1] >> 16u) & 0x1fu; // T#
	uint32_t vdata = (buffer[1] >> 8u) & 0xffu;
	uint32_t vaddr = (buffer[1] >> 0u) & 0xffu;

	EXIT_NOT_IMPLEMENTED(da == 1);
	EXIT_NOT_IMPLEMENTED(r128 == 1);
	EXIT_NOT_IMPLEMENTED(tff == 1);
	EXIT_NOT_IMPLEMENTED(lwe == 1);
	EXIT_NOT_IMPLEMENTED(glc == 1);
	EXIT_NOT_IMPLEMENTED(slc == 1);
	EXIT_NOT_IMPLEMENTED(unrm == 1);
	// EXIT_NOT_IMPLEMENTED(dmask != 0xf && dmask != 0x7);

	uint32_t size = 2;

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.dst     = operand_parse(vdata + 256);
	inst.src_num = 3;
	inst.src[0]  = operand_parse(vaddr + 256);
	inst.src[1]  = operand_parse(srsrc * 4);
	inst.src[2]  = operand_parse(ssamp * 4);

	switch (opcode)
	{
		case 0x00:
			inst.type        = ShaderInstructionType::ImageLoad;
			inst.src[0].size = 3;
			inst.src[1].size = 8;
			inst.src_num     = 2;
			if (dmask == 0xf)
			{
				inst.format   = ShaderInstructionFormat::Vdata4Vaddr3StDmaskF;
				inst.dst.size = 4;
			}
			break;
		case 0x01: KYTY_NI("image_load_mip"); break;
		case 0x02: KYTY_NI("image_load_pck"); break;
		case 0x03: KYTY_NI("image_load_pck_sgn"); break;
		case 0x04: KYTY_NI("image_load_mip_pck"); break;
		case 0x05: KYTY_NI("image_load_mip_pck_sgn"); break;
		case 0x08:
			inst.type        = ShaderInstructionType::ImageStore;
			inst.src[0].size = 3;
			inst.src[1].size = 8;
			inst.src_num     = 2;
			if (dmask == 0xf)
			{
				inst.format   = ShaderInstructionFormat::Vdata4Vaddr3StDmaskF;
				inst.dst.size = 4;
			}
			break;
		case 0x09:
			inst.type        = ShaderInstructionType::ImageStoreMip;
			inst.src[0].size = 4;
			inst.src[1].size = 8;
			inst.src_num     = 2;
			if (dmask == 0xf)
			{
				inst.format   = ShaderInstructionFormat::Vdata4Vaddr4StDmaskF;
				inst.dst.size = 4;
			}
			break;
		case 0x0A: KYTY_NI("image_store_pck"); break;
		case 0x0B: KYTY_NI("image_store_mip_pck"); break;
		case 0x0E: KYTY_NI("image_get_resinfo"); break;
		case 0x0F: KYTY_NI("image_atomic_swap"); break;
		case 0x10: KYTY_NI("image_atomic_cmpswap"); break;
		case 0x11: KYTY_NI("image_atomic_add"); break;
		case 0x12: KYTY_NI("image_atomic_sub"); break;
		case 0x13: KYTY_NI("image_atomic_rsub"); break;
		case 0x14: KYTY_NI("image_atomic_smin"); break;
		case 0x15: KYTY_NI("image_atomic_umin"); break;
		case 0x16: KYTY_NI("image_atomic_smax"); break;
		case 0x17: KYTY_NI("image_atomic_umax"); break;
		case 0x18: KYTY_NI("image_atomic_and"); break;
		case 0x19: KYTY_NI("image_atomic_or"); break;
		case 0x1A: KYTY_NI("image_atomic_xor"); break;
		case 0x1B: KYTY_NI("image_atomic_inc"); break;
		case 0x1C: KYTY_NI("image_atomic_dec"); break;
		case 0x1D: KYTY_NI("image_atomic_fcmpswap"); break;
		case 0x1E: KYTY_NI("image_atomic_fmin"); break;
		case 0x1F: KYTY_NI("image_atomic_fmax"); break;
		case 0x20:
			inst.type        = ShaderInstructionType::ImageSample;
			inst.src[0].size = 3;
			inst.src[1].size = 8;
			inst.src[2].size = 4;
			switch (dmask)
			{
				case 0x1:
				{
					inst.format   = ShaderInstructionFormat::Vdata1Vaddr3StSsDmask1;
					inst.dst.size = 1;
					break;
				}
				case 0x3:
				{
					inst.format   = ShaderInstructionFormat::Vdata2Vaddr3StSsDmask3;
					inst.dst.size = 2;
					break;
				}
				case 0x5:
				{
					inst.format   = ShaderInstructionFormat::Vdata2Vaddr3StSsDmask5;
					inst.dst.size = 2;
					break;
				}
				case 0x7:
				{
					inst.format   = ShaderInstructionFormat::Vdata3Vaddr3StSsDmask7;
					inst.dst.size = 3;
					break;
				}
				case 0x8:
				{
					inst.format   = ShaderInstructionFormat::Vdata1Vaddr3StSsDmask8;
					inst.dst.size = 1;
					break;
				}
				case 0x9:
				{
					inst.format   = ShaderInstructionFormat::Vdata2Vaddr3StSsDmask9;
					inst.dst.size = 2;
					break;
				}
				case 0xf:
				{
					inst.format   = ShaderInstructionFormat::Vdata4Vaddr3StSsDmaskF;
					inst.dst.size = 4;
					break;
				}
				default:;
			}
			break;
		case 0x21: KYTY_NI("image_sample_cl"); break;
		case 0x22: KYTY_NI("image_sample_d"); break;
		case 0x23: KYTY_NI("image_sample_d_cl"); break;
		case 0x24: KYTY_NI("image_sample_l"); break;
		case 0x25: KYTY_NI("image_sample_b"); break;
		case 0x26: KYTY_NI("image_sample_b_cl"); break;
		case 0x27:
			inst.type        = ShaderInstructionType::ImageSampleLz;
			inst.src[0].size = 3;
			inst.src[1].size = 8;
			inst.src[2].size = 4;
			switch (dmask) // NOLINT
			{
				case 0x7:
				{
					inst.format   = ShaderInstructionFormat::Vdata3Vaddr3StSsDmask7;
					inst.dst.size = 3;
					break;
				}
				default:;
			}
			break;
		case 0x28: KYTY_NI("image_sample_c"); break;
		case 0x29: KYTY_NI("image_sample_c_cl"); break;
		case 0x2A: KYTY_NI("image_sample_c_d"); break;
		case 0x2B: KYTY_NI("image_sample_c_d_cl"); break;
		case 0x2C: KYTY_NI("image_sample_c_l"); break;
		case 0x2D: KYTY_NI("image_sample_c_b"); break;
		case 0x2E: KYTY_NI("image_sample_c_b_cl"); break;
		case 0x2F: KYTY_NI("image_sample_c_lz"); break;
		case 0x30: KYTY_NI("image_sample_o"); break;
		case 0x31: KYTY_NI("image_sample_cl_o"); break;
		case 0x32: KYTY_NI("image_sample_d_o"); break;
		case 0x33: KYTY_NI("image_sample_d_cl_o"); break;
		case 0x34: KYTY_NI("image_sample_l_o"); break;
		case 0x35: KYTY_NI("image_sample_b_o"); break;
		case 0x36: KYTY_NI("image_sample_b_cl_o"); break;
		case 0x37:
			inst.type        = ShaderInstructionType::ImageSampleLzO;
			inst.src[0].size = 4;
			inst.src[1].size = 8;
			inst.src[2].size = 4;
			switch (dmask) // NOLINT
			{
				case 0x7:
				{
					inst.format   = ShaderInstructionFormat::Vdata3Vaddr4StSsDmask7;
					inst.dst.size = 3;
					break;
				}
				default:;
			}
			break;
		case 0x38: KYTY_NI("image_sample_c_o"); break;
		case 0x39: KYTY_NI("image_sample_c_cl_o"); break;
		case 0x3A: KYTY_NI("image_sample_c_d_o"); break;
		case 0x3B: KYTY_NI("image_sample_c_d_cl_o"); break;
		case 0x3C: KYTY_NI("image_sample_c_l_o"); break;
		case 0x3D: KYTY_NI("image_sample_c_b_o"); break;
		case 0x3E: KYTY_NI("image_sample_c_b_cl_o"); break;
		case 0x3F: KYTY_NI("image_sample_c_lz_o"); break;
		case 0x40: KYTY_NI("image_gather4"); break;
		case 0x41: KYTY_NI("image_gather4_cl"); break;
		case 0x44: KYTY_NI("image_gather4_l"); break;
		case 0x45: KYTY_NI("image_gather4_b"); break;
		case 0x46: KYTY_NI("image_gather4_b_cl"); break;
		case 0x47: KYTY_NI("image_gather4_lz"); break;
		case 0x48: KYTY_NI("image_gather4_c"); break;
		case 0x49: KYTY_NI("image_gather4_c_cl"); break;
		case 0x4C: KYTY_NI("image_gather4_c_l"); break;
		case 0x4D: KYTY_NI("image_gather4_c_b"); break;
		case 0x4E: KYTY_NI("image_gather4_c_b_cl"); break;
		case 0x4F: KYTY_NI("image_gather4_c_lz"); break;
		case 0x50: KYTY_NI("image_gather4_o"); break;
		case 0x51: KYTY_NI("image_gather4_cl_o"); break;
		case 0x54: KYTY_NI("image_gather4_l_o"); break;
		case 0x55: KYTY_NI("image_gather4_b_o"); break;
		case 0x56: KYTY_NI("image_gather4_b_cl_o"); break;
		case 0x57: KYTY_NI("image_gather4_lz_o"); break;
		case 0x58: KYTY_NI("image_gather4_c_o"); break;
		case 0x59: KYTY_NI("image_gather4_c_cl_o"); break;
		case 0x5C: KYTY_NI("image_gather4_c_l_o"); break;
		case 0x5D: KYTY_NI("image_gather4_c_b_o"); break;
		case 0x5E: KYTY_NI("image_gather4_c_b_cl_o"); break;
		case 0x5F: KYTY_NI("image_gather4_c_lz_o"); break;
		case 0x60: KYTY_NI("image_get_lod"); break;
		case 0x68: KYTY_NI("image_sample_cd"); break;
		case 0x69: KYTY_NI("image_sample_cd_cl"); break;
		case 0x6A: KYTY_NI("image_sample_c_cd"); break;
		case 0x6B: KYTY_NI("image_sample_c_cd_cl"); break;
		case 0x6C: KYTY_NI("image_sample_cd_o"); break;
		case 0x6D: KYTY_NI("image_sample_cd_cl_o"); break;
		case 0x6E: KYTY_NI("image_sample_c_cd_o"); break;
		case 0x6F: KYTY_NI("image_sample_c_cd_cl_o"); break;
		case 0x7E: KYTY_NI("image_rsrc256"); break;
		case 0x7F: KYTY_NI("image_sampler"); break;
		case 0xA0: KYTY_NI("image_sample_a"); break;
		case 0xA1: KYTY_NI("image_sample_cl_a"); break;
		case 0xA5: KYTY_NI("image_sample_b_a"); break;
		case 0xA6: KYTY_NI("image_sample_b_cl_a"); break;
		case 0xA8: KYTY_NI("image_sample_c_a"); break;
		case 0xA9: KYTY_NI("image_sample_c_cl_a"); break;
		case 0xAD: KYTY_NI("image_sample_c_b_a"); break;
		case 0xAE: KYTY_NI("image_sample_c_b_cl_a"); break;
		case 0xB0: KYTY_NI("image_sample_o_a"); break;
		case 0xB1: KYTY_NI("image_sample_cl_o_a"); break;
		case 0xB5: KYTY_NI("image_sample_b_o_a"); break;
		case 0xB6: KYTY_NI("image_sample_b_cl_o_a"); break;
		case 0xB8: KYTY_NI("image_sample_c_o_a"); break;
		case 0xB9: KYTY_NI("image_sample_c_cl_o_a"); break;
		case 0xBD: KYTY_NI("image_sample_c_b_o_a"); break;
		case 0xBE: KYTY_NI("image_sample_c_b_cl_o_a"); break;
		case 0xC0: KYTY_NI("image_gather4_a"); break;
		case 0xC1: KYTY_NI("image_gather4_cl_a"); break;
		case 0xC5: KYTY_NI("image_gather4_b_a"); break;
		case 0xC6: KYTY_NI("image_gather4_b_cl_a"); break;
		case 0xC8: KYTY_NI("image_gather4_c_a"); break;
		case 0xC9: KYTY_NI("image_gather4_c_cl_a"); break;
		case 0xCD: KYTY_NI("image_gather4_c_b_a"); break;
		case 0xCE: KYTY_NI("image_gather4_c_b_cl_a"); break;
		case 0xD0: KYTY_NI("image_gather4_o_a"); break;
		case 0xD1: KYTY_NI("image_gather4_cl_o_a"); break;
		case 0xD5: KYTY_NI("image_gather4_b_o_a"); break;
		case 0xD6: KYTY_NI("image_gather4_b_cl_o_a"); break;
		case 0xD8: KYTY_NI("image_gather4_c_o_a"); break;
		case 0xD9: KYTY_NI("image_gather4_c_cl_o_a"); break;
		case 0xDD: KYTY_NI("image_gather4_c_b_o_a"); break;
		case 0xDE: KYTY_NI("image_gather4_c_b_cl_o_a"); break;

		default: KYTY_UNKNOWN_OP();
	}

	if (inst.format == ShaderInstructionFormat::Unknown)
	{
		printf("%s", dst->DbgDump().c_str());
		EXIT("unknown mimg format for opcode: 0x%02" PRIx32 " at addr 0x%08" PRIx32 ", dmask: 0x%" PRIx32 "\n", opcode, pc, dmask);
	}

	dst->GetInstructions().Add(inst);

	return size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KYTY_SHADER_PARSER(shader_parse_mtbuf)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("mtbuf");

	uint32_t opcode = (buffer[0] >> 16u) & 0x7u;
	uint32_t dfmt   = (buffer[0] >> 19u) & 0xfu;
	uint32_t nfmt   = (buffer[0] >> 23u) & 0x7u;
	uint32_t glc    = (buffer[0] >> 14u) & 0x1u;
	uint32_t idxen  = (buffer[0] >> 13u) & 0x1u;
	uint32_t offen  = (buffer[0] >> 12u) & 0x1u;
	uint32_t offset = (buffer[0] >> 0u) & 0xfffu;

	uint32_t soffset = (buffer[1] >> 24u) & 0xffu;
	uint32_t tfe     = (buffer[1] >> 23u) & 0x1u;
	uint32_t slc     = (buffer[1] >> 22u) & 0x1u;
	uint32_t srsrc   = (buffer[1] >> 16u) & 0x1fu;
	uint32_t vdata   = (buffer[1] >> 8u) & 0xffu;
	uint32_t vaddr   = (buffer[1] >> 0u) & 0xffu;

	EXIT_NOT_IMPLEMENTED(idxen == 0);
	// EXIT_NOT_IMPLEMENTED(offen == 1);
	EXIT_NOT_IMPLEMENTED(offset != 0);
	EXIT_NOT_IMPLEMENTED(glc == 1);
	EXIT_NOT_IMPLEMENTED(slc == 1);
	EXIT_NOT_IMPLEMENTED(tfe == 1);
	// EXIT_NOT_IMPLEMENTED(dfmt != 14);
	// EXIT_NOT_IMPLEMENTED(nfmt != 7);

	if ((dfmt != 14 && dfmt != 4) || nfmt != 7)
	{
		EXIT("unknown format: dfmt = %d, nfmt = %d at addr 0x%08" PRIx32 " (hash0 = 0x%08" PRIx32 ", crc32 = 0x%08" PRIx32 ")\n", dfmt,
		     nfmt, pc, dst->GetHash0(), dst->GetCrc32());
	}

	uint32_t size = 2;

	ShaderInstruction inst;
	inst.pc      = pc;
	inst.dst     = operand_parse(vdata + 256);
	inst.src_num = 3;
	inst.src[0]  = operand_parse(vaddr + 256);
	inst.src[1]  = operand_parse(srsrc * 4);
	inst.src[2]  = operand_parse(soffset);

	if (inst.src[2].type == ShaderOperandType::LiteralConstant)
	{
		inst.src[2].constant.u = buffer[size];
		size++;
	}

	inst.src[1].size = 4;

	switch (opcode)
	{
		case 0x00:
			inst.type   = ShaderInstructionType::TBufferLoadFormatX;
			inst.format = ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxenFloat1;
			EXIT_NOT_IMPLEMENTED(offen == 1);
			EXIT_NOT_IMPLEMENTED(!(dfmt == 4 && nfmt == 7));
			break;
		case 0x01: KYTY_NI("tbuffer_load_format_xy"); break;
		case 0x02: KYTY_NI("tbuffer_load_format_xyz"); break;
		case 0x03:
			inst.type   = ShaderInstructionType::TBufferLoadFormatXyzw;
			inst.format = (offen == 1 ? ShaderInstructionFormat::Vdata4Vaddr2SvSoffsOffenIdxenFloat4
			                          : ShaderInstructionFormat::Vdata4VaddrSvSoffsIdxenFloat4);
			inst.src[0].size += static_cast<int>(offen);
			inst.dst.size = 4;
			EXIT_NOT_IMPLEMENTED(!(dfmt == 14 && nfmt == 7));
			break;
		case 0x04: KYTY_NI("tbuffer_store_format_x"); break;
		case 0x05: KYTY_NI("tbuffer_store_format_xy"); break;
		case 0x06: KYTY_NI("tbuffer_store_format_xyz"); break;
		case 0x07: KYTY_NI("tbuffer_store_format_xyzw"); break;
		case 0x08: KYTY_NI("tbuffer_load_format_d16_x"); break;
		case 0x09: KYTY_NI("tbuffer_load_format_d16_xy"); break;
		case 0x0A: KYTY_NI("tbuffer_load_format_d16_xyz"); break;
		case 0x0B: KYTY_NI("tbuffer_load_format_d16_xyzw"); break;
		case 0x0C: KYTY_NI("tbuffer_store_format_d16_x"); break;
		case 0x0D: KYTY_NI("tbuffer_store_format_d16_xy"); break;
		case 0x0E: KYTY_NI("tbuffer_store_format_d16_xyz"); break;
		case 0x0F: KYTY_NI("tbuffer_store_format_d16_xyzw"); break;
		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return size;
}

KYTY_SHADER_PARSER(shader_parse_vintrp)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer == nullptr || buffer < src);

	KYTY_TYPE_STR("vintrp");

	uint32_t opcode = (buffer[0] >> 16u) & 0x3u;
	uint32_t vdst   = (buffer[0] >> 18u) & 0xffu;
	uint32_t attr   = (buffer[0] >> 10u) & 0x3fu;
	uint32_t chan   = (buffer[0] >> 8u) & 0x3u;
	uint32_t vsrc   = (buffer[0] >> 0u) & 0xffu;

	ShaderInstruction inst;
	inst.pc                = pc;
	inst.src[0]            = operand_parse(vsrc + 256);
	inst.dst               = operand_parse(vdst + 256);
	inst.src[1].type       = ShaderOperandType::IntegerInlineConstant;
	inst.src[1].constant.u = attr;
	inst.src[2].type       = ShaderOperandType::IntegerInlineConstant;
	inst.src[2].constant.u = chan;
	inst.src_num           = 3;

	inst.format = ShaderInstructionFormat::VdstVsrcAttrChan;

	switch (opcode)
	{
		case 0x00: inst.type = ShaderInstructionType::VInterpP1F32; break;
		case 0x01: inst.type = ShaderInstructionType::VInterpP2F32; break;
		case 0x02:
			inst.type              = ShaderInstructionType::VInterpMovF32;
			inst.src[0].type       = ShaderOperandType::IntegerInlineConstant;
			inst.src[0].constant.u = vsrc & 0x3u;
			inst.src[0].size       = 0;
			break;
		default: KYTY_UNKNOWN_OP();
	}

	dst->GetInstructions().Add(inst);

	return 1;
}

KYTY_SHADER_PARSER(shader_parse)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(src == nullptr);
	EXIT_IF(buffer != nullptr);

	auto type = dst->GetType();

	dst->GetInstructions().Clear();
	dst->GetLabels().Clear();
	dst->GetIndirectLabels().Clear();

	const auto* ptr = src + pc / 4;
	for (;;)
	{
		auto instruction = ptr[0];
		auto pc          = 4 * static_cast<uint32_t>(ptr - src);

		if ((instruction & 0x80000000u) == 0x00000000)
		{
			ptr += shader_parse_vop2(pc, src, ptr, dst, next_gen);
		} else if ((instruction & 0xF8000000u) == 0xC0000000)
		{
			EXIT_NOT_IMPLEMENTED(next_gen);
			ptr += shader_parse_smrd(pc, src, ptr, dst, next_gen);
		} else if ((instruction & 0xC0000000u) == 0x80000000)
		{
			ptr += shader_parse_sop2(pc, src, ptr, dst, next_gen);
		} else
		{
			switch (instruction >> 26u)
			{
				case 0x32: ptr += shader_parse_vintrp(pc, src, ptr, dst, next_gen); break;
				case 0x34:
					EXIT_NOT_IMPLEMENTED(next_gen);
					ptr += shader_parse_vop3(pc, src, ptr, dst, next_gen);
					break;
				case 0x35:
					EXIT_NOT_IMPLEMENTED(!next_gen);
					ptr += shader_parse_vop3(pc, src, ptr, dst, next_gen);
					break;
				case 0x36: ptr += shader_parse_ds(pc, src, ptr, dst, next_gen); break;
				case 0x38: ptr += shader_parse_mubuf(pc, src, ptr, dst, next_gen); break;
				case 0x3a: ptr += shader_parse_mtbuf(pc, src, ptr, dst, next_gen); break;
				case 0x3c: ptr += shader_parse_mimg(pc, src, ptr, dst, next_gen); break;
				case 0x3d:
					EXIT_NOT_IMPLEMENTED(!next_gen);
					ptr += shader_parse_smem(pc, src, ptr, dst, next_gen);
					break;
				case 0x3e: ptr += shader_parse_exp(pc, src, ptr, dst, next_gen); break;
				default:
				{
					printf("%s", dst->DbgDump().c_str());
					EXIT("unknown code 0x%08" PRIx32 " at addr 0x%08" PRIx32 "\n", ptr[0], pc);
				}
			}
		}

		if ((instruction == 0xBF810000 && (type == ShaderType::Vertex || type == ShaderType::Pixel || type == ShaderType::Compute) &&
		     !dst->GetLabels().Contains(4 * static_cast<uint32_t>(ptr - src), [](auto label, auto pc) { return label.GetDst() == pc; })) ||
		    (instruction == 0xBE802000 && type == ShaderType::Fetch))
		{
			break;
		}
	}

	return ptr - src;
}

void ShaderParse(const uint32_t* src, ShaderCode* dst)
{
	shader_parse(0, src, nullptr, dst, Config::IsNextGen());
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
