#include "Emulator/Graphics/Shader.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/String8.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Config.h"
#include "Emulator/Graphics/GraphicsRun.h"
#include "Emulator/Graphics/HardwareContext.h"
#include "Emulator/Graphics/ShaderParse.h"
#include "Emulator/Graphics/ShaderSpirv.h"
#include "Emulator/Profiler.h"

#include "spirv-tools/libspirv.h"
#include "spirv-tools/libspirv.hpp"
#include "spirv-tools/optimizer.hpp"

#include <algorithm>
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>

//#define SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS
//#include "spirv_cross/spirv_glsl.hpp"

#ifdef KYTY_EMU_ENABLED

KYTY_ENUM_RANGE(Kyty::Libs::Graphics::ShaderInstructionType, 0, static_cast<int>(Kyty::Libs::Graphics::ShaderInstructionType::ZMax));

namespace Kyty::Libs::Graphics {

struct ShaderBinaryInfo
{
	uint8_t  signature[7];
	uint8_t  version;
	uint32_t pssl_or_cg  : 1;
	uint32_t cached      : 1;
	uint32_t type        : 4;
	uint32_t source_type : 2;
	uint32_t length      : 24;
	uint8_t  chunk_usage_base_offset_dw;
	uint8_t  num_input_usage_slots;
	uint8_t  is_srt                 : 1;
	uint8_t  is_srt_used_info_valid : 1;
	uint8_t  is_extended_usage_info : 1;
	uint8_t  reserved2              : 5;
	uint8_t  reserved3;
	uint32_t hash0;
	uint32_t hash1;
	uint32_t crc32;
};

struct ShaderUsageSlot
{
	uint8_t type;
	uint8_t slot;
	uint8_t start_register;
	uint8_t flags;
};

struct ShaderUsageInfo
{
	const uint32_t*        usage_masks = nullptr;
	const ShaderUsageSlot* slots       = nullptr;
	int                    slots_num   = 0;
	bool                   valid       = false;
};

struct ShaderParsedUsage
{
	bool fetch                     = false;
	int  fetch_reg                 = 0;
	bool vertex_buffer             = false;
	int  vertex_buffer_reg         = 0;
	bool vertex_attrib             = false;
	int  vertex_attrib_reg         = 0;
	int  storage_buffers_readwrite = 0;
	int  storage_buffers_readonly  = 0;
	int  storage_buffers_constant  = 0;
	int  textures2D_readonly       = 0;
	int  textures2D_readwrite      = 0;
	bool extended_buffer           = false;
	int  samplers                  = 0;
	int  gds_pointers              = 0;
	int  direct_sgprs              = 0;
};

struct ShaderDebugPrintfCmds
{
	uint64_t                  id = 0;
	Vector<ShaderDebugPrintf> cmds;
};

static Vector<uint64_t>*                               g_disabled_shaders = nullptr;
static Vector<ShaderDebugPrintfCmds>*                  g_debug_printfs    = nullptr;
static std::unordered_map<uint64_t, ShaderMappedData>* g_shader_map       = nullptr;

void ShaderInit()
{
	EXIT_IF(g_shader_map != nullptr);

	g_shader_map = new std::unordered_map<uint64_t, ShaderMappedData>();
}

void ShaderMapUserData(uint64_t addr, const ShaderMappedData& data)
{
	EXIT_IF(g_shader_map == nullptr);

	g_shader_map->insert({addr, data});
}

static String8 operand_to_str(ShaderOperand op)
{
	String8 ret = "???";
	switch (op.type)
	{
		case ShaderOperandType::LiteralConstant:
			EXIT_IF(op.size != 0);
			EXIT_IF(op.negate || op.absolute);
			return String8::FromPrintf("%f (%u)", op.constant.f, op.constant.u);
			break;
		case ShaderOperandType::IntegerInlineConstant:
			EXIT_IF(op.size != 0);
			EXIT_IF(op.negate || op.absolute);
			return String8::FromPrintf("%d", op.constant.i);
			break;
		case ShaderOperandType::FloatInlineConstant:
			EXIT_IF(op.size != 0);
			EXIT_IF(op.negate || op.absolute);
			return String8::FromPrintf("%f", op.constant.f);
			break;
		default: break;
	}

	EXIT_IF(op.size != 1);

	switch (op.type)
	{
		case ShaderOperandType::VccHi: ret = "vcc_hi"; break;
		case ShaderOperandType::VccLo: ret = "vcc_lo"; break;
		case ShaderOperandType::ExecHi: ret = "exec_hi"; break;
		case ShaderOperandType::ExecLo: ret = "exec_lo"; break;
		case ShaderOperandType::ExecZ: ret = "execz"; break;
		case ShaderOperandType::Scc: ret = "scc"; break;
		case ShaderOperandType::M0: ret = "m0"; break;
		case ShaderOperandType::Vgpr: ret = String8::FromPrintf("v%d", op.register_id); break;
		case ShaderOperandType::Sgpr: ret = String8::FromPrintf("s%d", op.register_id); break;
		case ShaderOperandType::Null: ret = "null"; break;
		default: break;
	}

	if (op.absolute)
	{
		ret = "abs(" + ret + ")";
	}

	if (op.negate)
	{
		return "-" + ret;
	}

	return ret;
}

static String8 operand_array_to_str(ShaderOperand op, int n)
{
	String8 ret = "???";

	EXIT_IF(op.size != n);

	switch (op.type)
	{
		case ShaderOperandType::VccLo:
			if (n == 2)
			{
				ret = "vcc";
			}
			break;
		case ShaderOperandType::ExecLo:
			if (n == 2)
			{
				ret = "exec";
			}
			break;
		case ShaderOperandType::Sgpr: ret = String8::FromPrintf("s[%d:%d]", op.register_id, op.register_id + n - 1); break;
		case ShaderOperandType::Vgpr: ret = String8::FromPrintf("v[%d:%d]", op.register_id, op.register_id + n - 1); break;
		case ShaderOperandType::LiteralConstant:
			if (n == 2)
			{
				ret = String8::FromPrintf("%f (%u)", op.constant.f, op.constant.u);
			}
			break;
		case ShaderOperandType::IntegerInlineConstant:
			if (n == 2)
			{
				ret = String8::FromPrintf("%d", op.constant.i);
			}
			break;
		default: break;
	}

	EXIT_IF(ret == "???");

	if (op.absolute)
	{
		ret = "abs(" + ret + ")";
	}

	if (op.negate)
	{
		return "-" + ret;
	}

	return ret;
}

static String8 dbg_fmt_to_str(const ShaderInstruction& inst)
{
	switch (inst.format)
	{
		case ShaderInstructionFormat::Unknown: return "Unknown"; break;
		case ShaderInstructionFormat::Empty: return "Empty"; break;
		case ShaderInstructionFormat::Imm: return "Imm"; break;
		case ShaderInstructionFormat::Mrt0OffOffComprVmDone: return "Mrt0OffOffComprVmDone"; break;
		case ShaderInstructionFormat::Mrt0Vsrc0Vsrc1ComprVmDone: return "Mrt0Vsrc0Vsrc1ComprVmDone"; break;
		case ShaderInstructionFormat::Mrt0Vsrc0Vsrc1Vsrc2Vsrc3VmDone: return "Mrt0Vsrc0Vsrc1Vsrc2Vsrc3VmDone"; break;
		case ShaderInstructionFormat::Param0Vsrc0Vsrc1Vsrc2Vsrc3: return "Param0Vsrc0Vsrc1Vsrc2Vsrc3"; break;
		case ShaderInstructionFormat::Param1Vsrc0Vsrc1Vsrc2Vsrc3: return "Param1Vsrc0Vsrc1Vsrc2Vsrc3"; break;
		case ShaderInstructionFormat::Param2Vsrc0Vsrc1Vsrc2Vsrc3: return "Param2Vsrc0Vsrc1Vsrc2Vsrc3"; break;
		case ShaderInstructionFormat::Param3Vsrc0Vsrc1Vsrc2Vsrc3: return "Param3Vsrc0Vsrc1Vsrc2Vsrc3"; break;
		case ShaderInstructionFormat::Param4Vsrc0Vsrc1Vsrc2Vsrc3: return "Param4Vsrc0Vsrc1Vsrc2Vsrc3"; break;
		case ShaderInstructionFormat::Pos0Vsrc0Vsrc1Vsrc2Vsrc3Done: return "Pos0Vsrc0Vsrc1Vsrc2Vsrc3Done"; break;
		case ShaderInstructionFormat::PrimVsrc0OffOffOffDone: return "PrimVsrc0OffOffOffDone"; break;
		case ShaderInstructionFormat::Saddr: return "Saddr"; break;
		case ShaderInstructionFormat::SdstSbaseSoffset: return "SdstSbaseSoffset"; break;
		case ShaderInstructionFormat::Sdst4SbaseSoffset: return "Sdst4SbaseSoffset"; break;
		case ShaderInstructionFormat::Sdst8SbaseSoffset: return "Sdst8SbaseSoffset"; break;
		case ShaderInstructionFormat::SdstSvSoffset: return "SdstSvSoffset"; break;
		case ShaderInstructionFormat::Sdst2SvSoffset: return "Sdst2SvSoffset"; break;
		case ShaderInstructionFormat::Sdst4SvSoffset: return "Sdst4SvSoffset"; break;
		case ShaderInstructionFormat::Sdst8SvSoffset: return "Sdst8SvSoffset"; break;
		case ShaderInstructionFormat::Sdst16SvSoffset: return "Sdst16SvSoffset"; break;
		case ShaderInstructionFormat::SVdstSVsrc0: return "SVdstSVsrc0"; break;
		case ShaderInstructionFormat::Sdst2Ssrc02: return "Sdst2Ssrc02"; break;
		case ShaderInstructionFormat::Sdst2Ssrc02Ssrc1: return "Sdst2Ssrc02Ssrc1"; break;
		case ShaderInstructionFormat::Sdst2Ssrc02Ssrc12: return "Sdst2Ssrc02Ssrc12"; break;
		case ShaderInstructionFormat::SmaskVsrc0Vsrc1: return "SmaskVsrc0Vsrc1"; break;
		case ShaderInstructionFormat::Ssrc0Ssrc1: return "Ssrc0Ssrc1"; break;
		case ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxen: return "Vdata1VaddrSvSoffsIdxen"; break;
		case ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxenFloat1: return "Vdata1VaddrSvSoffsIdxenFloat1"; break;
		case ShaderInstructionFormat::Vdata2VaddrSvSoffsIdxen: return "Vdata2VaddrSvSoffsIdxen"; break;
		case ShaderInstructionFormat::Vdata3VaddrSvSoffsIdxen: return "Vdata3VaddrSvSoffsIdxen"; break;
		case ShaderInstructionFormat::Vdata4VaddrSvSoffsIdxen: return "Vdata4VaddrSvSoffsIdxen"; break;
		case ShaderInstructionFormat::Vdata4VaddrSvSoffsIdxenFloat4: return "Vdata4VaddrSvSoffsIdxenFloat4"; break;
		case ShaderInstructionFormat::Vdata4Vaddr2SvSoffsOffenIdxenFloat4: return "Vdata4Vaddr2SvSoffsOffenIdxenFloat4"; break;
		case ShaderInstructionFormat::Vdata1Vaddr3StSsDmask1: return "Vdata1Vaddr3StSsDmask1"; break;
		case ShaderInstructionFormat::Vdata1Vaddr3StSsDmask8: return "Vdata1Vaddr3StSsDmask8"; break;
		case ShaderInstructionFormat::Vdata2Vaddr3StSsDmask3: return "Vdata2Vaddr3StSsDmask3"; break;
		case ShaderInstructionFormat::Vdata2Vaddr3StSsDmask5: return "Vdata2Vaddr3StSsDmask5"; break;
		case ShaderInstructionFormat::Vdata2Vaddr3StSsDmask9: return "Vdata2Vaddr3StSsDmask9"; break;
		case ShaderInstructionFormat::Vdata3Vaddr3StSsDmask7: return "Vdata3Vaddr3StSsDmask7"; break;
		case ShaderInstructionFormat::Vdata3Vaddr4StSsDmask7: return "Vdata3Vaddr4StSsDmask7"; break;
		case ShaderInstructionFormat::Vdata4Vaddr3StSsDmaskF: return "Vdata4Vaddr3StSsDmaskF"; break;
		case ShaderInstructionFormat::Vdata4Vaddr3StDmaskF: return "Vdata4Vaddr3StDmaskF"; break;
		case ShaderInstructionFormat::Vdata4Vaddr4StDmaskF: return "Vdata4Vaddr4StDmaskF"; break;
		case ShaderInstructionFormat::SVdstSVsrc0SVsrc1: return "SVdstSVsrc0SVsrc1"; break;
		case ShaderInstructionFormat::VdstVsrc0Vsrc1Smask2: return "VdstVsrc0Vsrc1Smask2"; break;
		case ShaderInstructionFormat::VdstVsrc0Vsrc1Vsrc2: return "VdstVsrc0Vsrc1Vsrc2"; break;
		case ShaderInstructionFormat::VdstVsrcAttrChan: return "VdstVsrcAttrChan"; break;
		case ShaderInstructionFormat::VdstSdst2Vsrc0Vsrc1: return "VdstSdst2Vsrc0Vsrc1"; break;
		case ShaderInstructionFormat::VdstGds: return "VdstGds"; break;
		case ShaderInstructionFormat::Label: return "Label"; break;
		default: return "????"; break;
	}
}

static String8 dbg_fmt_print(const ShaderInstruction& inst)
{
	uint64_t f = inst.format;
	EXIT_IF(f == ShaderInstructionFormat::Unknown);
	String8 str;
	if (f == ShaderInstructionFormat::Empty)
	{
		return str;
	}
	int src_num = 0;
	for (;;)
	{
		String8 s;
		auto    fu = f & 0xffu;
		if (fu == 0)
		{
			break;
		}
		switch (fu)
		{
			case ShaderInstructionFormat::D: s = operand_to_str(inst.dst); break;
			case ShaderInstructionFormat::D2: s = operand_to_str(inst.dst2); break;
			case ShaderInstructionFormat::S0: s = operand_to_str(inst.src[0]); break;
			case ShaderInstructionFormat::S1: s = operand_to_str(inst.src[1]); break;
			case ShaderInstructionFormat::S2: s = operand_to_str(inst.src[2]); break;
			case ShaderInstructionFormat::S3: s = operand_to_str(inst.src[3]); break;
			case ShaderInstructionFormat::DA2: s = operand_array_to_str(inst.dst, 2); break;
			case ShaderInstructionFormat::DA3: s = operand_array_to_str(inst.dst, 3); break;
			case ShaderInstructionFormat::DA4: s = operand_array_to_str(inst.dst, 4); break;
			case ShaderInstructionFormat::DA8: s = operand_array_to_str(inst.dst, 8); break;
			case ShaderInstructionFormat::DA16: s = operand_array_to_str(inst.dst, 16); break;
			case ShaderInstructionFormat::D2A2: s = operand_array_to_str(inst.dst2, 2); break;
			case ShaderInstructionFormat::D2A3: s = operand_array_to_str(inst.dst2, 3); break;
			case ShaderInstructionFormat::D2A4: s = operand_array_to_str(inst.dst2, 4); break;
			case ShaderInstructionFormat::S0A2: s = operand_array_to_str(inst.src[0], 2); break;
			case ShaderInstructionFormat::S0A3: s = operand_array_to_str(inst.src[0], 3); break;
			case ShaderInstructionFormat::S0A4: s = operand_array_to_str(inst.src[0], 4); break;
			case ShaderInstructionFormat::S1A2: s = operand_array_to_str(inst.src[1], 2); break;
			case ShaderInstructionFormat::S1A3: s = operand_array_to_str(inst.src[1], 3); break;
			case ShaderInstructionFormat::S1A4: s = operand_array_to_str(inst.src[1], 4); break;
			case ShaderInstructionFormat::S1A8: s = operand_array_to_str(inst.src[1], 8); break;
			case ShaderInstructionFormat::S2A2: s = operand_array_to_str(inst.src[2], 2); break;
			case ShaderInstructionFormat::S2A3: s = operand_array_to_str(inst.src[2], 3); break;
			case ShaderInstructionFormat::S2A4: s = operand_array_to_str(inst.src[2], 4); break;
			case ShaderInstructionFormat::Attr: s = String8::FromPrintf("attr%u.%u", inst.src[1].constant.u, inst.src[2].constant.u); break;
			case ShaderInstructionFormat::Idxen: s = "idxen"; break;
			case ShaderInstructionFormat::Offen: s = "offen"; break;
			case ShaderInstructionFormat::Float1: s = "format:float1"; break;
			case ShaderInstructionFormat::Float4: s = "format:float4"; break;
			case ShaderInstructionFormat::Pos0: s = "pos0"; break;
			case ShaderInstructionFormat::Done: s = "done"; break;
			case ShaderInstructionFormat::Param0: s = "param0"; break;
			case ShaderInstructionFormat::Param1: s = "param1"; break;
			case ShaderInstructionFormat::Param2: s = "param2"; break;
			case ShaderInstructionFormat::Param3: s = "param3"; break;
			case ShaderInstructionFormat::Param4: s = "param4"; break;
			case ShaderInstructionFormat::Mrt0: s = "mrt_color0"; break;
			case ShaderInstructionFormat::Prim: s = "prim"; break;
			case ShaderInstructionFormat::Off: s = "off"; break;
			case ShaderInstructionFormat::Compr: s = "compr"; break;
			case ShaderInstructionFormat::Vm: s = "vm"; break;
			case ShaderInstructionFormat::L: s = String8::FromPrintf("label_%04" PRIx32, inst.pc + 4 + inst.src[0].constant.i); break;
			case ShaderInstructionFormat::Dmask1: s = "dmask:0x1"; break;
			case ShaderInstructionFormat::Dmask8: s = "dmask:0x8"; break;
			case ShaderInstructionFormat::Dmask3: s = "dmask:0x3"; break;
			case ShaderInstructionFormat::Dmask5: s = "dmask:0x5"; break;
			case ShaderInstructionFormat::Dmask7: s = "dmask:0x7"; break;
			case ShaderInstructionFormat::Dmask9: s = "dmask:0x9"; break;
			case ShaderInstructionFormat::DmaskF: s = "dmask:0xf"; break;
			case ShaderInstructionFormat::Gds: s = "gds"; break;
			default: EXIT("unknown code: %u\n", static_cast<uint32_t>(fu));
		}
		switch (fu)
		{
			case ShaderInstructionFormat::L:
			case ShaderInstructionFormat::S0:
			case ShaderInstructionFormat::S0A2:
			case ShaderInstructionFormat::S0A3:
			case ShaderInstructionFormat::S0A4: src_num = std::max(src_num, 1); break;
			case ShaderInstructionFormat::S1:
			case ShaderInstructionFormat::S1A2:
			case ShaderInstructionFormat::S1A3:
			case ShaderInstructionFormat::S1A4:
			case ShaderInstructionFormat::S1A8: src_num = std::max(src_num, 2); break;
			case ShaderInstructionFormat::S2:
			case ShaderInstructionFormat::S2A2:
			case ShaderInstructionFormat::S2A3:
			case ShaderInstructionFormat::S2A4:
			case ShaderInstructionFormat::Attr: src_num = std::max(src_num, 3); break;
			case ShaderInstructionFormat::S3: src_num = std::max(src_num, 4); break;
			default: break;
		}
		str = s + (str.IsEmpty() ? "" : ", " + str);
		f >>= 8u;
	}
	EXIT_IF(src_num != inst.src_num);
	if (inst.dst.multiplier == 2.0f)
	{
		str += " mul:2";
	}
	if (inst.dst.multiplier == 4.0f)
	{
		str += " mul:4";
	}
	if (inst.dst.multiplier == 0.5f)
	{
		str += " div:2";
	}
	if (inst.dst.clamp)
	{
		str += " clamp";
	}
	return str;
}

String8 ShaderCode::DbgInstructionToStr(const ShaderInstruction& inst)
{
	String8 ret;

	String8 name   = Core::EnumName8(inst.type);
	String8 format = dbg_fmt_to_str(inst);

	ret += String8::FromPrintf("%-20s [%-30s] ", name.c_str(), format.c_str());
	ret += dbg_fmt_print(inst);

	return ret;
}

String8 ShaderCode::DbgDump() const
{
	String8 ret;
	for (const auto& inst: m_instructions)
	{
		if (m_labels.Contains(inst.pc, [](auto label, auto pc) { return (!label.IsDisabled() && label.GetDst() == pc); }))
		{
			ret += String8::FromPrintf("\nlabel_%04" PRIx32 ":\n", inst.pc);
		}
		if (m_indirect_labels.Contains(inst.pc, [](auto label, auto pc) { return (!label.IsDisabled() && label.GetDst() == pc); }))
		{
			ret += "\n";
		}
		ret += String8::FromPrintf("  %s\n", DbgInstructionToStr(inst).c_str());
	}
	return ret;
}

static bool IsDiscardInstruction(const Vector<ShaderInstruction>& code, uint32_t index)
{
	if (!(index == 0 || index + 1 >= code.Size()))
	{
		const auto& prev_inst = code.At(index - 1);
		const auto& inst      = code.At(index);
		const auto& next_inst = code.At(index + 1);

		return (inst.type == ShaderInstructionType::Exp && inst.format == ShaderInstructionFormat::Mrt0OffOffComprVmDone &&
		        prev_inst.type == ShaderInstructionType::SMovB64 && prev_inst.format == ShaderInstructionFormat::Sdst2Ssrc02 &&
		        prev_inst.dst.type == ShaderOperandType::ExecLo && prev_inst.src[0].type == ShaderOperandType::IntegerInlineConstant &&
		        prev_inst.src[0].constant.i == 0 && next_inst.type == ShaderInstructionType::SEndpgm);
	}
	return false;
}

// bool ShaderCode::IsDiscardBlock(uint32_t pc) const
//{
//	auto inst_count = m_instructions.Size();
//	for (uint32_t index = 0; index < inst_count; index++)
//	{
//		const auto& inst = m_instructions.At(index);
//		if (inst.pc == pc)
//		{
//			for (uint32_t i = index; i < inst_count; i++)
//			{
//				const auto& inst = m_instructions.At(i);
//
//				if (inst.type == ShaderInstructionType::SEndpgm || inst.type == ShaderInstructionType::SCbranchExecz ||
//				    inst.type == ShaderInstructionType::SCbranchScc0 || inst.type == ShaderInstructionType::SCbranchScc1 ||
//				    inst.type == ShaderInstructionType::SCbranchVccz)
//				{
//					return false;
//				}
//
//				if (IsDiscardInstruction(i))
//				{
//					return true;
//				}
//			}
//			return false;
//		}
//	}
//	return false;
// }

ShaderControlFlowBlock ShaderCode::ReadBlock(uint32_t pc) const
{
	ShaderControlFlowBlock ret;
	auto                   inst_count = m_instructions.Size();
	for (uint32_t index = 0; index < inst_count; index++)
	{
		const auto& inst = m_instructions.At(index);
		if (inst.pc == pc)
		{
			ret.pc       = pc;
			ret.is_valid = true;
			for (uint32_t i = index; i < inst_count; i++)
			{
				const auto& inst = m_instructions.At(i);

				if (inst.type == ShaderInstructionType::SEndpgm || inst.type == ShaderInstructionType::SCbranchExecz ||
				    inst.type == ShaderInstructionType::SCbranchScc0 || inst.type == ShaderInstructionType::SCbranchScc1 ||
				    inst.type == ShaderInstructionType::SCbranchVccz || inst.type == ShaderInstructionType::SCbranchVccnz ||
				    inst.type == ShaderInstructionType::SBranch)
				{
					ret.last = inst;
					break;
				}

				if (IsDiscardInstruction(m_instructions, i))
				{
					ret.is_discard = true;
				}
			}
			break;
		}
	}
	return ret;
}

Vector<ShaderInstruction> ShaderCode::ReadIntructions(const ShaderControlFlowBlock& block) const
{
	Vector<ShaderInstruction> ret;

	auto inst_count = m_instructions.Size();
	for (uint32_t index = 0; index < inst_count; index++)
	{
		const auto& inst = m_instructions.At(index);
		if (inst.pc == block.pc)
		{
			for (uint32_t i = index; i < inst_count; i++)
			{
				const auto& inst = m_instructions.At(i);

				ret.Add(inst);

				if (inst.pc == block.last.pc)
				{
					break;
				}
			}
			break;
		}
	}

	return ret;
}

static void vs_print(const char* func, const HW::VertexShaderInfo& vs, const HW::ShaderRegisters& sh)
{
	printf("%s\n", func);

	printf("\t vs.data_addr                 = 0x%016" PRIx64 "\n", vs.vs_regs.data_addr);
	printf("\t es.data_addr                 = 0x%016" PRIx64 "\n", vs.es_regs.data_addr);
	printf("\t gs.data_addr                 = 0x%016" PRIx64 "\n", vs.gs_regs.data_addr);

	if (vs.vs_regs.data_addr != 0)
	{
		printf("\t vs.vgprs                     = 0x%02" PRIx8 "\n", vs.vs_regs.rsrc1.vgprs);
		printf("\t vs.sgprs                     = 0x%02" PRIx8 "\n", vs.vs_regs.rsrc1.sgprs);
		printf("\t vs.priority                  = 0x%02" PRIx8 "\n", vs.vs_regs.rsrc1.priority);
		printf("\t vs.float_mode                = 0x%02" PRIx8 "\n", vs.vs_regs.rsrc1.float_mode);
		printf("\t vs.dx10_clamp                = %s\n", vs.vs_regs.rsrc1.dx10_clamp ? "true" : "false");
		printf("\t vs.ieee_mode                 = %s\n", vs.vs_regs.rsrc1.ieee_mode ? "true" : "false");
		printf("\t vs.vgpr_component_count      = 0x%02" PRIx8 "\n", vs.vs_regs.rsrc1.vgpr_component_count);
		printf("\t vs.cu_group_enable           = %s\n", vs.vs_regs.rsrc1.cu_group_enable ? "true" : "false");
		printf("\t vs.require_forward_progress  = %s\n", vs.vs_regs.rsrc1.require_forward_progress ? "true" : "false");
		printf("\t vs.fp16_overflow             = %s\n", vs.vs_regs.rsrc1.fp16_overflow ? "true" : "false");
		printf("\t vs.scratch_en                = %s\n", vs.vs_regs.rsrc2.scratch_en ? "true" : "false");
		printf("\t vs.user_sgpr                 = 0x%02" PRIx8 "\n", vs.vs_regs.rsrc2.user_sgpr);
		printf("\t vs.offchip_lds               = %s\n", vs.vs_regs.rsrc2.offchip_lds ? "true" : "false");
		printf("\t vs.streamout_enabled         = %s\n", vs.vs_regs.rsrc2.streamout_enabled ? "true" : "false");
		printf("\t vs.shared_vgprs              = 0x%02" PRIx8 "\n", vs.vs_regs.rsrc2.shared_vgprs);
	}

	if (vs.gs_regs.data_addr != 0 || vs.es_regs.data_addr != 0)
	{
		printf("\t chksum                       = 0x%016" PRIx64 "\n", vs.gs_regs.chksum);
		printf("\t gs.vgprs                     = 0x%02" PRIx8 "\n", vs.gs_regs.rsrc1.vgprs);
		printf("\t gs.sgprs                     = 0x%02" PRIx8 "\n", vs.gs_regs.rsrc1.sgprs);
		printf("\t gs.priority                  = 0x%02" PRIx8 "\n", vs.gs_regs.rsrc1.priority);
		printf("\t gs.float_mode                = 0x%02" PRIx8 "\n", vs.gs_regs.rsrc1.float_mode);
		printf("\t gs.dx10_clamp                = %s\n", vs.gs_regs.rsrc1.dx10_clamp ? "true" : "false");
		printf("\t gs.ieee_mode                 = %s\n", vs.gs_regs.rsrc1.ieee_mode ? "true" : "false");
		printf("\t gs.debug_mode                = %s\n", vs.gs_regs.rsrc1.debug_mode ? "true" : "false");
		printf("\t gs.lds_configuration         = %s\n", vs.gs_regs.rsrc1.lds_configuration ? "true" : "false");
		printf("\t gs.cu_group_enable           = %s\n", vs.gs_regs.rsrc1.cu_group_enable ? "true" : "false");
		printf("\t gs.require_forward_progress  = %s\n", vs.gs_regs.rsrc1.require_forward_progress ? "true" : "false");
		printf("\t gs.fp16_overflow             = %s\n", vs.gs_regs.rsrc1.fp16_overflow ? "true" : "false");
		printf("\t gs.gs_vgpr_component_count   = 0x%02" PRIx8 "\n", vs.gs_regs.rsrc1.gs_vgpr_component_count);
		printf("\t gs.scratch_en                = %s\n", vs.gs_regs.rsrc2.scratch_en ? "true" : "false");
		printf("\t gs.user_sgpr                 = 0x%02" PRIx8 "\n", vs.gs_regs.rsrc2.user_sgpr);
		printf("\t gs.offchip_lds               = %s\n", vs.gs_regs.rsrc2.offchip_lds ? "true" : "false");
		printf("\t gs.shared_vgprs              = 0x%02" PRIx8 "\n", vs.gs_regs.rsrc2.shared_vgprs);
		printf("\t gs.es_vgpr_component_count   = 0x%02" PRIx8 "\n", vs.gs_regs.rsrc2.es_vgpr_component_count);
		printf("\t gs.lds_size                  = 0x%02" PRIx8 "\n", vs.gs_regs.rsrc2.lds_size);
	}

	printf("\t m_spiVsOutConfig          = 0x%08" PRIx32 "\n", sh.m_spiVsOutConfig);
	printf("\t m_spiShaderPosFormat      = 0x%08" PRIx32 "\n", sh.m_spiShaderPosFormat);
	printf("\t m_paClVsOutCntl           = 0x%08" PRIx32 "\n", sh.m_paClVsOutCntl);
	printf("\t m_spiShaderIdxFormat      = 0x%08" PRIx32 "\n", sh.m_spiShaderIdxFormat);
	printf("\t m_geNggSubgrpCntl         = 0x%08" PRIx32 "\n", sh.m_geNggSubgrpCntl);
	printf("\t m_vgtGsInstanceCnt        = 0x%08" PRIx32 "\n", sh.m_vgtGsInstanceCnt);
	printf("\t GetEsVertsPerSubgrp()     = 0x%08" PRIx32 "\n", sh.GetEsVertsPerSubgrp());
	printf("\t GetGsPrimsPerSubgrp()     = 0x%08" PRIx32 "\n", sh.GetGsPrimsPerSubgrp());
	printf("\t GetGsInstPrimsInSubgrp()  = 0x%08" PRIx32 "\n", sh.GetGsInstPrimsInSubgrp());
	printf("\t m_geMaxOutputPerSubgroup  = 0x%08" PRIx32 "\n", sh.m_geMaxOutputPerSubgroup);
	printf("\t m_vgtEsgsRingItemsize     = 0x%08" PRIx32 "\n", sh.m_vgtEsgsRingItemsize);
	printf("\t m_vgtGsMaxVertOut         = 0x%08" PRIx32 "\n", sh.m_vgtGsMaxVertOut);
	printf("\t m_vgtGsOutPrimType        = 0x%08" PRIx32 "\n", sh.m_vgtGsOutPrimType);
}

static void ps_print(const char* func, const HW::PsStageRegisters& ps, const HW::ShaderRegisters& sh)
{
	printf("%s\n", func);

	printf("\t data_addr                   = 0x%016" PRIx64 "\n", ps.data_addr);
	printf("\t chksum                      = 0x%016" PRIx64 "\n", ps.chksum);
	printf("\t conservative_z_export_value = 0x%08" PRIx32 "\n", sh.db_shader_control.conservative_z_export_value);
	printf("\t shader_z_behavior           = 0x%08" PRIx32 "\n", sh.db_shader_control.shader_z_behavior);
	printf("\t shader_kill_enable          = %s\n", sh.db_shader_control.shader_kill_enable ? "true" : "false");
	printf("\t shader_z_export_enable      = %s\n", sh.db_shader_control.shader_z_export_enable ? "true" : "false");
	printf("\t shader_execute_on_noop      = %s\n", sh.db_shader_control.shader_execute_on_noop ? "true" : "false");
	printf("\t vgprs                       = 0x%02" PRIx8 "\n", ps.rsrc1.vgprs);
	printf("\t sgprs                       = 0x%02" PRIx8 "\n", ps.rsrc1.sgprs);
	printf("\t priority                    = 0x%02" PRIx8 "\n", ps.rsrc1.priority);
	printf("\t float_mode                  = 0x%02" PRIx8 "\n", ps.rsrc1.float_mode);
	printf("\t dx10_clamp                  = %s\n", ps.rsrc1.dx10_clamp ? "true" : "false");
	printf("\t debug_mode                  = %s\n", ps.rsrc1.debug_mode ? "true" : "false");
	printf("\t ieee_mode                   = %s\n", ps.rsrc1.ieee_mode ? "true" : "false");
	printf("\t cu_group_disable            = %s\n", ps.rsrc1.cu_group_disable ? "true" : "false");
	printf("\t require_forward_progress    = %s\n", ps.rsrc1.require_forward_progress ? "true" : "false");
	printf("\t fp16_overflow               = %s\n", ps.rsrc1.fp16_overflow ? "true" : "false");
	printf("\t scratch_en                  = %s\n", ps.rsrc2.scratch_en ? "true" : "false");
	printf("\t user_sgpr                   = 0x%02" PRIx8 "\n", ps.rsrc2.user_sgpr);
	printf("\t wave_cnt_en                 = %s\n", ps.rsrc2.wave_cnt_en ? "true" : "false");
	printf("\t extra_lds_size              = 0x%02" PRIx8 "\n", ps.rsrc2.extra_lds_size);
	printf("\t raster_ordered_shading      = %s\n", ps.rsrc2.raster_ordered_shading ? "true" : "false");
	printf("\t shared_vgprs                = 0x%02" PRIx8 "\n", ps.rsrc2.shared_vgprs);

	printf("\t shader_z_format             = 0x%08" PRIx32 "\n", sh.shader_z_format);
	printf("\t target_output_mode[0]       = 0x%02" PRIx8 "\n", sh.target_output_mode[0]);
	printf("\t ps_input_ena                = 0x%08" PRIx32 "\n", sh.ps_input_ena);
	printf("\t ps_input_addr               = 0x%08" PRIx32 "\n", sh.ps_input_addr);
	printf("\t ps_in_control               = 0x%08" PRIx32 "\n", sh.ps_in_control);
	printf("\t baryc_cntl                  = 0x%08" PRIx32 "\n", sh.baryc_cntl);
	printf("\t m_cbShaderMask              = 0x%08" PRIx32 "\n", sh.m_cbShaderMask);

	printf("\t m_paScShaderControl         = 0x%08" PRIx32 "\n", sh.m_paScShaderControl);
}

static void cs_print(const char* func, const HW::CsStageRegisters& cs, const HW::ShaderRegisters& /*sh*/)
{
	printf("%s\n", func);

	//	printf("\t GetGpuAddress()        = 0x%016" PRIx64 "\n", cs.GetGpuAddress());
	//	printf("\t m_computePgmLo         = 0x%08" PRIx32 "\n", cs.m_computePgmLo);
	//	printf("\t m_computePgmHi         = 0x%08" PRIx32 "\n", cs.m_computePgmHi);
	//	printf("\t m_computePgmRsrc1      = 0x%08" PRIx32 "\n", cs.m_computePgmRsrc1);
	//	printf("\t m_computePgmRsrc2      = 0x%08" PRIx32 "\n", cs.m_computePgmRsrc2);
	//	printf("\t m_computeNumThreadX    = 0x%08" PRIx32 "\n", cs.m_computeNumThreadX);
	//	printf("\t m_computeNumThreadY    = 0x%08" PRIx32 "\n", cs.m_computeNumThreadY);
	//	printf("\t m_computeNumThreadZ    = 0x%08" PRIx32 "\n", cs.m_computeNumThreadZ);
	printf("\t data_addr      = 0x%016" PRIx64 "\n", cs.data_addr);
	printf("\t num_thread_x   = 0x%08" PRIx32 "\n", cs.num_thread_x);
	printf("\t num_thread_y   = 0x%08" PRIx32 "\n", cs.num_thread_y);
	printf("\t num_thread_z   = 0x%08" PRIx32 "\n", cs.num_thread_z);
	printf("\t vgprs          = 0x%02" PRIx8 "\n", cs.vgprs);
	printf("\t sgprs          = 0x%02" PRIx8 "\n", cs.sgprs);
	printf("\t bulky          = 0x%02" PRIx8 "\n", cs.bulky);
	printf("\t scratch_en     = 0x%02" PRIx8 "\n", cs.scratch_en);
	printf("\t user_sgpr      = 0x%02" PRIx8 "\n", cs.user_sgpr);
	printf("\t tgid_x_en      = 0x%02" PRIx8 "\n", cs.tgid_x_en);
	printf("\t tgid_y_en      = 0x%02" PRIx8 "\n", cs.tgid_y_en);
	printf("\t tgid_z_en      = 0x%02" PRIx8 "\n", cs.tgid_z_en);
	printf("\t tg_size_en     = 0x%02" PRIx8 "\n", cs.tg_size_en);
	printf("\t tidig_comp_cnt = 0x%02" PRIx8 "\n", cs.tidig_comp_cnt);
	printf("\t lds_size       = 0x%02" PRIx8 "\n", cs.lds_size);
}

static void bi_print(const char* func, const ShaderBinaryInfo& bi)
{
	printf("%s\n", func);

	printf("\t signature                  = %.7s\n", bi.signature);
	printf("\t version                    = 0x%02" PRIx8 "\n", bi.version);
	printf("\t pssl_or_cg                 = 0x%08" PRIx32 "\n", static_cast<uint32_t>(bi.pssl_or_cg));
	printf("\t cached                     = 0x%08" PRIx32 "\n", static_cast<uint32_t>(bi.cached));
	printf("\t type                       = 0x%08" PRIx32 "\n", static_cast<uint32_t>(bi.type));
	printf("\t source_type                = 0x%08" PRIx32 "\n", static_cast<uint32_t>(bi.source_type));
	printf("\t length                     = 0x%08" PRIx32 "\n", static_cast<uint32_t>(bi.length));
	printf("\t chunk_usage_base_offset_dw = 0x%02" PRIx8 "\n", bi.chunk_usage_base_offset_dw);
	printf("\t num_input_usage_slots      = 0x%02" PRIx8 "\n", bi.num_input_usage_slots);
	printf("\t is_srt                     = 0x%02" PRIx8 "\n", bi.is_srt);
	printf("\t is_srt_used_info_valid     = 0x%02" PRIx8 "\n", bi.is_srt_used_info_valid);
	printf("\t is_extended_usage_info     = 0x%02" PRIx8 "\n", bi.is_extended_usage_info);
	printf("\t reserved2                  = 0x%02" PRIx8 "\n", bi.reserved2);
	printf("\t reserved3                  = 0x%02" PRIx8 "\n", bi.reserved3);
	printf("\t hash0                      = 0x%08" PRIx32 "\n", bi.hash0);
	printf("\t hash1                      = 0x%08" PRIx32 "\n", bi.hash1);
	printf("\t crc32                      = 0x%08" PRIx32 "\n", bi.crc32);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void vs_check(const HW::VertexShaderInfo& vs, const HW::ShaderRegisters& sh)
{
	if (vs.vs_regs.data_addr != 0)
	{
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc1.priority != 0);
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc1.float_mode != 192);
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc1.dx10_clamp != true);
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc1.ieee_mode != false);
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc1.cu_group_enable != false);
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc1.require_forward_progress != false);
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc1.fp16_overflow != false);
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc2.scratch_en != false);
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc2.offchip_lds != false);
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc2.streamout_enabled != false);
		EXIT_NOT_IMPLEMENTED(vs.vs_regs.rsrc2.shared_vgprs != 0);
	}

	if (vs.es_regs.data_addr != 0 || vs.gs_regs.data_addr != 0)
	{
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc1.priority != 0);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc1.float_mode != 192);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc1.dx10_clamp != true);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc1.debug_mode != false);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc1.ieee_mode != false);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc1.cu_group_enable != false);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc1.require_forward_progress != false);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc1.lds_configuration != false);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc1.gs_vgpr_component_count != 3);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc1.fp16_overflow != false);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc2.scratch_en != false);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc2.offchip_lds != false);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc2.es_vgpr_component_count != 3);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc2.lds_size != 0);
		EXIT_NOT_IMPLEMENTED(vs.gs_regs.rsrc2.shared_vgprs != 0);
	}

	EXIT_NOT_IMPLEMENTED(sh.m_spiShaderPosFormat != 0x00000004);
	EXIT_NOT_IMPLEMENTED(sh.m_paClVsOutCntl != 0x00000000);

	EXIT_NOT_IMPLEMENTED(sh.m_spiShaderIdxFormat != 0x00000000 && sh.m_spiShaderIdxFormat != 0x00000001);
	EXIT_NOT_IMPLEMENTED(sh.m_geNggSubgrpCntl != 0x00000000 && sh.m_geNggSubgrpCntl != 0x00000001);
	EXIT_NOT_IMPLEMENTED(sh.m_vgtGsInstanceCnt != 0x00000000);
	EXIT_NOT_IMPLEMENTED(sh.GetEsVertsPerSubgrp() != 0x00000000 && sh.GetEsVertsPerSubgrp() != 0x00000040);
	EXIT_NOT_IMPLEMENTED(sh.GetGsPrimsPerSubgrp() != 0x00000000 && sh.GetGsPrimsPerSubgrp() != 0x00000040);
	EXIT_NOT_IMPLEMENTED(sh.GetGsInstPrimsInSubgrp() != 0x00000000 && sh.GetGsInstPrimsInSubgrp() != 0x00000040);
	EXIT_NOT_IMPLEMENTED(sh.m_geMaxOutputPerSubgroup != 0x00000000 && sh.m_geMaxOutputPerSubgroup != 0x00000040);
	EXIT_NOT_IMPLEMENTED(sh.m_vgtEsgsRingItemsize != 0x00000000 && sh.m_vgtEsgsRingItemsize != 0x00000004);
	EXIT_NOT_IMPLEMENTED(sh.m_vgtGsMaxVertOut != 0x00000000);
	EXIT_NOT_IMPLEMENTED(sh.m_vgtGsOutPrimType != 0x00000000);
}

static void ps_check(const HW::PsStageRegisters& ps, const HW::ShaderRegisters& sh)
{
	EXIT_NOT_IMPLEMENTED(sh.target_output_mode[0] != 4 && sh.target_output_mode[0] != 9);
	EXIT_NOT_IMPLEMENTED(sh.db_shader_control.conservative_z_export_value != 0x00000000);
	EXIT_NOT_IMPLEMENTED(sh.db_shader_control.shader_z_behavior != 0x00000001 && sh.db_shader_control.shader_z_behavior != 0x00000000);
	// EXIT_NOT_IMPLEMENTED(ps.shader_kill_enable != false);
	EXIT_NOT_IMPLEMENTED(sh.db_shader_control.shader_z_export_enable != false);
	// EXIT_NOT_IMPLEMENTED(ps.shader_execute_on_noop != false);
	// EXIT_NOT_IMPLEMENTED(ps.m_spiShaderPgmRsrc1Ps != 0x002c0000);
	// EXIT_NOT_IMPLEMENTED(ps.m_spiShaderPgmRsrc2Ps != 0x00000000);
	// EXIT_NOT_IMPLEMENTED(ps.vgprs != 0x00 && ps.vgprs != 0x01);
	// EXIT_NOT_IMPLEMENTED(ps.sgprs != 0x00 && ps.sgprs != 0x01);
	EXIT_NOT_IMPLEMENTED(ps.rsrc1.priority != 0);
	EXIT_NOT_IMPLEMENTED(ps.rsrc1.float_mode != 192);
	EXIT_NOT_IMPLEMENTED(ps.rsrc1.dx10_clamp != true);
	EXIT_NOT_IMPLEMENTED(ps.rsrc1.debug_mode != false);
	EXIT_NOT_IMPLEMENTED(ps.rsrc1.ieee_mode != false);
	EXIT_NOT_IMPLEMENTED(ps.rsrc1.cu_group_disable != false);
	EXIT_NOT_IMPLEMENTED(ps.rsrc1.require_forward_progress != false);
	EXIT_NOT_IMPLEMENTED(ps.rsrc1.fp16_overflow != false);
	EXIT_NOT_IMPLEMENTED(ps.rsrc2.scratch_en != false);
	// EXIT_NOT_IMPLEMENTED(ps.user_sgpr != 0 && ps.user_sgpr != 4 && ps.user_sgpr != 12);
	EXIT_NOT_IMPLEMENTED(ps.rsrc2.wave_cnt_en != false);
	EXIT_NOT_IMPLEMENTED(ps.rsrc2.extra_lds_size != 0);
	EXIT_NOT_IMPLEMENTED(ps.rsrc2.raster_ordered_shading != false);
	EXIT_NOT_IMPLEMENTED(ps.rsrc2.shared_vgprs != 0);

	EXIT_NOT_IMPLEMENTED(sh.shader_z_format != 0x00000000);
	EXIT_NOT_IMPLEMENTED(sh.ps_input_ena != 0x00000002 && sh.ps_input_ena != 0x00000302);
	EXIT_NOT_IMPLEMENTED(sh.ps_input_addr != 0x00000002 && sh.ps_input_addr != 0x00000302);
	// EXIT_NOT_IMPLEMENTED(ps.m_spiPsInControl != 0x00000000);
	EXIT_NOT_IMPLEMENTED(sh.baryc_cntl != 0x00000000 && sh.baryc_cntl != 0x01000000);
	EXIT_NOT_IMPLEMENTED(sh.m_cbShaderMask != 0x0000000f);

	EXIT_NOT_IMPLEMENTED(sh.db_shader_control.other_bits != 0x00000000);
	EXIT_NOT_IMPLEMENTED(sh.m_paScShaderControl != 0x00000000);
}

static void cs_check(const HW::CsStageRegisters& cs, const HW::ShaderRegisters& /*sh*/)
{
	// EXIT_NOT_IMPLEMENTED(cs.num_thread_x != 0x00000040);
	// EXIT_NOT_IMPLEMENTED(cs.num_thread_y != 0x00000001);
	// EXIT_NOT_IMPLEMENTED(cs.num_thread_z != 0x00000001);
	// EXIT_NOT_IMPLEMENTED(cs.vgprs != 0x00 && cs.vgprs != 0x01);
	// EXIT_NOT_IMPLEMENTED(cs.sgprs != 0x01 && cs.sgprs != 0x02);
	EXIT_NOT_IMPLEMENTED(cs.bulky != 0x00);
	EXIT_NOT_IMPLEMENTED(cs.scratch_en != 0x00);
	// EXIT_NOT_IMPLEMENTED(cs.user_sgpr != 0x0c);
	EXIT_NOT_IMPLEMENTED(cs.tgid_x_en != 0x01);
	// EXIT_NOT_IMPLEMENTED(cs.tgid_y_en != 0x00);
	// EXIT_NOT_IMPLEMENTED(cs.tgid_z_en != 0x00);
	EXIT_NOT_IMPLEMENTED(cs.tg_size_en != 0x00);
	EXIT_NOT_IMPLEMENTED(cs.tidig_comp_cnt > 2);
	EXIT_NOT_IMPLEMENTED(cs.lds_size != 0x00);

	//	EXIT_NOT_IMPLEMENTED(cs.m_computePgmRsrc1 != 0x002c0040);
	//	EXIT_NOT_IMPLEMENTED(cs.m_computePgmRsrc2 != 0x00000098);
	//	EXIT_NOT_IMPLEMENTED(cs.m_computeNumThreadX != 0x00000040);
	//	EXIT_NOT_IMPLEMENTED(cs.m_computeNumThreadY != 0x00000001);
	//	EXIT_NOT_IMPLEMENTED(cs.m_computeNumThreadZ != 0x00000001);
}

static bool SpirvDisassemble(const uint32_t* src_binary, size_t src_binary_size, String8* dst_disassembly)
{
	if (dst_disassembly != nullptr)
	{
		spvtools::SpirvTools core(SPV_ENV_VULKAN_1_2);

		std::string disassembly;
		if (!core.Disassemble(src_binary, src_binary_size, &disassembly,
		                      static_cast<uint32_t>(SPV_BINARY_TO_TEXT_OPTION_NO_HEADER) |
		                          static_cast<uint32_t>(SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES) |
		                          static_cast<uint32_t>(SPV_BINARY_TO_TEXT_OPTION_COMMENT) |
		                          static_cast<uint32_t>(SPV_BINARY_TO_TEXT_OPTION_INDENT) |
		                          static_cast<uint32_t>(SPV_BINARY_TO_TEXT_OPTION_COLOR)))
		{
			*dst_disassembly = disassembly.c_str();

			printf("Disassemble failed\n");
			return false;
		}

		*dst_disassembly = disassembly.c_str();
	}
	return true;
}

static bool SpirvToGlsl(const uint32_t* /*src_binary*/, size_t /*src_binary_size*/, String8* /*dst_code*/)
{
	//	if (dst_code != nullptr)
	//	{
	//		spirv_cross::CompilerGLSL glsl(src_binary, src_binary_size);
	//
	//		std::string source = glsl.compile();
	//
	//		*dst_code = source.c_str();
	//	}
	return true;
}

static bool SpirvRun(const String8& src, Vector<uint32_t>* dst, String8* err_msg)
{
	EXIT_IF(dst == nullptr);
	EXIT_IF(err_msg == nullptr);

	spvtools::SpirvTools core(SPV_ENV_VULKAN_1_2);
	spvtools::Optimizer  opt(SPV_ENV_VULKAN_1_2);

	spv_position_t error_position {};
	String8        error_msg;

	auto print_msg_to_stderr = [&error_position, &error_msg](spv_message_level_t /* level */, const char* /*source*/,
	                                                         [[maybe_unused]] const spv_position_t& position, const char* m)
	{
		// printf("%s\n", source);
		error_msg = String8::FromPrintf("%d: %d (%d) %s", static_cast<int>(position.line), static_cast<int>(position.column),
		                                static_cast<int>(position.index), m);
		printf(FG_BRIGHT_RED "error: %s\n" FG_DEFAULT, error_msg.c_str());
		error_position = position;
	};
	core.SetMessageConsumer(print_msg_to_stderr);
	opt.SetMessageConsumer(print_msg_to_stderr);

	dst->Clear();

	std::vector<uint32_t> spirv;
	if (!core.Assemble(src.GetDataConst(), src.Size(), &spirv))
	{
		printf("Assemble failed at:\n%s\n", src.Mid(src.FindIndex('\n', error_position.index - 100), 200).c_str());
		*err_msg = String8::FromPrintf("Assemble failed at:\n%s\n", src.Mid(src.FindIndex('\n', error_position.index - 100), 200).c_str());
		return false;
	}

	if (Config::ShaderValidationEnabled() && !core.Validate(spirv))
	{
		String8 disassembly;
		SpirvDisassemble(spirv.data(), spirv.size(), &disassembly);
		printf("%s\n", disassembly.c_str());
		printf("Validate failed\n");
		*err_msg = String8::FromPrintf("%s\n\nValidate failed:\n%s\n", Log::RemoveColors(String::FromUtf8(disassembly.c_str())).C_Str(),
		                               error_msg.c_str());
		return false;
	}

	bool optimize = true;
	switch (Config::GetShaderOptimizationType())
	{
		case Config::ShaderOptimizationType::Performance: opt.RegisterPerformancePasses(); break;
		case Config::ShaderOptimizationType::Size: opt.RegisterSizePasses(); break;
		default: optimize = false; break;
	}

	if (optimize && !opt.Run(spirv.data(), spirv.size(), &spirv))
	{
		printf("Optimize failed\n");
		*err_msg = String8::FromPrintf("Optimize failed\n");
		return false;
	}

	dst->Add(spirv.data(), spirv.size());

	return true;
}

static const ShaderBinaryInfo* GetBinaryInfo(const uint32_t* code)
{
	EXIT_IF(code == nullptr);

	if (code[0] == 0xBEEB03FF)
	{
		return reinterpret_cast<const ShaderBinaryInfo*>(code + static_cast<size_t>(code[1] + 1) * 2);
	}

	return nullptr;
}

static ShaderUsageInfo GetUsageSlots(const uint32_t* code)
{
	EXIT_IF(code == nullptr);

	const auto* binary_info = GetBinaryInfo(code);

	ShaderUsageInfo ret;

	if (binary_info != nullptr)
	{
		EXIT_NOT_IMPLEMENTED(binary_info->chunk_usage_base_offset_dw == 0);

		ret.usage_masks = (binary_info->chunk_usage_base_offset_dw == 0
		                       ? nullptr
		                       : reinterpret_cast<const uint32_t*>(binary_info) - binary_info->chunk_usage_base_offset_dw);
		ret.slots_num   = binary_info->num_input_usage_slots;
		ret.slots       = (ret.slots_num == 0 ? nullptr : reinterpret_cast<const ShaderUsageSlot*>(ret.usage_masks) - ret.slots_num);
		ret.valid       = true;
	}

	return ret;
}

static void ShaderDetectBuffers(ShaderVertexInputInfo* info, bool ps5)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(info == nullptr);

	info->buffers_num = 0;

	for (int ri = 0; ri < info->resources_num; ri++)
	{
		const auto& r = info->resources[ri];

		bool merged = false;
		for (int bi = 0; bi < info->buffers_num; bi++)
		{
			auto& b = info->buffers[bi];

			uint64_t stride = b.stride;

			if (stride == r.Stride())
			{
				uint64_t rbase   = (ps5 ? r.Base48() : r.Base44());
				uint64_t base    = std::min(rbase, b.addr);
				uint64_t offset1 = rbase - base;
				uint64_t offset2 = b.addr - base;

				if (offset1 < stride && offset2 < stride)
				{
					EXIT_NOT_IMPLEMENTED(b.num_records != r.NumRecords());
					b.addr = base;
					EXIT_NOT_IMPLEMENTED(b.attr_num >= ShaderVertexInputBuffer::ATTR_MAX);
					b.attr_indices[b.attr_num++] = ri;
					merged                       = true;
					break;
				}
			}
		}

		if (!merged)
		{
			EXIT_NOT_IMPLEMENTED(info->buffers_num >= ShaderVertexInputInfo::RES_MAX);
			int bi                            = info->buffers_num++;
			info->buffers[bi].addr            = (ps5 ? r.Base48() : r.Base44());
			info->buffers[bi].stride          = r.Stride();
			info->buffers[bi].num_records     = r.NumRecords();
			info->buffers[bi].attr_num        = 1;
			info->buffers[bi].attr_indices[0] = ri;
		}
	}

	for (int bi = 0; bi < info->buffers_num; bi++)
	{
		auto& b = info->buffers[bi];
		for (int ri = 0; ri < b.attr_num; ri++)
		{
			b.attr_offsets[ri] =
			    (ps5 ? info->resources[b.attr_indices[ri]].Base48() : info->resources[b.attr_indices[ri]].Base44()) - b.addr;
		}
	}
}

static void ShaderParseFetch(ShaderVertexInputInfo* info, const uint32_t* fetch, const uint32_t* buffer)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(info == nullptr || fetch == nullptr || buffer == nullptr);

	KYTY_PROFILER_BLOCK("ShaderParseFetch::parse_code");

	ShaderCode code;
	code.SetType(ShaderType::Fetch);
	// shader_parse(0, fetch, nullptr, &code);
	ShaderParse(fetch, &code);

	KYTY_PROFILER_END_BLOCK;

	// printf("%s", code.DbgDump().c_str());

	KYTY_PROFILER_BLOCK("ShaderParseFetch::check_insts");

	const auto& insts = code.GetInstructions();
	uint32_t    size  = insts.Size();
	// int         temp_register = 0;
	uint32_t temp_value[104] = {0};
	int      s_num           = 0;
	int      v_num           = 0;

	for (uint32_t i = 0; i < size; i++)
	{
		const auto& inst = insts.At(i);

		if (inst.type == ShaderInstructionType::SLoadDwordx4)
		{
			EXIT_NOT_IMPLEMENTED(inst.src[1].type != ShaderOperandType::LiteralConstant || (inst.src[1].constant.u & 3u) != 0);
			EXIT_NOT_IMPLEMENTED(inst.src[0].type != ShaderOperandType::Sgpr || inst.src[0].register_id != 2);
			EXIT_NOT_IMPLEMENTED(inst.dst.type != ShaderOperandType::Sgpr);

			uint32_t index    = inst.src[1].constant.u >> 2u;
			int      t        = inst.dst.register_id;
			temp_value[t + 0] = buffer[index + 0];
			temp_value[t + 1] = buffer[index + 1];
			temp_value[t + 2] = buffer[index + 2];
			temp_value[t + 3] = buffer[index + 3];

			s_num++;
		}

		bool load_inst     = true;
		int  registers_num = 0;
		switch (inst.type)
		{
			case ShaderInstructionType::BufferLoadFormatX: registers_num = 1; break;
			case ShaderInstructionType::BufferLoadFormatXy: registers_num = 2; break;
			case ShaderInstructionType::BufferLoadFormatXyz: registers_num = 3; break;
			case ShaderInstructionType::BufferLoadFormatXyzw: registers_num = 4; break;
			default: load_inst = false;
		}

		if (load_inst && registers_num > 0)
		{
			// EXIT_NOT_IMPLEMENTED(!(i >= 2 && insts.At(i - 1).type == ShaderInstructionType::SWaitcnt &&
			//                       insts.At(i - 2).type == ShaderInstructionType::SLoadDwordx4));
			EXIT_NOT_IMPLEMENTED(inst.dst.type != ShaderOperandType::Vgpr);
			EXIT_NOT_IMPLEMENTED(inst.src[0].type != ShaderOperandType::Vgpr || inst.src[0].register_id != 0);
			EXIT_NOT_IMPLEMENTED(inst.src[1].type != ShaderOperandType::Sgpr);
			EXIT_NOT_IMPLEMENTED(inst.src[2].type != ShaderOperandType::IntegerInlineConstant || inst.src[2].constant.i != 0);

			EXIT_NOT_IMPLEMENTED(info->resources_num >= ShaderVertexInputInfo::RES_MAX);

			int t = inst.src[1].register_id;

			auto& r           = info->resources[info->resources_num];
			auto& rd          = info->resources_dst[info->resources_num];
			rd.register_start = inst.dst.register_id;
			rd.registers_num  = registers_num;
			r.fields[0]       = temp_value[t + 0];
			r.fields[1]       = temp_value[t + 1];
			r.fields[2]       = temp_value[t + 2];
			r.fields[3]       = temp_value[t + 3];

			info->resources_num++;

			v_num++;
		}
	}

	KYTY_PROFILER_END_BLOCK;

	EXIT_NOT_IMPLEMENTED(s_num != v_num);
}

static void ShaderParseAttrib(ShaderVertexInputInfo* info, const ShaderSemantic* input_semantics, uint32_t num_input_semantics,
                              const uint32_t* attrib, const uint32_t* buffer)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(info == nullptr || attrib == nullptr || buffer == nullptr);

	for (uint32_t i = 0; i < num_input_semantics; i++)
	{
		const auto& in = input_semantics[i];

		EXIT_NOT_IMPLEMENTED(in.static_vb_index == 1 || in.static_attribute == 1);

		uint32_t reg  = in.hardware_mapping;
		uint32_t size = in.size_in_elements;

		printf("reg = %u, size = %u, va[%u] = 0x%08" PRIx32 "\n", reg, size, i, attrib[in.semantic]);

		size_t   index       = attrib[in.semantic] & 0x1fu;
		uint32_t format      = (attrib[in.semantic] >> 5u) & 0x1ffu;
		uint32_t offset      = (attrib[in.semantic] >> 14u) & 0xfffu;
		uint32_t fetch_index = (attrib[in.semantic] >> 26u) & 0x1u;

		EXIT_NOT_IMPLEMENTED(format != 0);
		EXIT_NOT_IMPLEMENTED(offset != 0);
		EXIT_NOT_IMPLEMENTED(fetch_index != 0);

		EXIT_NOT_IMPLEMENTED(index >= ShaderVertexInputInfo::RES_MAX);

		const auto* sharp = &buffer[index * 4];

		EXIT_NOT_IMPLEMENTED(info->resources_num >= ShaderVertexInputInfo::RES_MAX);

		auto& r           = info->resources[info->resources_num];
		auto& rd          = info->resources_dst[info->resources_num];
		rd.register_start = static_cast<int>(reg);
		rd.registers_num  = static_cast<int>(size);
		r.fields[0]       = sharp[0];
		r.fields[1]       = sharp[1];
		r.fields[2]       = sharp[2];
		r.fields[3]       = sharp[3];

		info->resources_num++;
	}
}

static void ShaderGetStorageBuffer(ShaderStorageResources* info, bool* direct_sgprs, int start_index, int slot, ShaderStorageUsage usage,
                                   const HW::UserSgprInfo& user_sgpr, const uint32_t* extended_buffer)
{
	EXIT_IF(info == nullptr);

	EXIT_NOT_IMPLEMENTED(info->buffers_num < 0 || info->buffers_num >= ShaderStorageResources::BUFFERS_MAX);

	int  index    = info->buffers_num;
	bool extended = (extended_buffer != nullptr);

	EXIT_NOT_IMPLEMENTED(!extended && start_index >= 16);
	EXIT_NOT_IMPLEMENTED(extended && !(start_index >= 16));

	info->start_register[index] = start_index;
	info->slots[index]          = slot;
	info->usages[index]         = usage;
	info->extended[index]       = extended;
	// info->extended_index[index] = extended_index;

	if (!extended)
	{
		for (int j = 0; j < 4; j++)
		{
			auto type = user_sgpr.type[start_index + j];
			EXIT_NOT_IMPLEMENTED(type != HW::UserSgprType::Vsharp && type != HW::UserSgprType::Region);

			direct_sgprs[start_index + j] = false;
		}
	}

	info->buffers[index].fields[0] = (extended ? extended_buffer[start_index - 16 + 0] : user_sgpr.value[start_index + 0]);
	info->buffers[index].fields[1] = (extended ? extended_buffer[start_index - 16 + 1] : user_sgpr.value[start_index + 1]);
	info->buffers[index].fields[2] = (extended ? extended_buffer[start_index - 16 + 2] : user_sgpr.value[start_index + 2]);
	info->buffers[index].fields[3] = (extended ? extended_buffer[start_index - 16 + 3] : user_sgpr.value[start_index + 3]);

	info->buffers_num++;
}

static void ShaderGetTextureBuffer(ShaderTextureResources* info, bool* direct_sgprs, int start_index, int slot, ShaderTextureUsage usage,
                                   const HW::UserSgprInfo& user_sgpr, const uint32_t* extended_buffer)
{
	EXIT_IF(info == nullptr);

	EXIT_NOT_IMPLEMENTED(info->textures_num < 0 || info->textures_num >= ShaderTextureResources::RES_MAX);
	// EXIT_NOT_IMPLEMENTED(info->textures_num != slot);

	int  index    = info->textures_num;
	bool extended = (extended_buffer != nullptr);

	EXIT_NOT_IMPLEMENTED(!extended && start_index >= 16);
	EXIT_NOT_IMPLEMENTED(extended && !(start_index >= 16));

	info->desc[index].start_register = start_index;
	info->desc[index].extended       = extended;
	info->desc[index].slot           = slot;
	info->desc[index].usage          = usage;

	EXIT_IF(usage == ShaderTextureUsage::Unknown);

	if (usage == ShaderTextureUsage::ReadWrite)
	{
		info->textures2d_storage_num++;
		info->desc[index].textures2d_without_sampler = true;
	} else
	{
		info->textures2d_sampled_num++;
		info->desc[index].textures2d_without_sampler = false;
	}

	if (!extended)
	{
		for (int j = 0; j < 8; j++)
		{
			auto type = user_sgpr.type[start_index + j];
			EXIT_NOT_IMPLEMENTED(type != HW::UserSgprType::Vsharp && type != HW::UserSgprType::Region);

			direct_sgprs[start_index + j] = false;
		}
	}

	info->desc[index].texture.fields[0] = (extended ? extended_buffer[start_index - 16 + 0] : user_sgpr.value[start_index + 0]);
	info->desc[index].texture.fields[1] = (extended ? extended_buffer[start_index - 16 + 1] : user_sgpr.value[start_index + 1]);
	info->desc[index].texture.fields[2] = (extended ? extended_buffer[start_index - 16 + 2] : user_sgpr.value[start_index + 2]);
	info->desc[index].texture.fields[3] = (extended ? extended_buffer[start_index - 16 + 3] : user_sgpr.value[start_index + 3]);
	info->desc[index].texture.fields[4] = (extended ? extended_buffer[start_index - 16 + 4] : user_sgpr.value[start_index + 4]);
	info->desc[index].texture.fields[5] = (extended ? extended_buffer[start_index - 16 + 5] : user_sgpr.value[start_index + 5]);
	info->desc[index].texture.fields[6] = (extended ? extended_buffer[start_index - 16 + 6] : user_sgpr.value[start_index + 6]);
	info->desc[index].texture.fields[7] = (extended ? extended_buffer[start_index - 16 + 7] : user_sgpr.value[start_index + 7]);

	info->textures_num++;
}

static void ShaderGetSampler(ShaderSamplerResources* info, bool* direct_sgprs, int start_index, int slot, const HW::UserSgprInfo& user_sgpr,
                             const uint32_t* extended_buffer)
{
	EXIT_IF(info == nullptr);

	EXIT_NOT_IMPLEMENTED(info->samplers_num < 0 || info->samplers_num >= ShaderSamplerResources::RES_MAX);
	// EXIT_NOT_IMPLEMENTED(info->samplers_num != slot);

	int  index    = info->samplers_num;
	bool extended = (extended_buffer != nullptr);

	EXIT_NOT_IMPLEMENTED(!extended && start_index >= 16);
	EXIT_NOT_IMPLEMENTED(extended && !(start_index >= 16));

	info->start_register[index] = start_index;
	info->extended[index]       = extended;
	info->slots[index]          = slot;

	if (!extended)
	{
		for (int j = 0; j < 4; j++)
		{
			auto type = user_sgpr.type[start_index + j];
			EXIT_NOT_IMPLEMENTED(type != HW::UserSgprType::Vsharp && type != HW::UserSgprType::Region);

			direct_sgprs[start_index + j] = false;
		}
	}

	info->samplers[index].fields[0] = (extended ? extended_buffer[start_index - 16 + 0] : user_sgpr.value[start_index + 0]);
	info->samplers[index].fields[1] = (extended ? extended_buffer[start_index - 16 + 1] : user_sgpr.value[start_index + 1]);
	info->samplers[index].fields[2] = (extended ? extended_buffer[start_index - 16 + 2] : user_sgpr.value[start_index + 2]);
	info->samplers[index].fields[3] = (extended ? extended_buffer[start_index - 16 + 3] : user_sgpr.value[start_index + 3]);

	info->samplers_num++;
}

static void ShaderGetGdsPointer(ShaderGdsResources* info, bool* direct_sgprs, int start_index, int slot, const HW::UserSgprInfo& user_sgpr,
                                const uint32_t* extended_buffer)
{
	EXIT_IF(info == nullptr);

	EXIT_NOT_IMPLEMENTED(info->pointers_num < 0 || info->pointers_num >= ShaderGdsResources::POINTERS_MAX);
	// EXIT_NOT_IMPLEMENTED(info->pointers_num != slot);

	int  index    = info->pointers_num;
	bool extended = (extended_buffer != nullptr);

	EXIT_NOT_IMPLEMENTED(!extended && start_index >= 16);
	EXIT_NOT_IMPLEMENTED(extended && !(start_index >= 16));

	info->start_register[index] = start_index;
	info->extended[index]       = extended;
	info->slots[index]          = slot;

	if (!extended)
	{
		auto type = user_sgpr.type[start_index];
		EXIT_NOT_IMPLEMENTED(type != HW::UserSgprType::Unknown);

		direct_sgprs[start_index] = false;
	}

	info->pointers[index].field = (extended ? extended_buffer[start_index - 16] : user_sgpr.value[start_index]);

	info->pointers_num++;
}

static void ShaderGetDirectSgpr(ShaderDirectSgprsResources* info, int start_index, const HW::UserSgprInfo& user_sgpr)
{
	EXIT_IF(info == nullptr);

	EXIT_NOT_IMPLEMENTED(info->sgprs_num < 0 || info->sgprs_num >= ShaderDirectSgprsResources::SGPRS_MAX);

	int index = info->sgprs_num;

	EXIT_NOT_IMPLEMENTED(start_index >= 16);

	info->start_register[index] = start_index;

	auto type = user_sgpr.type[start_index];
	EXIT_NOT_IMPLEMENTED(type != HW::UserSgprType::Unknown);

	info->sgprs[index].field = user_sgpr.value[start_index];

	info->sgprs_num++;
}

void ShaderCalcBindingIndices(ShaderBindResources* bind)
{
	KYTY_PROFILER_FUNCTION();

	int binding_index = 0;

	bind->push_constant_size = 0;

	if (bind->storage_buffers.buffers_num > 0)
	{
		bind->storage_buffers.binding_index = binding_index++;
		bind->push_constant_size += bind->storage_buffers.buffers_num * 16;
	}

	if (bind->textures2D.textures_num > 0)
	{
		bind->textures2D.binding_sampled_index = binding_index++;
		bind->textures2D.binding_storage_index = binding_index++;

		bind->push_constant_size += bind->textures2D.textures_num * 32;
	}

	if (bind->samplers.samplers_num > 0)
	{
		bind->samplers.binding_index = binding_index++;
		bind->push_constant_size += bind->samplers.samplers_num * 16;
	}

	if (bind->gds_pointers.pointers_num > 0)
	{
		bind->gds_pointers.binding_index = binding_index++;
		bind->push_constant_size += (((bind->gds_pointers.pointers_num - 1) / 4) + 1) * 16;
	}

	if (bind->direct_sgprs.sgprs_num > 0)
	{
		bind->push_constant_size += (((bind->direct_sgprs.sgprs_num - 1) / 4) + 1) * 16;
	}

	EXIT_IF((bind->push_constant_size % 16) != 0);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void ShaderParseUsage(uint64_t addr, ShaderParsedUsage* info, ShaderBindResources* bind, const HW::UserSgprInfo& user_sgpr,
                      int user_sgpr_num)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(bind == nullptr);
	EXIT_IF(info == nullptr);

	const auto* src = reinterpret_cast<const uint32_t*>(addr);

	auto usages = GetUsageSlots(src);

	EXIT_NOT_IMPLEMENTED(!usages.valid);

	info->fetch                     = false;
	info->fetch_reg                 = 0;
	info->vertex_buffer             = false;
	info->vertex_buffer_reg         = 0;
	info->storage_buffers_readonly  = 0;
	info->storage_buffers_constant  = 0;
	info->storage_buffers_readwrite = 0;
	info->textures2D_readonly       = 0;
	info->textures2D_readwrite      = 0;
	info->extended_buffer           = false;
	info->samplers                  = 0;
	info->gds_pointers              = 0;
	info->direct_sgprs              = 0;

	uint32_t* extended_buffer = nullptr;

	bool direct_sgprs[HW::UserSgprInfo::SGPRS_MAX];
	for (int i = 0; i < HW::UserSgprInfo::SGPRS_MAX; i++)
	{
		direct_sgprs[i] = (i < user_sgpr_num);
	}

	for (int i = 0; i < usages.slots_num; i++)
	{
		const auto& usage = usages.slots[i];
		switch (usage.type)
		{
			case 0x00:
				EXIT_NOT_IMPLEMENTED(usage.flags != 0 && usage.flags != 3);
				if (usage.flags == 0)
				{
					ShaderGetStorageBuffer(&bind->storage_buffers, direct_sgprs, usage.start_register, usage.slot,
					                       ShaderStorageUsage::ReadOnly, user_sgpr, extended_buffer);
					info->storage_buffers_readonly++;
				} else if (usage.flags == 3)
				{
					ShaderGetTextureBuffer(&bind->textures2D, direct_sgprs, usage.start_register, usage.slot, ShaderTextureUsage::ReadOnly,
					                       user_sgpr, extended_buffer);
					info->textures2D_readonly++;
					EXIT_NOT_IMPLEMENTED(bind->textures2D.desc[bind->textures2D.textures_num - 1].texture.Type() != 9);
				}
				break;

			case 0x01:
				EXIT_NOT_IMPLEMENTED(usage.flags != 0);
				ShaderGetSampler(&bind->samplers, direct_sgprs, usage.start_register, usage.slot, user_sgpr, extended_buffer);
				info->samplers++;
				break;

			case 0x02:
				EXIT_NOT_IMPLEMENTED(usage.flags != 0);
				ShaderGetStorageBuffer(&bind->storage_buffers, direct_sgprs, usage.start_register, usage.slot, ShaderStorageUsage::Constant,
				                       user_sgpr, extended_buffer);
				info->storage_buffers_constant++;
				break;

			case 0x04:
				EXIT_NOT_IMPLEMENTED(usage.flags != 0 && usage.flags != 3);
				if (usage.flags == 0)
				{
					ShaderGetStorageBuffer(&bind->storage_buffers, direct_sgprs, usage.start_register, usage.slot,
					                       ShaderStorageUsage::ReadWrite, user_sgpr, extended_buffer);
					info->storage_buffers_readwrite++;
				} else if (usage.flags == 3)
				{
					ShaderGetTextureBuffer(&bind->textures2D, direct_sgprs, usage.start_register, usage.slot, ShaderTextureUsage::ReadWrite,
					                       user_sgpr, extended_buffer);
					info->textures2D_readwrite++;
					EXIT_NOT_IMPLEMENTED(bind->textures2D.desc[bind->textures2D.textures_num - 1].texture.Type() != 9);
				}
				break;

			case 0x07:
				EXIT_NOT_IMPLEMENTED(usage.flags != 0);
				ShaderGetGdsPointer(&bind->gds_pointers, direct_sgprs, usage.start_register, usage.slot, user_sgpr, extended_buffer);
				info->gds_pointers++;
				break;

			case 0x12:
				EXIT_NOT_IMPLEMENTED(usage.slot != 0);
				EXIT_NOT_IMPLEMENTED(usage.flags != 0);
				info->fetch                            = true;
				info->fetch_reg                        = usage.start_register;
				direct_sgprs[usage.start_register]     = false;
				direct_sgprs[usage.start_register + 1] = false;
				break;

			case 0x17:
				EXIT_NOT_IMPLEMENTED(usage.slot != 0);
				EXIT_NOT_IMPLEMENTED(usage.flags != 0);
				info->vertex_buffer                    = true;
				info->vertex_buffer_reg                = usage.start_register;
				direct_sgprs[usage.start_register]     = false;
				direct_sgprs[usage.start_register + 1] = false;
				break;

			case 0x1b:
				EXIT_NOT_IMPLEMENTED(usage.flags != 0);
				EXIT_NOT_IMPLEMENTED(usage.slot != 1);
				EXIT_NOT_IMPLEMENTED(bind->extended.used);
				EXIT_NOT_IMPLEMENTED(usage.start_register + 1 >= HW::UserSgprInfo::SGPRS_MAX);
				bind->extended.used                    = true;
				bind->extended.slot                    = usage.slot;
				bind->extended.start_register          = usage.start_register;
				bind->extended.data.fields[0]          = user_sgpr.value[usage.start_register];
				bind->extended.data.fields[1]          = user_sgpr.value[usage.start_register + 1];
				extended_buffer                        = reinterpret_cast<uint32_t*>(bind->extended.data.Base());
				info->extended_buffer                  = true;
				direct_sgprs[usage.start_register]     = false;
				direct_sgprs[usage.start_register + 1] = false;
				break;

			default: EXIT("unknown usage type: 0x%02" PRIx8 "\n", usage.type);
		}
	}

	for (int i = 0; i < HW::UserSgprInfo::SGPRS_MAX; i++)
	{
		if (direct_sgprs[i])
		{
			ShaderGetDirectSgpr(&bind->direct_sgprs, i, user_sgpr);
			info->direct_sgprs++;
		}
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void ShaderParseUsage2(const ShaderUserData* user_data, ShaderParsedUsage* info, ShaderBindResources* bind,
                       const HW::UserSgprInfo& user_sgpr, int user_sgpr_num)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(bind == nullptr);
	EXIT_IF(info == nullptr);

	info->fetch                     = false;
	info->fetch_reg                 = 0;
	info->vertex_buffer             = false;
	info->vertex_buffer_reg         = 0;
	info->storage_buffers_readonly  = 0;
	info->storage_buffers_constant  = 0;
	info->storage_buffers_readwrite = 0;
	info->textures2D_readonly       = 0;
	info->textures2D_readwrite      = 0;
	info->extended_buffer           = false;
	info->samplers                  = 0;
	info->gds_pointers              = 0;
	info->direct_sgprs              = 0;

	EXIT_NOT_IMPLEMENTED(user_data == nullptr);
	EXIT_NOT_IMPLEMENTED(user_data->eud_size_dw != 0);
	EXIT_NOT_IMPLEMENTED(user_data->srt_size_dw != 0);

	uint32_t* extended_buffer = nullptr;

	bool direct_sgprs[HW::UserSgprInfo::SGPRS_MAX];
	for (int i = 0; i < HW::UserSgprInfo::SGPRS_MAX; i++)
	{
		direct_sgprs[i] = (i < user_sgpr_num);
	}

	for (uint16_t type = 0; type < user_data->direct_resource_count; type++)
	{
		if (user_data->direct_resource_offset[type] == 0xffff)
		{
			continue;
		}

		int reg = user_data->direct_resource_offset[type];

		switch (type)
		{
			case 8:
				info->vertex_buffer                       = true;
				info->vertex_buffer_reg                   = reg;
				direct_sgprs[info->vertex_buffer_reg]     = false;
				direct_sgprs[info->vertex_buffer_reg + 1] = false;
				break;

			case 10:
				info->vertex_attrib                       = true;
				info->vertex_attrib_reg                   = reg;
				direct_sgprs[info->vertex_attrib_reg]     = false;
				direct_sgprs[info->vertex_attrib_reg + 1] = false;
				break;

			default: EXIT("unknown usage type: 0x%04" PRIx16 "\n", type);
		}
	}

	if (user_data->sharp_resource_count[0] != 0)
	{
		for (uint16_t slot = 0; slot < user_data->sharp_resource_count[0]; slot++)
		{
			if (user_data->sharp_resource_offset[0][slot].offset_dw == 0x7fff)
			{
				continue;
			}

			EXIT_NOT_IMPLEMENTED(user_data->sharp_resource_offset[0][slot].size != 0);
			ShaderGetTextureBuffer(&bind->textures2D, direct_sgprs, user_data->sharp_resource_offset[0][slot].offset_dw, slot,
			                       ShaderTextureUsage::ReadOnly, user_sgpr, extended_buffer);
			info->textures2D_readonly++;
			EXIT_NOT_IMPLEMENTED(bind->textures2D.desc[bind->textures2D.textures_num - 1].texture.Type() != 9);
		}
	}

	EXIT_NOT_IMPLEMENTED(user_data->sharp_resource_count[1] != 0);

	if (user_data->sharp_resource_count[2] != 0)
	{
		for (uint16_t slot = 0; slot < user_data->sharp_resource_count[2]; slot++)
		{
			if (user_data->sharp_resource_offset[2][slot].offset_dw == 0x7fff)
			{
				continue;
			}

			EXIT_NOT_IMPLEMENTED(user_data->sharp_resource_offset[2][slot].size != 1);
			ShaderGetSampler(&bind->samplers, direct_sgprs, user_data->sharp_resource_offset[2][slot].offset_dw, slot, user_sgpr,
			                 extended_buffer);
			info->samplers++;
		}
	}

	if (user_data->sharp_resource_count[3] != 0)
	{
		for (uint16_t slot = 0; slot < user_data->sharp_resource_count[3]; slot++)
		{
			if (user_data->sharp_resource_offset[3][slot].offset_dw == 0x7fff)
			{
				continue;
			}

			EXIT_NOT_IMPLEMENTED(user_data->sharp_resource_offset[3][slot].size != 1);
			ShaderGetStorageBuffer(&bind->storage_buffers, direct_sgprs, user_data->sharp_resource_offset[3][slot].offset_dw, slot,
			                       ShaderStorageUsage::Constant, user_sgpr, extended_buffer);
			info->storage_buffers_constant++;
		}
	}

	for (int i = 0; i < HW::UserSgprInfo::SGPRS_MAX; i++)
	{
		if (direct_sgprs[i])
		{
			ShaderGetDirectSgpr(&bind->direct_sgprs, i, user_sgpr);
			info->direct_sgprs++;
		}
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void ShaderGetInputInfoVS(const HW::VertexShaderInfo* regs, const HW::ShaderRegisters* sh, ShaderVertexInputInfo* info)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(info == nullptr || regs == nullptr);

	info->export_count              = static_cast<int>(sh->GetExportCount());
	info->bind.push_constant_offset = 0;
	info->bind.push_constant_size   = 0;
	info->bind.descriptor_set_slot  = 0;

	if (regs->vs_embedded)
	{
		return;
	}

	ShaderParsedUsage usage;

	bool gs_instead_of_vs =
	    (regs->vs_regs.data_addr == 0 && regs->gs_regs.data_addr == 0 && regs->es_regs.data_addr != 0 && regs->gs_regs.chksum != 0);

	uint64_t                shader_addr   = (gs_instead_of_vs ? regs->es_regs.data_addr : regs->vs_regs.data_addr);
	const HW::UserSgprInfo& user_sgpr     = (gs_instead_of_vs ? regs->gs_user_sgpr : regs->vs_user_sgpr);
	auto                    user_sgpr_num = (gs_instead_of_vs ? regs->gs_regs.rsrc2.user_sgpr : regs->vs_regs.rsrc2.user_sgpr);

	bool ps5 = Config::IsNextGen();

	ShaderMappedData data;

	if (ps5)
	{
		if (auto iter = g_shader_map->find(shader_addr); iter != g_shader_map->end())
		{
			data = iter->second;
		}
	}

	if (ps5)
	{
		EXIT_NOT_IMPLEMENTED(data.user_data == nullptr);
		EXIT_NOT_IMPLEMENTED(!gs_instead_of_vs);

		info->gs_prolog = true;

		ShaderParseUsage2(data.user_data, &usage, &info->bind, user_sgpr, static_cast<int>(user_sgpr_num));
	} else
	{
		EXIT_NOT_IMPLEMENTED(gs_instead_of_vs);

		info->gs_prolog = false;

		ShaderParseUsage(shader_addr, &usage, &info->bind, user_sgpr, user_sgpr_num);
	}

	EXIT_NOT_IMPLEMENTED(usage.extended_buffer);
	EXIT_NOT_IMPLEMENTED(usage.samplers > 0);
	EXIT_NOT_IMPLEMENTED(usage.gds_pointers > 0);
	EXIT_NOT_IMPLEMENTED(usage.storage_buffers_readonly > 0 || usage.textures2D_readonly > 0);
	EXIT_NOT_IMPLEMENTED(usage.storage_buffers_readwrite > 0 || usage.textures2D_readwrite > 0);
	EXIT_NOT_IMPLEMENTED(!ps5 && ((usage.fetch && !usage.vertex_buffer) || (!usage.fetch && usage.vertex_buffer)));
	EXIT_NOT_IMPLEMENTED(ps5 && ((usage.vertex_attrib && !usage.vertex_buffer) || (!usage.vertex_attrib && usage.vertex_buffer)));

	if (usage.vertex_buffer && usage.vertex_attrib)
	{
		info->fetch_external   = false;
		info->fetch_embedded   = true;
		info->fetch_inline     = false;
		info->fetch_attrib_reg = usage.vertex_attrib_reg;
		info->fetch_buffer_reg = usage.vertex_buffer_reg;

		EXIT_NOT_IMPLEMENTED(usage.vertex_attrib_reg + 1 >= HW::UserSgprInfo::SGPRS_MAX);
		EXIT_NOT_IMPLEMENTED(usage.vertex_buffer_reg + 1 >= HW::UserSgprInfo::SGPRS_MAX);

		const auto* attrib =
		    reinterpret_cast<const uint32_t*>(static_cast<uint64_t>(user_sgpr.value[usage.vertex_attrib_reg]) |
		                                      (static_cast<uint64_t>(user_sgpr.value[usage.vertex_attrib_reg + 1]) << 32u));
		const auto* buffer =
		    reinterpret_cast<const uint32_t*>(static_cast<uint64_t>(user_sgpr.value[usage.vertex_buffer_reg]) |
		                                      (static_cast<uint64_t>(user_sgpr.value[usage.vertex_buffer_reg + 1]) << 32u));

		EXIT_NOT_IMPLEMENTED(attrib == nullptr || buffer == nullptr);

		EXIT_NOT_IMPLEMENTED(data.input_semantics == nullptr || data.num_input_semantics == 0);

		ShaderParseAttrib(info, data.input_semantics, data.num_input_semantics, attrib, buffer);
		ShaderDetectBuffers(info, ps5);
	}

	if (usage.fetch && usage.vertex_buffer)
	{
		info->fetch_external   = true;
		info->fetch_embedded   = false;
		info->fetch_inline     = false;
		info->fetch_shader_reg = usage.fetch_reg;
		info->fetch_buffer_reg = usage.vertex_buffer_reg;

		EXIT_NOT_IMPLEMENTED(usage.fetch_reg + 1 >= HW::UserSgprInfo::SGPRS_MAX);
		EXIT_NOT_IMPLEMENTED(usage.vertex_buffer_reg + 1 >= HW::UserSgprInfo::SGPRS_MAX);

		const auto* fetch = reinterpret_cast<const uint32_t*>(static_cast<uint64_t>(user_sgpr.value[usage.fetch_reg]) |
		                                                      (static_cast<uint64_t>(user_sgpr.value[usage.fetch_reg + 1]) << 32u));
		const auto* buffer =
		    reinterpret_cast<const uint32_t*>(static_cast<uint64_t>(user_sgpr.value[usage.vertex_buffer_reg]) |
		                                      (static_cast<uint64_t>(user_sgpr.value[usage.vertex_buffer_reg + 1]) << 32u));

		EXIT_NOT_IMPLEMENTED(fetch == nullptr || buffer == nullptr);

		ShaderParseFetch(info, fetch, buffer);
		ShaderDetectBuffers(info, ps5);
	}

	ShaderCalcBindingIndices(&info->bind);
}

void ShaderGetInputInfoPS(const HW::PixelShaderInfo* regs, const HW::ShaderRegisters* sh, const ShaderVertexInputInfo* vs_info,
                          ShaderPixelInputInfo* ps_info)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(vs_info == nullptr);
	EXIT_IF(ps_info == nullptr);
	EXIT_IF(regs == nullptr);
	EXIT_IF(sh == nullptr);

	if (regs->ps_embedded)
	{
		return;
	}

	ps_info->input_num            = sh->ps_in_control;
	ps_info->ps_pos_xy            = (sh->ps_input_ena == 0x00000302 && sh->ps_input_addr == 0x00000302);
	ps_info->ps_pixel_kill_enable = sh->db_shader_control.shader_kill_enable;
	ps_info->ps_early_z           = (sh->db_shader_control.shader_z_behavior == 1);
	ps_info->ps_execute_on_noop   = sh->db_shader_control.shader_execute_on_noop;

	for (uint32_t i = 0; i < ps_info->input_num; i++)
	{
		ps_info->interpolator_settings[i] = sh->ps_interpolator_settings[i];
	}

	ps_info->bind.descriptor_set_slot  = (vs_info->bind.storage_buffers.buffers_num > 0 ? 1 : 0);
	ps_info->bind.push_constant_offset = vs_info->bind.push_constant_offset + vs_info->bind.push_constant_size;
	ps_info->bind.push_constant_size   = 0;

	for (int i = 0; i < 8; i++)
	{
		ps_info->target_output_mode[i] = sh->target_output_mode[i];
	}

	bool ps5 = Config::IsNextGen();

	ShaderMappedData data;

	if (ps5)
	{
		if (auto iter = g_shader_map->find(regs->ps_regs.data_addr); iter != g_shader_map->end())
		{
			data = iter->second;
		}
	}

	ShaderParsedUsage usage;

	if (ps5)
	{
		EXIT_NOT_IMPLEMENTED(data.user_data == nullptr);

		ShaderParseUsage2(data.user_data, &usage, &ps_info->bind, regs->ps_user_sgpr, regs->ps_regs.rsrc2.user_sgpr);
	} else
	{
		ShaderParseUsage(regs->ps_regs.data_addr, &usage, &ps_info->bind, regs->ps_user_sgpr, regs->ps_regs.rsrc2.user_sgpr);
	}

	EXIT_NOT_IMPLEMENTED(usage.fetch || usage.vertex_buffer || usage.vertex_attrib);
	EXIT_NOT_IMPLEMENTED(usage.storage_buffers_readwrite > 0);
	EXIT_NOT_IMPLEMENTED(usage.gds_pointers > 0);
	EXIT_NOT_IMPLEMENTED(usage.direct_sgprs > 0);

	ShaderCalcBindingIndices(&ps_info->bind);
}

void ShaderGetInputInfoCS(const HW::ComputeShaderInfo* regs, const HW::ShaderRegisters* /*sh*/, ShaderComputeInputInfo* info)
{
	EXIT_IF(info == nullptr);
	EXIT_IF(regs == nullptr);

	info->threads_num[0] = regs->cs_regs.num_thread_x;
	info->threads_num[1] = regs->cs_regs.num_thread_y;
	info->threads_num[2] = regs->cs_regs.num_thread_z;
	info->group_id[0]    = regs->cs_regs.tgid_x_en != 0;
	info->group_id[1]    = regs->cs_regs.tgid_y_en != 0;
	info->group_id[2]    = regs->cs_regs.tgid_z_en != 0;
	info->thread_ids_num = regs->cs_regs.tidig_comp_cnt + 1;

	info->workgroup_register = regs->cs_regs.user_sgpr;

	info->bind.push_constant_offset = 0;
	info->bind.push_constant_size   = 0;
	info->bind.descriptor_set_slot  = 0;

	ShaderParsedUsage usage;

	ShaderParseUsage(regs->cs_regs.data_addr, &usage, &info->bind, regs->cs_user_sgpr, regs->cs_regs.user_sgpr);

	EXIT_NOT_IMPLEMENTED(usage.samplers > 0);
	EXIT_NOT_IMPLEMENTED(usage.fetch || usage.vertex_buffer || usage.vertex_attrib);
	EXIT_NOT_IMPLEMENTED(usage.direct_sgprs > 0);

	ShaderCalcBindingIndices(&info->bind);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void ShaderDbgDumpResources(const ShaderBindResources& bind)
{
	printf("\t descriptor_set_slot            = %u\n", bind.descriptor_set_slot);
	printf("\t push_constant_offset           = %u\n", bind.push_constant_offset);
	printf("\t push_constant_size             = %u\n", bind.push_constant_size);
	printf("\t storage_buffers.buffers_num    = %d\n", bind.storage_buffers.buffers_num);
	printf("\t storage_buffers.binding_index  = %d\n", bind.storage_buffers.binding_index);
	printf("\t textures.textures_num          = %d\n", bind.textures2D.textures_num);
	printf("\t textures.binding_sampled_index = %d\n", bind.textures2D.binding_sampled_index);
	printf("\t textures.binding_storage_index = %d\n", bind.textures2D.binding_storage_index);
	printf("\t samplers.samplers_num          = %d\n", bind.samplers.samplers_num);
	printf("\t samplers.binding_index         = %d\n", bind.samplers.binding_index);
	printf("\t gds_pointers.pointers_num      = %d\n", bind.gds_pointers.pointers_num);
	printf("\t gds_pointers.binding_index     = %d\n", bind.gds_pointers.binding_index);
	printf("\t direct_sgprs.sgprs_num         = %d\n", bind.direct_sgprs.sgprs_num);
	printf("\t extended.used                  = %s\n", (bind.extended.used ? "true" : "false"));
	printf("\t extended.slot                  = %d\n", bind.extended.slot);
	printf("\t extended.start_register        = %d\n", bind.extended.start_register);
	printf("\t extended.data.Base             = %" PRIx64 "\n", bind.extended.data.Base());

	bool gen5 = Config::IsNextGen();

	for (int i = 0; i < bind.storage_buffers.buffers_num; i++)
	{
		const auto& r = bind.storage_buffers.buffers[i];

		printf("\t StorageBuffer %d\n", i);

		printf("\t\t fields           = %08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "\n", r.fields[3], r.fields[2], r.fields[1],
		       r.fields[0]);
		printf("\t\t Base()           = %" PRIx64 "\n", gen5 ? r.Base48() : r.Base44());
		printf("\t\t Stride()         = %" PRIu16 "\n", r.Stride());
		printf("\t\t SwizzleEnabled() = %s\n", r.SwizzleEnabled() ? "true" : "false");
		printf("\t\t NumRecords()     = %" PRIu32 "\n", r.NumRecords());
		printf("\t\t DstSelX()        = %" PRIu8 "\n", r.DstSelX());
		printf("\t\t DstSelY()        = %" PRIu8 "\n", r.DstSelY());
		printf("\t\t DstSelZ()        = %" PRIu8 "\n", r.DstSelZ());
		printf("\t\t DstSelW()        = %" PRIu8 "\n", r.DstSelW());
		if (!gen5)
		{
			printf("\t\t Nfmt()           = %" PRIu8 "\n", r.Nfmt());
			printf("\t\t Dfmt()           = %" PRIu8 "\n", r.Dfmt());
			printf("\t\t MemoryType()     = 0x%02" PRIx8 "\n", r.MemoryType());
		} else
		{
			printf("\t\t Format()         = %" PRIu8 "\n", r.Format());
			printf("\t\t OutOfBounds()    = %" PRIu8 "\n", r.OutOfBounds());
		}
		printf("\t\t AddTid()         = %s\n", r.AddTid() ? "true" : "false");
		printf("\t\t slot             = %d\n", bind.storage_buffers.slots[i]);
		printf("\t\t start_register   = %d\n", bind.storage_buffers.start_register[i]);
		printf("\t\t extended         = %s\n", (bind.storage_buffers.extended[i] ? "true" : "false"));
		printf("\t\t usage            = %s\n", Core::EnumName8(bind.storage_buffers.usages[i]).c_str());
	}

	for (int i = 0; i < bind.textures2D.textures_num; i++)
	{
		const auto& r = bind.textures2D.desc[i].texture;

		printf("\t Texture %d\n", i);

		printf("\t\t fields = %08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "\n",
		       r.fields[7], r.fields[6], r.fields[5], r.fields[4], r.fields[3], r.fields[2], r.fields[1], r.fields[0]);
		printf("\t\t Base()          = %016" PRIx64 "\n", gen5 ? r.Base40() : r.Base38());
		printf("\t\t MinLod()        = %" PRIu16 "\n", r.MinLod());
		if (gen5)
		{
			printf("\t\t Format()        = %" PRIu16 "\n", r.Format());
			printf("\t\t BCSwizzle()     = %" PRIu8 "\n", r.BCSwizzle());
			printf("\t\t BaseArray5()    = %" PRIu16 "\n", r.BaseArray5());
			printf("\t\t ArrayPitch()    = %" PRIu8 "\n", r.ArrayPitch());
			printf("\t\t MaxMip()        = %" PRIu8 "\n", r.MaxMip());
			printf("\t\t MinLodWarn5()   = %" PRIu16 "\n", r.MinLodWarn5());
			printf("\t\t PerfMod5()      = %" PRIu8 "\n", r.PerfMod5());
			printf("\t\t CornerSample()  = %s\n", r.CornerSample() ? "true" : "false");
			printf("\t\t MipStatsCntEn() = %s\n", r.MipStatsCntEn() ? "true" : "false");
			printf("\t\t PrtDefColor()   = %s\n", r.PrtDefColor() ? "true" : "false");
			printf("\t\t MipStatsCntId() = %" PRIu8 "\n", r.MipStatsCntId());
			printf("\t\t MsaaDepth()     = %s\n", r.MsaaDepth() ? "true" : "false");
			printf("\t\t MaxUncBlkSize() = %" PRIu8 "\n", r.MaxUncompBlkSize());
			printf("\t\t MaxCompBlkSize()= %" PRIu8 "\n", r.MaxCompBlkSize());
			printf("\t\t MetaPipeAlign() = %s\n", r.MetaPipeAligned() ? "true" : "false");
			printf("\t\t WriteCompress() = %s\n", r.WriteCompress() ? "true" : "false");
			printf("\t\t MetaCompress()  = %s\n", r.MetaCompress() ? "true" : "false");
			printf("\t\t DccAlphaPos()   = %s\n", r.DccAlphaPos() ? "true" : "false");
			printf("\t\t DccColorTransf()= %s\n", r.DccColorTransf() ? "true" : "false");
			printf("\t\t MetaAddr()      = %" PRIx64 "\n", r.MetaAddr());

		} else
		{
			printf("\t\t Dfmt()          = %" PRIu8 "\n", r.Dfmt());
			printf("\t\t Nfmt()          = %" PRIu8 "\n", r.Nfmt());
			printf("\t\t PerfMod()       = %" PRIu8 "\n", r.PerfMod());
			printf("\t\t Interlaced()    = %s\n", r.Interlaced() ? "true" : "false");
			printf("\t\t MemoryType()    = 0x%02" PRIx8 "\n", r.MemoryType());
			printf("\t\t Pow2Pad()       = %s\n", r.Pow2Pad() ? "true" : "false");
			printf("\t\t Pitch()         = %" PRIu16 "\n", r.Pitch());
			printf("\t\t BaseArray()     = %" PRIu16 "\n", r.BaseArray());
			printf("\t\t LastArray()     = %" PRIu16 "\n", r.LastArray());
			printf("\t\t MinLodWarn()    = %" PRIu16 "\n", r.MinLodWarn());
			printf("\t\t LodHdwCntEn()   = %s\n", r.LodHdwCntEn() ? "true" : "false");
			printf("\t\t CounterBankId() = %" PRIu8 "\n", r.CounterBankId());
		}
		printf("\t\t Width()         = %" PRIu16 "\n", gen5 ? r.Width5() : r.Width4());
		printf("\t\t Height()        = %" PRIu16 "\n", gen5 ? r.Height5() : r.Height4());
		printf("\t\t DstSelX()       = %" PRIu8 "\n", r.DstSelX());
		printf("\t\t DstSelY()       = %" PRIu8 "\n", r.DstSelY());
		printf("\t\t DstSelZ()       = %" PRIu8 "\n", r.DstSelZ());
		printf("\t\t DstSelW()       = %" PRIu8 "\n", r.DstSelW());
		printf("\t\t BaseLevel()     = %" PRIu8 "\n", r.BaseLevel());
		printf("\t\t LastLevel()     = %" PRIu8 "\n", r.LastLevel());
		printf("\t\t TileMode()      = %" PRIu8 "\n", r.TileMode());
		printf("\t\t Type()          = %" PRIu8 "\n", r.Type());
		printf("\t\t Depth()         = %" PRIu16 "\n", r.Depth());
		printf("\t\t slot            = %d\n", bind.textures2D.desc[i].slot);
		printf("\t\t start_register  = %d\n", bind.textures2D.desc[i].start_register);
		printf("\t\t extended        = %s\n", (bind.textures2D.desc[i].extended ? "true" : "false"));
		printf("\t\t usage           = %s\n", Core::EnumName8(bind.textures2D.desc[i].usage).c_str());
	}

	for (int i = 0; i < bind.samplers.samplers_num; i++)
	{
		const auto& r = bind.samplers.samplers[i];

		printf("\t Sampler %d\n", i);

		printf("\t\t fields = %08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "\n", r.fields[3], r.fields[2], r.fields[1], r.fields[0]);

		printf("\t\t ClampX()           = %" PRIu8 "\n", r.ClampX());
		printf("\t\t ClampY()           = %" PRIu8 "\n", r.ClampY());
		printf("\t\t ClampZ()           = %" PRIu8 "\n", r.ClampZ());
		printf("\t\t MaxAnisoRatio()    = %" PRIu8 "\n", r.MaxAnisoRatio());
		printf("\t\t DepthCompareFunc() = %" PRIu8 "\n", r.DepthCompareFunc());
		printf("\t\t ForceUnormCoords() = %s\n", r.ForceUnormCoords() ? "true" : "false");
		printf("\t\t AnisoThreshold()   = %" PRIu8 "\n", r.AnisoThreshold());
		if (!gen5)
		{
			printf("\t\t McCoordTrunc()     = %s\n", r.McCoordTrunc() ? "true" : "false");
		} else
		{
			printf("\t\t SkipDegamma()      = %s\n", r.SkipDegamma() ? "true" : "false");
			printf("\t\t PointPreclamp()    = %s\n", r.PointPreclamp() ? "true" : "false");
			printf("\t\t AnisoOverride()    = %s\n", r.AnisoOverride() ? "true" : "false");
			printf("\t\t BlendZeroPrt()     = %s\n", r.BlendZeroPrt() ? "true" : "false");
		}
		printf("\t\t ForceDegamma()     = %s\n", r.ForceDegamma() ? "true" : "false");
		printf("\t\t AnisoBias()        = %" PRIu8 "\n", r.AnisoBias());
		printf("\t\t TruncCoord()       = %s\n", r.TruncCoord() ? "true" : "false");
		printf("\t\t DisableCubeWrap()  = %s\n", r.DisableCubeWrap() ? "true" : "false");
		printf("\t\t FilterMode()       = %" PRIu8 "\n", r.FilterMode());
		printf("\t\t MinLod()           = %" PRIu16 "\n", r.MinLod());
		printf("\t\t MaxLod()           = %" PRIu16 "\n", r.MaxLod());
		printf("\t\t PerfMip()          = %" PRIu8 "\n", r.PerfMip());
		printf("\t\t PerfZ()            = %" PRIu8 "\n", r.PerfZ());
		printf("\t\t LodBias()          = %" PRIu16 "\n", r.LodBias());
		printf("\t\t LodBiasSec()       = %" PRIu8 "\n", r.LodBiasSec());
		printf("\t\t XyMagFilter()      = %" PRIu8 "\n", r.XyMagFilter());
		printf("\t\t XyMinFilter()      = %" PRIu8 "\n", r.XyMinFilter());
		printf("\t\t ZFilter()          = %" PRIu8 "\n", r.ZFilter());
		printf("\t\t MipFilter()        = %" PRIu8 "\n", r.MipFilter());
		printf("\t\t BorderColorPtr()   = %" PRIu16 "\n", r.BorderColorPtr());
		printf("\t\t BorderColorType()  = %" PRIu8 "\n", r.BorderColorType());
		printf("\t\t slot               = %d\n", bind.samplers.slots[i]);
		printf("\t\t start_register     = %d\n", bind.samplers.start_register[i]);
		printf("\t\t extended           = %s\n", (bind.samplers.extended[i] ? "true" : "false"));
	}

	for (int i = 0; i < bind.gds_pointers.pointers_num; i++)
	{
		const auto& r = bind.gds_pointers.pointers[i];

		printf("\t Gds Pointer %d\n", i);

		printf("\t\t field = %08" PRIx32 "\n", r.field);

		printf("\t\t Base()         = %" PRIu16 "\n", r.Base());
		printf("\t\t Size()         = %" PRIu16 "\n", r.Size());
		printf("\t\t slot           = %d\n", bind.gds_pointers.slots[i]);
		printf("\t\t start_register = %d\n", bind.gds_pointers.start_register[i]);
		printf("\t\t extended       = %s\n", (bind.gds_pointers.extended[i] ? "true" : "false"));
	}

	for (int i = 0; i < bind.direct_sgprs.sgprs_num; i++)
	{
		const auto& r = bind.direct_sgprs.sgprs[i];

		printf("\t Direct Sgprs %d\n", i);

		printf("\t\t field = %08" PRIx32 "\n", r.field);

		printf("\t\t start_register = %d\n", bind.direct_sgprs.start_register[i]);
	}
}

void ShaderDbgDumpInputInfo(const ShaderVertexInputInfo* info)
{
	KYTY_PROFILER_BLOCK("ShaderDbgDumpInputInfo(Vs)");

	printf("ShaderDbgDumpInputInfo()\n");

	printf("\t fetch_external = %s\n", info->fetch_external ? "true" : "false");
	printf("\t fetch_embedded = %s\n", info->fetch_embedded ? "true" : "false");
	printf("\t fetch_inline   = %s\n", info->fetch_inline ? "true" : "false");
	printf("\t export_count   = %d\n", info->export_count);

	bool gen5 = Config::IsNextGen();

	for (int i = 0; i < info->resources_num; i++)
	{
		printf("\t input %d\n", i);

		const auto& r  = info->resources[i];
		const auto& rd = info->resources_dst[i];

		printf("\t\t register_start   = %d\n", rd.register_start);
		printf("\t\t registers_num    = %d\n", rd.registers_num);
		printf("\t\t fields           = %08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "\n", r.fields[3], r.fields[2], r.fields[1],
		       r.fields[0]);
		printf("\t\t Base()           = %" PRIx64 "\n", gen5 ? r.Base48() : r.Base44());
		printf("\t\t Stride()         = %" PRIu16 "\n", r.Stride());
		printf("\t\t SwizzleEnabled() = %s\n", r.SwizzleEnabled() ? "true" : "false");
		printf("\t\t NumRecords()     = %" PRIu32 "\n", r.NumRecords());
		printf("\t\t DstSelX()        = %" PRIu8 "\n", r.DstSelX());
		printf("\t\t DstSelY()        = %" PRIu8 "\n", r.DstSelY());
		printf("\t\t DstSelZ()        = %" PRIu8 "\n", r.DstSelZ());
		printf("\t\t DstSelW()        = %" PRIu8 "\n", r.DstSelW());
		if (!gen5)
		{
			printf("\t\t Nfmt()           = %" PRIu8 "\n", r.Nfmt());
			printf("\t\t Dfmt()           = %" PRIu8 "\n", r.Dfmt());
			printf("\t\t MemoryType()     = 0x%02" PRIx8 "\n", r.MemoryType());
		} else
		{
			printf("\t\t Format()         = %" PRIu8 "\n", r.Format());
			printf("\t\t OutOfBounds()    = %" PRIu8 "\n", r.OutOfBounds());
		}
		printf("\t\t AddTid()         = %s\n", r.AddTid() ? "true" : "false");
	}

	for (int i = 0; i < info->buffers_num; i++)
	{
		printf("\t buffer %d\n", i);

		const auto& r = info->buffers[i];
		printf("\t\t addr        = %" PRIx64 "\n", r.addr);
		printf("\t\t stride      = %" PRIu32 "\n", r.stride);
		printf("\t\t num_records = %" PRIu32 "\n", r.num_records);
		printf("\t\t attr_num    = %" PRId32 "\n", r.attr_num);
		for (int j = 0; j < r.attr_num; j++)
		{
			printf("\t\t attr_indices[%d]  = %d\n", j, r.attr_indices[j]);
			printf("\t\t attr_offsets[%d]  = %u\n", j, r.attr_offsets[j]);
		}
	}

	ShaderDbgDumpResources(info->bind);
}

void ShaderDbgDumpInputInfo(const ShaderPixelInputInfo* info)
{
	KYTY_PROFILER_BLOCK("ShaderDbgDumpInputInfo(Ps)");

	printf("ShaderDbgDumpInputInfo()\n");

	printf("\t input_num            = %u\n", info->input_num);
	printf("\t ps_pos_xy            = %s\n", info->ps_pos_xy ? "true" : "false");
	printf("\t ps_pixel_kill_enable = %s\n", info->ps_pixel_kill_enable ? "true" : "false");
	printf("\t ps_early_z           = %s\n", info->ps_early_z ? "true" : "false");
	printf("\t ps_execute_on_noop   = %s\n", info->ps_execute_on_noop ? "true" : "false");

	for (uint32_t i = 0; i < info->input_num; i++)
	{
		printf("\t interpolator_settings[%u] = %u\n", i, info->interpolator_settings[i]);
	}

	ShaderDbgDumpResources(info->bind);
}

void ShaderDbgDumpInputInfo(const ShaderComputeInputInfo* info)
{
	printf("ShaderDbgDumpInputInfo()\n");

	printf("\t workgroup_register = %d\n", info->workgroup_register);
	printf("\t thread_ids_num     = %d\n", info->thread_ids_num);
	printf("\t threads_num        = {%u, %u, %u}\n", info->threads_num[0], info->threads_num[1], info->threads_num[2]);
	printf("\t threadgroup_id     = {%s, %s, %s}\n", info->group_id[0] ? "true" : "false", info->group_id[1] ? "true" : "false",
	       info->group_id[2] ? "true" : "false");

	ShaderDbgDumpResources(info->bind);
}

class ShaderLogHelper
{
public:
	explicit ShaderLogHelper(const char* type)
	{
		static std::atomic_int id = 0;

		switch (Config::GetShaderLogDirection())
		{
			case Config::ShaderLogDirection::Console:
				m_console = true;
				m_enabled = true;
				break;
			case Config::ShaderLogDirection::File:
			{
				m_file_name = Config::GetShaderLogFolder().FixDirectorySlash() +
				              String::FromPrintf("%04d_%04d_shader_%s.log", GraphicsRunGetFrameNum(), id++, type);
				Core::File::CreateDirectories(m_file_name.DirectoryWithoutFilename());
				m_file.Create(m_file_name);
				if (m_file.IsInvalid())
				{
					printf(FG_BRIGHT_RED "Can't create file: %s\n" FG_DEFAULT, m_file_name.C_Str());
					m_enabled = false;
				}
				m_enabled = true;
				m_console = false;
			}
			break;
			default: m_enabled = false; break;
		}
	}
	virtual ~ShaderLogHelper()
	{
		if (m_enabled && !m_console && !m_file.IsInvalid())
		{
			m_file.Close();
		}
	}

	KYTY_CLASS_NO_COPY(ShaderLogHelper);

	void DumpOriginalShader(const ShaderCode& code)
	{
		if (m_enabled)
		{
			if (m_console)
			{
				printf("--------- Original Shader ---------\n");
				printf("crc32 = %08" PRIx32 "\n", code.GetCrc32());
				printf("hash0 = %08" PRIx32 "\n", code.GetHash0());
				printf("---------\n");
				printf("%s", code.DbgDump().c_str());
				printf("---------\n");
			} else if (!m_file.IsInvalid())
			{
				m_file.Printf("--------- Original Shader ---------\n");
				m_file.Printf("crc32 = %08" PRIx32 "\n", code.GetCrc32());
				m_file.Printf("hash0 = %08" PRIx32 "\n", code.GetHash0());
				m_file.Printf("---------\n");
				m_file.Printf("%s", code.DbgDump().c_str());
				m_file.Printf("---------\n");
			}
		}
	}

	void DumpRecompiledShader(const String8& source)
	{
		if (m_enabled)
		{
			if (m_console)
			{
				printf("--------- Recompiled Shader ---------\n");
				printf("%s\n", source.c_str());
				printf("---------\n");
			} else if (!m_file.IsInvalid())
			{
				m_file.Printf("--------- Recompiled Shader ---------\n");
				m_file.Printf("%s\n", source.c_str());
				m_file.Printf("---------\n");
			}
		}
	}

	void DumpOptimizedShader(const Vector<uint32_t>& bin)
	{
		if (m_enabled)
		{
			String8 text;
			if (!SpirvDisassemble(bin.GetDataConst(), bin.Size(), &text))
			{
				EXIT("SpirvDisassemble() failed\n");
			}
			if (m_console)
			{
				printf("--------- Optimized Shader ---------\n");
				printf("%s\n", text.c_str());
				printf("---------\n");
			} else if (!m_file.IsInvalid())
			{
				m_file.Printf("--------- Optimized Shader ---------\n");
				m_file.Printf("%s\n", Log::RemoveColors(String::FromUtf8(text.c_str())).C_Str());
				m_file.Printf("---------\n");
			}
		}
	}

	void DumpGlslShader(const Vector<uint32_t>& bin)
	{
		if (m_enabled)
		{
			String8 text;
			if (!SpirvToGlsl(bin.GetDataConst(), bin.Size(), &text))
			{
				EXIT("SpirvToGlsl() failed\n");
			}
			if (m_console)
			{
				printf("--------- Glsl Shader ---------\n");
				printf("%s\n", text.c_str());
				printf("---------\n");
			} else if (!m_file.IsInvalid())
			{
				m_file.Printf("--------- Glsl Shader ---------\n");
				m_file.Printf("%s\n", Log::RemoveColors(String::FromUtf8(text.c_str())).C_Str());
				m_file.Printf("---------\n");
			}
		}
	}

	void DumpBinary(const Vector<uint32_t>& bin)
	{
		if (m_enabled && !m_console && !m_file.IsInvalid())
		{
			Core::File file;
			String     file_name = m_file_name.FilenameWithoutExtension() + ".spv";
			file.Create(file_name);
			if (file.IsInvalid())
			{
				printf(FG_BRIGHT_RED "Can't create file: %s\n" FG_DEFAULT, file_name.C_Str());
			} else
			{
				file.Write(bin.GetDataConst(), bin.Size() * 4);
				file.Close();
			}
		}
	}

private:
	bool       m_console = false;
	bool       m_enabled = false;
	Core::File m_file;
	String     m_file_name;
};

ShaderCode ShaderParseVS(const HW::VertexShaderInfo* regs, const HW::ShaderRegisters* sh)
{
	KYTY_PROFILER_FUNCTION(profiler::colors::Amber300);

	EXIT_IF(regs == nullptr);
	EXIT_IF(sh == nullptr);

	ShaderCode code;
	code.SetType(ShaderType::Vertex);

	if (regs->vs_embedded)
	{
		code.SetVsEmbedded(true);
		code.SetVsEmbeddedId(regs->vs_embedded_id);
	} else
	{
		uint32_t hash0 = 0;
		uint32_t crc32 = 0;

		bool gs_instead_of_vs =
		    (regs->vs_regs.data_addr == 0 && regs->gs_regs.data_addr == 0 && regs->es_regs.data_addr != 0 && regs->gs_regs.chksum != 0);
		uint64_t shader_addr = (gs_instead_of_vs ? regs->es_regs.data_addr : regs->vs_regs.data_addr);

		const auto* src = reinterpret_cast<const uint32_t*>(shader_addr);

		EXIT_NOT_IMPLEMENTED(src == nullptr);

		vs_print("ShaderParseVS()", *regs, *sh);
		vs_check(*regs, *sh);

		if (gs_instead_of_vs)
		{
			EXIT_NOT_IMPLEMENTED(regs->gs_regs.rsrc2.user_sgpr > regs->gs_user_sgpr.count);
		} else
		{
			EXIT_NOT_IMPLEMENTED(regs->vs_regs.rsrc2.user_sgpr > regs->vs_user_sgpr.count);
		}

		if (Config::IsNextGen())
		{
			EXIT_NOT_IMPLEMENTED(!gs_instead_of_vs);

			hash0 = (regs->gs_regs.chksum >> 32u) & 0xffffffffu;
			crc32 = regs->gs_regs.chksum & 0xffffffffu;
		} else
		{
			const auto* header = GetBinaryInfo(src);

			EXIT_NOT_IMPLEMENTED(header == nullptr);

			bi_print("ShaderParseVS():ShaderBinaryInfo", *header);

			hash0 = header->hash0;
			crc32 = header->crc32;
		}

		code.SetCrc32(crc32);
		code.SetHash0(hash0);
		// shader_parse(0, src, nullptr, &code);
		ShaderParse(src, &code);

		if (g_debug_printfs != nullptr)
		{
			auto id = (static_cast<uint64_t>(hash0) << 32u) | crc32;
			if (auto index = g_debug_printfs->Find(id, [](auto cmd, auto id) { return cmd.id == id; }); g_debug_printfs->IndexValid(index))
			{
				code.GetDebugPrintfs() = g_debug_printfs->At(index).cmds;
			}
		}
	}

	return code;
}

Vector<uint32_t> ShaderRecompileVS(const ShaderCode& code, const ShaderVertexInputInfo* input_info)
{
	KYTY_PROFILER_FUNCTION(profiler::colors::Amber300);

	String8          source;
	Vector<uint32_t> ret;
	ShaderLogHelper  log("vs");

	if (code.IsVsEmbedded())
	{
		source = SpirvGetEmbeddedVs(code.GetVsEmbeddedId());
	} else
	{
		for (int i = 0; i < input_info->bind.storage_buffers.buffers_num; i++)
		{
			const auto& r = input_info->bind.storage_buffers.buffers[i];
			EXIT_NOT_IMPLEMENTED(((r.Stride() * r.NumRecords()) & 0x3u) != 0);
		}

		log.DumpOriginalShader(code);

		source = SpirvGenerateSource(code, input_info, nullptr, nullptr);
	}

	log.DumpRecompiledShader(source);

	if (String8 err_msg; !SpirvRun(source, &ret, &err_msg))
	{
		EXIT("SpirvRun() failed:\n%s\n", err_msg.c_str());
	}

	log.DumpOptimizedShader(ret);

	return ret;
}

ShaderCode ShaderParsePS(const HW::PixelShaderInfo* regs, const HW::ShaderRegisters* sh)
{
	KYTY_PROFILER_FUNCTION(profiler::colors::Blue300);

	EXIT_IF(regs == nullptr);
	EXIT_IF(sh == nullptr);

	ShaderCode code;
	code.SetType(ShaderType::Pixel);

	if (regs->ps_embedded)
	{
		code.SetPsEmbedded(true);
		code.SetPsEmbeddedId(regs->ps_embedded_id);
	} else
	{
		uint32_t hash0 = 0;
		uint32_t crc32 = 0;

		ps_print("ShaderParsePS()", regs->ps_regs, *sh);
		ps_check(regs->ps_regs, *sh);

		EXIT_NOT_IMPLEMENTED(regs->ps_regs.rsrc2.user_sgpr > regs->ps_user_sgpr.count);

		const auto* src = reinterpret_cast<const uint32_t*>(regs->ps_regs.data_addr);

		EXIT_NOT_IMPLEMENTED(src == nullptr);

		if (Config::IsNextGen())
		{
			hash0 = (regs->ps_regs.chksum >> 32u) & 0xffffffffu;
			crc32 = regs->ps_regs.chksum & 0xffffffffu;
		} else
		{
			const auto* header = GetBinaryInfo(src);

			EXIT_NOT_IMPLEMENTED(header == nullptr);

			bi_print("ShaderParsePS():ShaderBinaryInfo", *header);

			hash0 = header->hash0;
			crc32 = header->crc32;
		}

		code.SetCrc32(crc32);
		code.SetHash0(hash0);
		// shader_parse(0, src, nullptr, &code);
		ShaderParse(src, &code);

		if (g_debug_printfs != nullptr)
		{
			auto id = (static_cast<uint64_t>(hash0) << 32u) | crc32;
			if (auto index = g_debug_printfs->Find(id, [](auto cmd, auto id) { return cmd.id == id; }); g_debug_printfs->IndexValid(index))
			{
				code.GetDebugPrintfs() = g_debug_printfs->At(index).cmds;
			}
		}
	}

	return code;
}

Vector<uint32_t> ShaderRecompilePS(const ShaderCode& code, const ShaderPixelInputInfo* input_info)
{
	KYTY_PROFILER_FUNCTION(profiler::colors::Blue300);

	String8          source;
	Vector<uint32_t> ret;
	ShaderLogHelper  log("ps");

	if (code.IsPsEmbedded())
	{
		source = SpirvGetEmbeddedPs(code.GetPsEmbeddedId());
	} else
	{
		//		for (uint32_t i = 0; i < input_info->input_num; i++)
		//		{
		//			EXIT_NOT_IMPLEMENTED(input_info->interpolator_settings[i] != i);
		//		}

		for (int i = 0; i < input_info->bind.storage_buffers.buffers_num; i++)
		{
			const auto& r = input_info->bind.storage_buffers.buffers[i];
			EXIT_NOT_IMPLEMENTED(((r.Stride() * r.NumRecords()) & 0x3u) != 0);
		}

		log.DumpOriginalShader(code);

		source = SpirvGenerateSource(code, nullptr, input_info, nullptr);
	}

	log.DumpRecompiledShader(source);

	if (String8 err_msg; !SpirvRun(source, &ret, &err_msg))
	{
		EXIT("SpirvRun() failed:\n%s\n", err_msg.c_str());
	}

	log.DumpOptimizedShader(ret);

	return ret;
}

ShaderCode ShaderParseCS(const HW::ComputeShaderInfo* regs, const HW::ShaderRegisters* sh)
{
	KYTY_PROFILER_FUNCTION(profiler::colors::CyanA700);

	EXIT_IF(regs == nullptr);
	EXIT_IF(sh == nullptr);

	const auto* src = reinterpret_cast<const uint32_t*>(regs->cs_regs.data_addr);

	EXIT_NOT_IMPLEMENTED(src == nullptr);

	cs_print("ShaderParseCS()", regs->cs_regs, *sh);
	cs_check(regs->cs_regs, *sh);

	EXIT_NOT_IMPLEMENTED(regs->cs_regs.user_sgpr > regs->cs_user_sgpr.count);

	const auto* header = GetBinaryInfo(src);

	EXIT_NOT_IMPLEMENTED(header == nullptr);

	bi_print("ShaderParseCS():ShaderBinaryInfo", *header);

	ShaderCode code;
	code.SetType(ShaderType::Compute);

	code.SetCrc32(header->crc32);
	code.SetHash0(header->hash0);
	// shader_parse(0, src, nullptr, &code);
	ShaderParse(src, &code);

	if (g_debug_printfs != nullptr)
	{
		auto id = (static_cast<uint64_t>(header->hash0) << 32u) | header->crc32;
		if (auto index = g_debug_printfs->Find(id, [](auto cmd, auto id) { return cmd.id == id; }); g_debug_printfs->IndexValid(index))
		{
			code.GetDebugPrintfs() = g_debug_printfs->At(index).cmds;
		}
	}

	return code;
}

Vector<uint32_t> ShaderRecompileCS(const ShaderCode& code, const ShaderComputeInputInfo* input_info)
{
	KYTY_PROFILER_FUNCTION(profiler::colors::CyanA700);

	ShaderLogHelper log("cs");

	for (int i = 0; i < input_info->bind.storage_buffers.buffers_num; i++)
	{
		const auto& r = input_info->bind.storage_buffers.buffers[i];
		EXIT_NOT_IMPLEMENTED(((r.Stride() * r.NumRecords()) & 0x3u) != 0);
	}

	Vector<uint32_t> ret;

	log.DumpOriginalShader(code);

	auto source = SpirvGenerateSource(code, nullptr, nullptr, input_info);

	log.DumpRecompiledShader(source);

	if (String8 err_msg; !SpirvRun(source, &ret, &err_msg))
	{
		EXIT("SpirvRun() failed:\n%s\n", err_msg.c_str());
	}

	log.DumpOptimizedShader(ret);
	// log.DumpGlslShader(ret);
	log.DumpBinary(ret);

	return ret;
}

//// NOLINTNEXTLINE(readability-function-cognitive-complexity)
// static ShaderBindParameters ShaderUpdateBindInfo(const ShaderCode& code, const ShaderBindResources* bind)
//{
//	ShaderBindParameters p {};
//
//	auto find_image_op = [&](int index, int s, bool& found, bool& without_sampler)
//	{
//		const auto& insts = code.GetInstructions();
//		int         size  = static_cast<int>(insts.Size());
//		for (int i = index; i < size; i++)
//		{
//			const auto& inst = insts.At(i);
//
//			if ((inst.dst.type == ShaderOperandType::Sgpr && s >= inst.dst.register_id && s < inst.dst.register_id + inst.dst.size) ||
//			    (inst.dst2.type == ShaderOperandType::Sgpr && s >= inst.dst2.register_id && s < inst.dst2.register_id + inst.dst2.size) ||
//			    inst.type == ShaderInstructionType::SEndpgm)
//			{
//				break;
//			}
//
//			if (inst.type == ShaderInstructionType::ImageStore || inst.type == ShaderInstructionType::ImageStoreMip)
//			{
//				if (inst.src[1].register_id == s)
//				{
//					EXIT_NOT_IMPLEMENTED(found && !without_sampler);
//					without_sampler = true;
//					found           = true;
//				}
//			} else if (inst.type == ShaderInstructionType::ImageSample || inst.type == ShaderInstructionType::ImageLoad)
//			{
//				if (inst.src[1].register_id == s)
//				{
//					EXIT_NOT_IMPLEMENTED(found && without_sampler);
//					without_sampler = false;
//					found           = true;
//				}
//			}
//		}
//	};
//
//	if (bind->textures2D.textures_num > 0)
//	{
//		const auto& insts = code.GetInstructions();
//
//		for (int ti = 0; ti < bind->textures2D.textures_num; ti++)
//		{
//			bool found = false;
//			if (bind->textures2D.desc[ti].extended)
//			{
//				int s = bind->extended.start_register;
//
//				int index = 0;
//				for (const auto& inst: insts)
//				{
//					if ((inst.dst.type == ShaderOperandType::Sgpr && s >= inst.dst.register_id &&
//					     s < inst.dst.register_id + inst.dst.size) ||
//					    (inst.dst2.type == ShaderOperandType::Sgpr && s >= inst.dst2.register_id &&
//					     s < inst.dst2.register_id + inst.dst2.size) ||
//					    inst.type == ShaderInstructionType::SEndpgm)
//					{
//						break;
//					}
//
//					if (inst.type == ShaderInstructionType::SLoadDwordx8 && inst.src[0].register_id == s &&
//					    static_cast<int>(inst.src[1].constant.u >> 2u) + 16 == bind->textures2D.desc[ti].start_register)
//					{
//						find_image_op(index + 1, inst.dst.register_id, found, p.textures2d_without_sampler[ti]);
//					}
//
//					index++;
//				}
//			} else
//			{
//				find_image_op(0, bind->textures2D.desc[ti].start_register, found, p.textures2d_without_sampler[ti]);
//			}
//
//			EXIT_NOT_IMPLEMENTED(!found);
//
//			if (p.textures2d_without_sampler[ti])
//			{
//				p.textures2d_storage_num++;
//			} else
//			{
//				p.textures2d_sampled_num++;
//			}
//		}
//	}
//	return p;
//}
//
// ShaderBindParameters ShaderGetBindParametersVS(const ShaderCode& code, const ShaderVertexInputInfo* input_info)
//{
//	return ShaderUpdateBindInfo(code, &input_info->bind);
//}
//
// ShaderBindParameters ShaderGetBindParametersPS(const ShaderCode& code, const ShaderPixelInputInfo* input_info)
//{
//	return ShaderUpdateBindInfo(code, &input_info->bind);
//}
//
// ShaderBindParameters ShaderGetBindParametersCS(const ShaderCode& code, const ShaderComputeInputInfo* input_info)
//{
//	return ShaderUpdateBindInfo(code, &input_info->bind);
//}

static void ShaderGetBindIds(ShaderId* ret, const ShaderBindResources& bind)
{
	ret->ids.Add(bind.storage_buffers.buffers_num);

	for (int i = 0; i < bind.storage_buffers.buffers_num; i++)
	{
		// const auto& r = bind.storage_buffers.buffers[i];

		// ret->ids.Add(static_cast<uint32_t>(r.SwizzleEnabled()));
		// ret->ids.Add(r.DstSelX());
		// ret->ids.Add(r.DstSelY());
		// ret->ids.Add(r.DstSelZ());
		// ret->ids.Add(r.DstSelW());
		// ret->ids.Add(r.Nfmt());
		// ret->ids.Add(r.Dfmt());
		// ret->ids.Add(static_cast<uint32_t>(r.AddTid()));
		ret->ids.Add(bind.storage_buffers.slots[i]);
		ret->ids.Add(bind.storage_buffers.start_register[i]);
		ret->ids.Add(static_cast<uint32_t>(bind.storage_buffers.extended[i]));
		ret->ids.Add(static_cast<uint32_t>(bind.storage_buffers.usages[i]));
	}

	ret->ids.Add(bind.textures2D.textures_num);

	for (int i = 0; i < bind.textures2D.textures_num; i++)
	{
		// const auto& r = bind.textures2D.textures[i];
		// ret->ids.Add(r.MinLod());
		// ret->ids.Add(r.Dfmt());
		// ret->ids.Add(r.Nfmt());
		// ret->ids.Add(r.Width());
		// ret->ids.Add(r.Height());
		// ret->ids.Add(r.PerfMod());
		// ret->ids.Add(static_cast<uint32_t>(r.Interlaced()));
		// ret->ids.Add(r.DstSelX());
		// ret->ids.Add(r.DstSelY());
		// ret->ids.Add(r.DstSelZ());
		// ret->ids.Add(r.DstSelW());
		// ret->ids.Add(r.BaseLevel());
		// ret->ids.Add(r.LastLevel());
		// ret->ids.Add(r.TilingIdx());
		// ret->ids.Add(static_cast<uint32_t>(r.Pow2Pad()));
		// ret->ids.Add(r.Type());
		// ret->ids.Add(r.Depth());
		// ret->ids.Add(r.Pitch());
		// ret->ids.Add(r.BaseArray());
		// ret->ids.Add(r.LastArray());
		// ret->ids.Add(r.MinLodWarn());
		// ret->ids.Add(r.CounterBankId());
		// ret->ids.Add(static_cast<uint32_t>(r.LodHdwCntEn()));
		ret->ids.Add(bind.textures2D.desc[i].slot);
		ret->ids.Add(bind.textures2D.desc[i].start_register);
		ret->ids.Add(static_cast<uint32_t>(bind.textures2D.desc[i].extended));
		ret->ids.Add(static_cast<uint32_t>(bind.textures2D.desc[i].usage));
	}

	ret->ids.Add(bind.samplers.samplers_num);

	for (int i = 0; i < bind.samplers.samplers_num; i++)
	{
		// const auto& r = bind.samplers.samplers[i];

		// ret->ids.Add(r.ClampX());
		// ret->ids.Add(r.ClampY());
		// ret->ids.Add(r.ClampZ());
		// ret->ids.Add(r.MaxAnisoRatio());
		// ret->ids.Add(r.DepthCompareFunc());
		// ret->ids.Add(static_cast<uint32_t>(r.ForceUnormCoords()));
		// ret->ids.Add(r.AnisoThreshold());
		// ret->ids.Add(static_cast<uint32_t>(r.McCoordTrunc()));
		// ret->ids.Add(static_cast<uint32_t>(r.ForceDegamma()));
		// ret->ids.Add(r.AnisoBias());
		// ret->ids.Add(static_cast<uint32_t>(r.TruncCoord()));
		// ret->ids.Add(static_cast<uint32_t>(r.DisableCubeWrap()));
		// ret->ids.Add(r.FilterMode());
		// ret->ids.Add(r.MinLod());
		// ret->ids.Add(r.MaxLod());
		// ret->ids.Add(r.PerfMip());
		// ret->ids.Add(r.PerfZ());
		// ret->ids.Add(r.LodBias());
		// ret->ids.Add(r.LodBiasSec());
		// ret->ids.Add(r.XyMagFilter());
		// ret->ids.Add(r.XyMinFilter());
		// ret->ids.Add(r.ZFilter());
		// ret->ids.Add(r.MipFilter());
		// ret->ids.Add(r.BorderColorPtr());
		// ret->ids.Add(r.BorderColorType());
		ret->ids.Add(bind.samplers.slots[i]);
		ret->ids.Add(bind.samplers.start_register[i]);
		ret->ids.Add(static_cast<uint32_t>(bind.samplers.extended[i]));
	}

	ret->ids.Add(bind.gds_pointers.pointers_num);

	for (int i = 0; i < bind.gds_pointers.pointers_num; i++)
	{
		// const auto& r = bind.gds_pointers.pointers[i];

		ret->ids.Add(bind.gds_pointers.slots[i]);
		ret->ids.Add(bind.gds_pointers.start_register[i]);
		ret->ids.Add(static_cast<uint32_t>(bind.gds_pointers.extended[i]));
	}

	ret->ids.Add(bind.direct_sgprs.sgprs_num);

	for (int i = 0; i < bind.direct_sgprs.sgprs_num; i++)
	{
		ret->ids.Add(bind.direct_sgprs.start_register[i]);
	}

	ret->ids.Add(static_cast<uint32_t>(bind.extended.used));
	ret->ids.Add(bind.extended.slot);
	ret->ids.Add(bind.extended.start_register);
}

ShaderId ShaderGetIdVS(const HW::VertexShaderInfo* regs, const ShaderVertexInputInfo* input_info)
{
	KYTY_PROFILER_FUNCTION();

	ShaderId ret;

	if (regs->vs_embedded)
	{
		ret.ids.Add(regs->vs_embedded_id);
		return ret;
	}

	ret.ids.Expand(64);

	bool gs_instead_of_vs =
	    (regs->vs_regs.data_addr == 0 && regs->gs_regs.data_addr == 0 && regs->es_regs.data_addr != 0 && regs->gs_regs.chksum != 0);
	uint64_t shader_addr = (gs_instead_of_vs ? regs->es_regs.data_addr : regs->vs_regs.data_addr);

	bool gen5 = Config::IsNextGen();

	if (gen5)
	{
		EXIT_NOT_IMPLEMENTED(!gs_instead_of_vs);

		ret.hash0 = (regs->gs_regs.chksum >> 32u) & 0xffffffffu;
		ret.crc32 = regs->gs_regs.chksum & 0xffffffffu;
	} else
	{
		const auto* src = reinterpret_cast<const uint32_t*>(shader_addr);

		EXIT_NOT_IMPLEMENTED(src == nullptr);

		const auto* header = GetBinaryInfo(src);

		EXIT_NOT_IMPLEMENTED(header == nullptr);

		ret.hash0 = header->hash0;
		ret.crc32 = header->crc32;
		ret.ids.Add(header->length);
	}

	ret.ids.Add(static_cast<uint32_t>(input_info->fetch_external));
	ret.ids.Add(static_cast<uint32_t>(input_info->fetch_embedded));
	ret.ids.Add(static_cast<uint32_t>(input_info->fetch_inline));
	ret.ids.Add(input_info->resources_num);
	ret.ids.Add(input_info->export_count);

	for (int i = 0; i < input_info->resources_num; i++)
	{
		const auto& r  = input_info->resources[i];
		const auto& rd = input_info->resources_dst[i];

		ret.ids.Add(rd.register_start);
		ret.ids.Add(rd.registers_num);
		ret.ids.Add(r.Stride());
		ret.ids.Add(static_cast<uint32_t>(r.SwizzleEnabled()));
		ret.ids.Add(r.DstSelX());
		ret.ids.Add(r.DstSelY());
		ret.ids.Add(r.DstSelZ());
		ret.ids.Add(r.DstSelW());
		if (gen5)
		{
			ret.ids.Add(r.Format());
			ret.ids.Add(r.OutOfBounds());
		} else
		{
			ret.ids.Add(r.Nfmt());
			ret.ids.Add(r.Dfmt());
		}
		ret.ids.Add(static_cast<uint32_t>(r.AddTid()));
	}

	ret.ids.Add(input_info->buffers_num);

	for (int i = 0; i < input_info->buffers_num; i++)
	{
		const auto& r = input_info->buffers[i];
		ret.ids.Add(r.attr_num);
		ret.ids.Add(r.stride);
		for (int j = 0; j < r.attr_num; j++)
		{
			ret.ids.Add(r.attr_indices[j]);
			ret.ids.Add(r.attr_offsets[j]);
		}
	}

	ShaderGetBindIds(&ret, input_info->bind);

	return ret;
}

ShaderId ShaderGetIdPS(const HW::PixelShaderInfo* regs, const ShaderPixelInputInfo* input_info)
{
	KYTY_PROFILER_FUNCTION();

	ShaderId ret;

	if (regs->ps_embedded)
	{
		ret.ids.Add(regs->ps_embedded_id);
		return ret;
	}

	ret.ids.Expand(64);

	if (Config::IsNextGen())
	{
		ret.hash0 = (regs->ps_regs.chksum >> 32u) & 0xffffffffu;
		ret.crc32 = regs->ps_regs.chksum & 0xffffffffu;
	} else
	{
		const auto* src = reinterpret_cast<const uint32_t*>(regs->ps_regs.data_addr);

		EXIT_NOT_IMPLEMENTED(src == nullptr);

		const auto* header = GetBinaryInfo(src);

		EXIT_NOT_IMPLEMENTED(header == nullptr);

		ret.hash0 = header->hash0;
		ret.crc32 = header->crc32;

		ret.ids.Add(header->length);
	}

	ret.ids.Add(input_info->input_num);
	ret.ids.Add(static_cast<uint32_t>(input_info->ps_pos_xy));
	ret.ids.Add(static_cast<uint32_t>(input_info->ps_pixel_kill_enable));
	ret.ids.Add(static_cast<uint32_t>(input_info->ps_early_z));
	ret.ids.Add(static_cast<uint32_t>(input_info->ps_execute_on_noop));

	for (uint32_t i = 0; i < input_info->input_num; i++)
	{
		ret.ids.Add(input_info->interpolator_settings[i]);
	}

	ShaderGetBindIds(&ret, input_info->bind);

	return ret;
}

ShaderId ShaderGetIdCS(const HW::ComputeShaderInfo* regs, const ShaderComputeInputInfo* input_info)
{
	const auto* src = reinterpret_cast<const uint32_t*>(regs->cs_regs.data_addr);

	EXIT_NOT_IMPLEMENTED(src == nullptr);

	const auto* header = GetBinaryInfo(src);

	EXIT_NOT_IMPLEMENTED(header == nullptr);

	ShaderId ret;
	ret.ids.Expand(64);

	ret.hash0 = header->hash0;
	ret.crc32 = header->crc32;

	ret.ids.Add(header->length);

	ret.ids.Add(input_info->workgroup_register);
	ret.ids.Add(input_info->thread_ids_num);

	for (int i = 0; i < 3; i++)
	{
		ret.ids.Add(input_info->threads_num[i]);
		ret.ids.Add(static_cast<uint32_t>(input_info->group_id[i]));
	}

	ShaderGetBindIds(&ret, input_info->bind);

	return ret;
}

bool ShaderIsDisabled(uint64_t addr)
{
	const auto* src = reinterpret_cast<const uint32_t*>(addr);
	EXIT_NOT_IMPLEMENTED(src == nullptr);

	const auto* header = GetBinaryInfo(src);
	EXIT_NOT_IMPLEMENTED(header == nullptr);

	auto id = (static_cast<uint64_t>(header->hash0) << 32u) | header->crc32;

	bool disabled = (g_disabled_shaders != nullptr && g_disabled_shaders->Contains(id));

	printf("Shader 0x%016" PRIx64 ": id = 0x%016" PRIx64 " - %s\n", addr, id, (disabled ? "disabled" : "enabled"));

	return disabled;
}

bool ShaderIsDisabled2(uint64_t addr, uint64_t chksum)
{
	bool disabled = (g_disabled_shaders != nullptr && g_disabled_shaders->Contains(chksum));

	printf("Shader 0x%016" PRIx64 ": id = 0x%016" PRIx64 " - %s\n", addr, chksum, (disabled ? "disabled" : "enabled"));

	return disabled;
}

void ShaderDisable(uint64_t id)
{
	if (g_disabled_shaders == nullptr)
	{
		g_disabled_shaders = new Vector<uint64_t>;
	}

	if (!g_disabled_shaders->Contains(id))
	{
		g_disabled_shaders->Add(id);
	}
}

void ShaderInjectDebugPrintf(uint64_t id, const ShaderDebugPrintf& cmd)
{
	if (g_debug_printfs == nullptr)
	{
		g_debug_printfs = new Vector<ShaderDebugPrintfCmds>;
	}

	for (auto& c: *g_debug_printfs)
	{
		if (c.id == id)
		{
			c.cmds.Add(cmd);
			return;
		}
	}

	ShaderDebugPrintfCmds c;
	c.id = id;
	c.cmds.Add(cmd);

	g_debug_printfs->Add(c);
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
