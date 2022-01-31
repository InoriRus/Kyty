#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_SHADER_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_SHADER_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Common.h"

#include <algorithm>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct VertexShaderInfo;
struct PixelShaderInfo;
struct ComputeShaderInfo;

enum class ShaderType
{
	Unknown,
	Vertex,
	Pixel,
	Fetch,
	Compute
};

enum class ShaderInstructionType
{
	Unknown,
	BufferLoadDword,
	BufferLoadFormatX,
	BufferLoadFormatXy,
	BufferLoadFormatXyz,
	BufferLoadFormatXyzw,
	BufferStoreDword,
	BufferStoreFormatX,
	BufferStoreFormatXy,
	DsAppend,
	DsConsume,
	Exp,
	ImageLoad,
	ImageSample,
	SAddcU32,
	SAddI32,
	SAddU32,
	SAndB32,
	SAndB64,
	SAndn2B64,
	SAndSaveexecB64,
	SBfmB32,
	SBufferLoadDword,
	SBufferLoadDwordx16,
	SBufferLoadDwordx2,
	SBufferLoadDwordx4,
	SBufferLoadDwordx8,
	SCbranchExecz,
	SCbranchScc0,
	SCmpEqI32,
	SCmpEqU32,
	SCmpGeI32,
	SCmpGeU32,
	SCmpGtI32,
	SCmpGtU32,
	SCmpLeI32,
	SCmpLeU32,
	SCmpLgI32,
	SCmpLgU32,
	SCmpLtI32,
	SCmpLtU32,
	SCselectB32,
	SCselectB64,
	SEndpgm,
	SLoadDwordx4,
	SLoadDwordx8,
	SLshlB32,
	SLshrB32,
	SMovB32,
	SMovB64,
	SMovkI32,
	SMulI32,
	SNandB64,
	SNorB64,
	SOrB64,
	SOrn2B64,
	SSetpcB64,
	SSwappcB64,
	SWaitcnt,
	SWqmB64,
	SXnorB64,
	SXorB64,
	TBufferLoadFormatXyzw,
	VAddI32,
	VAndB32,
	VAshrI32,
	VAshrrevI32,
	VBcntU32B32,
	VBfeU32,
	VBfmB32,
	VBfrevB32,
	VCeilF32,
	VCmpEqF32,
	VCmpEqI32,
	VCmpEqU32,
	VCmpFF32,
	VCmpFI32,
	VCmpFU32,
	VCmpGeF32,
	VCmpGeI32,
	VCmpGeU32,
	VCmpGtF32,
	VCmpGtI32,
	VCmpGtU32,
	VCmpLeF32,
	VCmpLeI32,
	VCmpLeU32,
	VCmpLgF32,
	VCmpLtF32,
	VCmpLtI32,
	VCmpLtU32,
	VCmpNeI32,
	VCmpNeqF32,
	VCmpNeU32,
	VCmpNgeF32,
	VCmpNgtF32,
	VCmpNleF32,
	VCmpNlgF32,
	VCmpNltF32,
	VCmpOF32,
	VCmpTI32,
	VCmpTruF32,
	VCmpTU32,
	VCmpUF32,
	VCmpxEqU32,
	VCmpxGeU32,
	VCmpxGtU32,
	VCmpxNeU32,
	VCndmaskB32,
	VCosF32,
	VCvtF32F16,
	VCvtF32I32,
	VCvtF32U32,
	VCvtF32Ubyte0,
	VCvtF32Ubyte1,
	VCvtF32Ubyte2,
	VCvtF32Ubyte3,
	VCvtPkrtzF16F32,
	VCvtU32F32,
	VExpF32,
	VFloorF32,
	VFractF32,
	VInterpP1F32,
	VInterpP2F32,
	VLshlB32,
	VLshlrevB32,
	VLshrB32,
	VLshrrevB32,
	VMacF32,
	VMadakF32,
	VMadF32,
	VMadmkF32,
	VMadU32U24,
	VMaxF32,
	VMbcntHiU32B32,
	VMbcntLoU32B32,
	VMinF32,
	VMovB32,
	VMulF32,
	VMulHiU32,
	VMulLoI32,
	VMulLoU32,
	VMulU32U24,
	VNotB32,
	VOrB32,
	VRcpF32,
	VRndneF32,
	VRsqF32,
	VSadU32,
	VSqrtF32,
	VSubF32,
	VSubI32,
	VSubrevF32,
	VSubrevI32,
	VTruncF32,
	VXorB32,
	ZMax
};

namespace ShaderInstructionFormat {

enum FormatByte : uint64_t
{
	U = 0,
	N,
	D,      // operand_to_str(inst.dst)
	D2,     // operand_to_str(inst.dst2)
	S0,     // operand_to_str(inst.src[0])
	S1,     // operand_to_str(inst.src[1])
	S2,     // operand_to_str(inst.src[2])
	S3,     // operand_to_str(inst.src[3])
	DA2,    // operand_array_to_str(inst.dst, 2)
	DA3,    // operand_array_to_str(inst.dst, 3)
	DA4,    // operand_array_to_str(inst.dst, 4)
	DA8,    // operand_array_to_str(inst.dst, 8)
	DA16,   // operand_array_to_str(inst.dst, 16)
	D2A2,   // operand_array_to_str(inst.dst2, 2)
	D2A3,   // operand_array_to_str(inst.dst2, 3)
	D2A4,   // operand_array_to_str(inst.dst2, 4)
	S0A2,   // operand_array_to_str(inst.src[0], 2)
	S0A3,   // operand_array_to_str(inst.src[0], 3)
	S0A4,   // operand_array_to_str(inst.src[0], 4)
	S1A2,   // operand_array_to_str(inst.src[1], 2)
	S1A3,   // operand_array_to_str(inst.src[1], 3)
	S1A4,   // operand_array_to_str(inst.src[1], 4)
	S1A8,   // operand_array_to_str(inst.src[1], 8)
	S2A2,   // operand_array_to_str(inst.src[2], 2)
	S2A3,   // operand_array_to_str(inst.src[2], 3)
	S2A4,   // operand_array_to_str(inst.src[2], 4)
	Attr,   // attr%u.%u <- inst.src[1].constant.u, inst.src[2].constant.u
	Idxen,  // idxen
	Offen,  // offen
	Float4, // format:float4
	Pos0,   // pos0
	Done,   // done
	Param0, // param0
	Param1, // param1
	Param2, // param2
	Param3, // param3
	Mrt0,   // mrt_color0
	Off,    // off
	Compr,  // compr
	Vm,     // vm
	L,      // label_%u
	DmaskF, // dmask:0xf
	Dmask7, // dmask:0x7
	Gds,    // gds
};

constexpr uint64_t FormatDefine(std::initializer_list<uint64_t> f)
{
	uint64_t r = 0;
	for (auto n: f)
	{
		r = (r << 8u) | n;
	}
	return r;
}

enum Format : uint64_t
{
	Unknown                             = FormatDefine({U}),
	Empty                               = FormatDefine({N}),
	Imm                                 = FormatDefine({S0}),
	Label                               = FormatDefine({L}),
	Mrt0OffOffComprVmDone               = FormatDefine({Mrt0, Off, Off, Compr, Vm, Done}),
	Mrt0Vsrc0Vsrc1ComprVmDone           = FormatDefine({Mrt0, S0, S1, Compr, Vm, Done}),
	Mrt0Vsrc0Vsrc1Vsrc2Vsrc3VmDone      = FormatDefine({Mrt0, S0, S1, S2, S3, Vm, Done}),
	Param0Vsrc0Vsrc1Vsrc2Vsrc3          = FormatDefine({Param0, S0, S1, S2, S3}),
	Param1Vsrc0Vsrc1Vsrc2Vsrc3          = FormatDefine({Param1, S0, S1, S2, S3}),
	Param2Vsrc0Vsrc1Vsrc2Vsrc3          = FormatDefine({Param2, S0, S1, S2, S3}),
	Param3Vsrc0Vsrc1Vsrc2Vsrc3          = FormatDefine({Param3, S0, S1, S2, S3}),
	Pos0Vsrc0Vsrc1Vsrc2Vsrc3Done        = FormatDefine({Pos0, S0, S1, S2, S3, Done}),
	Saddr                               = FormatDefine({S0A2}),
	Sdst4SbaseSoffset                   = FormatDefine({DA4, S0A2, S1}),
	Sdst8SbaseSoffset                   = FormatDefine({DA8, S0A2, S1}),
	SdstSvSoffset                       = FormatDefine({D, S0A4, S1}),
	Sdst2SvSoffset                      = FormatDefine({DA2, S0A4, S1}),
	Sdst4SvSoffset                      = FormatDefine({DA4, S0A4, S1}),
	Sdst8SvSoffset                      = FormatDefine({DA8, S0A4, S1}),
	Sdst16SvSoffset                     = FormatDefine({DA16, S0A4, S1}),
	SVdstSVsrc0                         = FormatDefine({D, S0}),
	SVdstSVsrc0SVsrc1                   = FormatDefine({D, S0, S1}),
	Sdst2Ssrc02                         = FormatDefine({DA2, S0A2}),
	Sdst2Ssrc02Ssrc12                   = FormatDefine({DA2, S0A2, S1A2}),
	SmaskVsrc0Vsrc1                     = FormatDefine({DA2, S0, S1}),
	Ssrc0Ssrc1                          = FormatDefine({S0, S1}),
	Vdata1VaddrSvSoffsIdxen             = FormatDefine({D, S0, S1A4, S2, Idxen}),
	Vdata2VaddrSvSoffsIdxen             = FormatDefine({DA2, S0, S1A4, S2, Idxen}),
	Vdata3VaddrSvSoffsIdxen             = FormatDefine({DA3, S0, S1A4, S2, Idxen}),
	Vdata4VaddrSvSoffsIdxen             = FormatDefine({DA4, S0, S1A4, S2, Idxen}),
	Vdata4VaddrSvSoffsIdxenFloat4       = FormatDefine({DA4, S0, S1A4, S2, Idxen, Float4}),
	Vdata4Vaddr2SvSoffsOffenIdxenFloat4 = FormatDefine({DA4, S0A2, S1A4, S2, Offen, Idxen, Float4}),
	Vdata3Vaddr3StSsDmask7              = FormatDefine({DA3, S0A3, S1A8, S2A4, Dmask7}),
	Vdata4Vaddr3StSsDmaskF              = FormatDefine({DA4, S0A3, S1A8, S2A4, DmaskF}),
	Vdata4Vaddr3StDmaskF                = FormatDefine({DA4, S0A3, S1A8, DmaskF}),
	VdstVsrc0Vsrc1Smask2                = FormatDefine({D, S0, S1, S2A2}),
	VdstVsrc0Vsrc1Vsrc2                 = FormatDefine({D, S0, S1, S2}),
	VdstVsrcAttrChan                    = FormatDefine({D, S0, Attr}),
	VdstSdst2Vsrc0Vsrc1                 = FormatDefine({D, D2A2, S0, S1}),
	VdstGds                             = FormatDefine({D, Gds}),
};

} // namespace ShaderInstructionFormat

enum class ShaderOperandType
{
	Unknown,
	LiteralConstant,
	IntegerInlineConstant,
	FloatInlineConstant,
	VccLo,
	VccHi,
	ExecLo,
	ExecHi,
	ExecZ,
	Scc,
	Vgpr,
	Sgpr,
	M0
};

union ShaderConstant
{
	int32_t  i;
	uint32_t u;
	float    f;
};

struct ShaderOperand
{
	ShaderOperandType type        = ShaderOperandType::Unknown;
	ShaderConstant    constant    = {0};
	int               register_id = 0;
	int               size        = 0;
	float             multiplier  = 1.0f;
	bool              negate      = false;
	bool              clamp       = false;

	bool operator==(const ShaderOperand& other) const
	{
		return type == other.type && constant.u == other.constant.u && register_id == other.register_id && size == other.size;
	}
};

struct ShaderInstruction
{
	uint32_t                        pc     = 0;
	ShaderInstructionType           type   = ShaderInstructionType::Unknown;
	ShaderInstructionFormat::Format format = ShaderInstructionFormat::Unknown;
	ShaderOperand                   src[4];
	int                             src_num = 0;
	ShaderOperand                   dst;
	ShaderOperand                   dst2;
};

struct ShaderLabel
{
	uint32_t dst;
	uint32_t src;
};

struct ShaderDebugPrintf
{
	enum class Type
	{
		Uint,
		Int,
		Float
	};
	uint32_t              pc = 0;
	String                format;
	Vector<Type>          types;
	Vector<ShaderOperand> args;
};

class ShaderCode
{
public:
	ShaderCode() { m_instructions.Expand(128); };
	virtual ~ShaderCode() = default;
	KYTY_CLASS_DEFAULT_COPY(ShaderCode);

	[[nodiscard]] const Vector<ShaderInstruction>& GetInstructions() const { return m_instructions; }
	Vector<ShaderInstruction>&                     GetInstructions() { return m_instructions; }
	[[nodiscard]] const Vector<ShaderLabel>&       GetLabels() const { return m_labels; }
	Vector<ShaderLabel>&                           GetLabels() { return m_labels; }
	[[nodiscard]] const Vector<ShaderDebugPrintf>& GetDebugPrintfs() const { return m_debug_printfs; }
	Vector<ShaderDebugPrintf>&                     GetDebugPrintfs() { return m_debug_printfs; }

	[[nodiscard]] String DbgDump() const;

	static String DbgInstructionToStr(const ShaderInstruction& inst);

	[[nodiscard]] ShaderType GetType() const { return m_type; }
	void                     SetType(ShaderType type) { this->m_type = type; }

	[[nodiscard]] bool HasAnyOf(std::initializer_list<ShaderInstructionType> types) const
	{
		return std::any_of(types.begin(), types.end(),
		                   [this](auto type)
		                   { return m_instructions.Contains(type, [](auto inst, auto type) { return inst.type == type; }); });
	}

	[[nodiscard]] bool     IsEmbedded() const { return m_embedded; }
	void                   SetEmbedded(bool embedded) { this->m_embedded = embedded; }
	[[nodiscard]] uint32_t GetEmbeddedId() const { return m_embedded_id; }
	void                   SetEmbeddedId(uint32_t embedded_id) { m_embedded_id = embedded_id; }

private:
	Vector<ShaderInstruction> m_instructions;
	Vector<ShaderLabel>       m_labels;
	ShaderType                m_type = ShaderType::Unknown;
	Vector<ShaderDebugPrintf> m_debug_printfs;
	uint32_t                  m_embedded_id = 0;
	bool                      m_embedded    = false;
};

struct ShaderId
{
	Vector<uint32_t> ids;

	bool operator==(const ShaderId& other) const { return ids == other.ids; }
	bool operator!=(const ShaderId& other) const { return !(*this == other); }
};

struct ShaderBufferResource
{
	uint32_t fields[4] = {0};

	void UpdateAddress(uint64_t gpu_addr)
	{
		auto lo   = static_cast<uint32_t>(gpu_addr & 0xffffffffu);
		auto hi   = static_cast<uint32_t>(gpu_addr >> 32u);
		fields[0] = lo;
		fields[1] = (fields[1] & 0xfffff000u) | (hi & 0xfffu);
	}

	[[nodiscard]] uint64_t Base() const { return (fields[0] | (static_cast<uint64_t>(fields[1]) << 32u)) & 0xFFFFFFFFFFFu; }
	[[nodiscard]] uint16_t Stride() const { return (fields[1] >> 16u) & 0x3FFFu; }
	[[nodiscard]] bool     SwizzleEnabled() const { return ((fields[1] >> 31u) & 0x1u) == 1; }
	[[nodiscard]] uint32_t NumRecords() const { return fields[2]; }
	[[nodiscard]] uint8_t  DstSelX() const { return (fields[3] >> 0u) & 0x7u; }
	[[nodiscard]] uint8_t  DstSelY() const { return (fields[3] >> 3u) & 0x7u; }
	[[nodiscard]] uint8_t  DstSelZ() const { return (fields[3] >> 6u) & 0x7u; }
	[[nodiscard]] uint8_t  DstSelW() const { return (fields[3] >> 9u) & 0x7u; }
	[[nodiscard]] uint8_t  Nfmt() const { return (fields[3] >> 12u) & 0x7u; }
	[[nodiscard]] uint8_t  Dfmt() const { return (fields[3] >> 15u) & 0xFu; }
	[[nodiscard]] bool     AddTid() const { return ((fields[3] >> 23u) & 0x1u) == 1; }

	[[nodiscard]] uint8_t MemoryType() const
	{
		return ((fields[1] >> 7u) & 0x60u) | ((fields[3] >> 25u) & 0x1cu) | ((fields[1] >> 14u) & 0x3u);
	}
};

struct ShaderTextureResource
{
	uint32_t fields[8] = {0};

	void UpdateAddress(uint64_t gpu_addr)
	{
		auto lo   = static_cast<uint32_t>(gpu_addr & 0xffffffffu);
		auto hi   = static_cast<uint32_t>(gpu_addr >> 32u);
		fields[0] = lo;
		fields[1] = (fields[1] & 0xffffffc0u) | (hi & 0x3fu);
	}

	[[nodiscard]] uint64_t Base() const { return ((fields[0] | (static_cast<uint64_t>(fields[1]) << 32u)) & 0x3FFFFFFFFFu) << 8u; }
	[[nodiscard]] uint16_t MinLod() const { return (fields[1] >> 8u) & 0xFFFu; }
	[[nodiscard]] uint8_t  Dfmt() const { return (fields[1] >> 20u) & 0x3Fu; }
	[[nodiscard]] uint8_t  Nfmt() const { return (fields[1] >> 26u) & 0xFu; }
	[[nodiscard]] uint16_t Width() const { return (fields[2] >> 0u) & 0x3FFFu; }
	[[nodiscard]] uint16_t Height() const { return (fields[2] >> 14u) & 0x3FFFu; }
	[[nodiscard]] uint8_t  PerfMod() const { return (fields[2] >> 28u) & 0x7u; }
	[[nodiscard]] bool     Interlaced() const { return ((fields[2] >> 31u) & 0x1u) == 1; }
	[[nodiscard]] uint8_t  DstSelX() const { return (fields[3] >> 0u) & 0x7u; }
	[[nodiscard]] uint8_t  DstSelY() const { return (fields[3] >> 3u) & 0x7u; }
	[[nodiscard]] uint8_t  DstSelZ() const { return (fields[3] >> 6u) & 0x7u; }
	[[nodiscard]] uint8_t  DstSelW() const { return (fields[3] >> 9u) & 0x7u; }
	[[nodiscard]] uint8_t  BaseLevel() const { return (fields[3] >> 12u) & 0xFu; }
	[[nodiscard]] uint8_t  LastLevel() const { return (fields[3] >> 16u) & 0xFu; }
	[[nodiscard]] uint8_t  TilingIdx() const { return (fields[3] >> 20u) & 0x1Fu; }
	[[nodiscard]] bool     Pow2Pad() const { return ((fields[3] >> 25u) & 0x1u) == 1; }
	[[nodiscard]] uint8_t  Type() const { return (fields[3] >> 28u) & 0xFu; }

	[[nodiscard]] uint16_t Depth() const { return (fields[4] >> 0u) & 0x1FFFu; }
	[[nodiscard]] uint16_t Pitch() const { return (fields[4] >> 13u) & 0x3FFFu; }
	[[nodiscard]] uint16_t BaseArray() const { return (fields[5] >> 0u) & 0x1FFFu; }
	[[nodiscard]] uint16_t LastArray() const { return (fields[5] >> 13u) & 0x1FFFu; }
	[[nodiscard]] uint16_t MinLodWarn() const { return (fields[6] >> 0u) & 0xFFFu; }
	[[nodiscard]] uint8_t  CounterBankId() const { return (fields[6] >> 12u) & 0xFFu; }
	[[nodiscard]] bool     LodHdwCntEn() const { return ((fields[6] >> 20u) & 0x1u) == 1; }

	[[nodiscard]] uint8_t MemoryType() const
	{
		return ((fields[1] >> 6u) & 0x3u) | ((fields[1] >> 30u) << 2u) | ((fields[3] & 0x04000000u) == 0 ? 0x60u : 0x10u);
	}
};

struct ShaderSamplerResource
{
	uint32_t fields[4] = {0};

	void UpdateIndex(uint32_t index) { fields[0] = index; }

	[[nodiscard]] uint8_t  ClampX() const { return (fields[0] >> 0u) & 0x7u; }
	[[nodiscard]] uint8_t  ClampY() const { return (fields[0] >> 3u) & 0x7u; }
	[[nodiscard]] uint8_t  ClampZ() const { return (fields[0] >> 6u) & 0x7u; }
	[[nodiscard]] uint8_t  MaxAnisoRatio() const { return (fields[0] >> 9u) & 0x7u; }
	[[nodiscard]] uint8_t  DepthCompareFunc() const { return (fields[0] >> 12u) & 0x7u; }
	[[nodiscard]] bool     ForceUnormCoords() const { return ((fields[0] >> 15u) & 0x1u) == 1; }
	[[nodiscard]] uint8_t  AnisoThreshold() const { return (fields[0] >> 16u) & 0x7u; }
	[[nodiscard]] bool     McCoordTrunc() const { return ((fields[0] >> 19u) & 0x1u) == 1; }
	[[nodiscard]] bool     ForceDegamma() const { return ((fields[0] >> 20u) & 0x1u) == 1; }
	[[nodiscard]] uint8_t  AnisoBias() const { return (fields[0] >> 21u) & 0x3Fu; }
	[[nodiscard]] bool     TruncCoord() const { return ((fields[0] >> 27u) & 0x1u) == 1; }
	[[nodiscard]] bool     DisableCubeWrap() const { return ((fields[0] >> 28u) & 0x1u) == 1; }
	[[nodiscard]] uint8_t  FilterMode() const { return (fields[0] >> 29u) & 0x3u; }
	[[nodiscard]] uint16_t MinLod() const { return (fields[1] >> 0u) & 0xFFFu; }
	[[nodiscard]] uint16_t MaxLod() const { return (fields[1] >> 12u) & 0xFFFu; }
	[[nodiscard]] uint8_t  PerfMip() const { return (fields[1] >> 24u) & 0xFu; }
	[[nodiscard]] uint8_t  PerfZ() const { return (fields[1] >> 28u) & 0xFu; }
	[[nodiscard]] uint16_t LodBias() const { return (fields[2] >> 0u) & 0x3FFFu; }
	[[nodiscard]] uint8_t  LodBiasSec() const { return (fields[2] >> 14u) & 0x3Fu; }
	[[nodiscard]] uint8_t  XyMagFilter() const { return (fields[2] >> 20u) & 0x3u; }
	[[nodiscard]] uint8_t  XyMinFilter() const { return (fields[2] >> 22u) & 0x3u; }
	[[nodiscard]] uint8_t  ZFilter() const { return (fields[2] >> 24u) & 0x3u; }
	[[nodiscard]] uint8_t  MipFilter() const { return (fields[2] >> 26u) & 0x3u; }
	[[nodiscard]] uint16_t BorderColorPtr() const { return (fields[3] >> 0u) & 0xFFFu; }
	[[nodiscard]] uint8_t  BorderColorType() const { return (fields[3] >> 30u) & 0x3u; }
};

struct ShaderGdsResource
{
	uint32_t field = 0;

	[[nodiscard]] uint16_t Base() const { return (field >> 16u) & 0xFFFFu; }
	[[nodiscard]] uint16_t Size() const { return field & 0xFFFFu; }
};

struct ShaderExtendedResource
{
	uint32_t fields[2] = {0};

	void UpdateAddress(uint64_t gpu_addr)
	{
		auto lo   = static_cast<uint32_t>(gpu_addr & 0xffffffffu);
		auto hi   = static_cast<uint32_t>(gpu_addr >> 32u);
		fields[0] = lo;
		fields[1] = hi;
	}

	[[nodiscard]] uint64_t Base() const { return (fields[0] | (static_cast<uint64_t>(fields[1]) << 32u)); }
};

struct ShaderVertexInputBuffer
{
	static constexpr int ATTR_MAX = 16;

	uint64_t addr                   = 0;
	uint32_t stride                 = 0;
	uint32_t num_records            = 0;
	int      attr_num               = 0;
	int      attr_indices[ATTR_MAX] = {0};
	uint32_t attr_offsets[ATTR_MAX] = {0};
};

struct ShaderVertexDestination
{
	int register_start = 0;
	int registers_num  = 0;
};

enum class ShaderStorageUsage
{
	Unknown,
	Constant,
	ReadOnly,
	ReadWrite,
};

struct ShaderStorageResources
{
	static constexpr int BUFFERS_MAX = 16;

	ShaderBufferResource buffers[BUFFERS_MAX];
	ShaderStorageUsage   usages[BUFFERS_MAX]         = {};
	int                  slots[BUFFERS_MAX]          = {0};
	int                  start_register[BUFFERS_MAX] = {0};
	bool                 extended[BUFFERS_MAX]       = {};
	int                  buffers_num                 = 0;
	int                  binding_index               = 0;
};

struct ShaderTextureResources
{
	static constexpr int RES_MAX = 16;

	ShaderTextureResource textures[RES_MAX];
	int                   slots[RES_MAX]          = {0};
	int                   start_register[RES_MAX] = {0};
	bool                  extended[RES_MAX]       = {};
	int                   textures_num            = 0;
	int                   binding_index           = 0;
};

struct ShaderSamplerResources
{
	static constexpr int RES_MAX = 16;

	ShaderSamplerResource samplers[RES_MAX];
	int                   slots[RES_MAX]          = {0};
	int                   start_register[RES_MAX] = {0};
	bool                  extended[RES_MAX]       = {};
	int                   samplers_num            = 0;
	int                   binding_index           = 0;
};

struct ShaderGdsResources
{
	static constexpr int POINTERS_MAX = 1;

	ShaderGdsResource pointers[POINTERS_MAX];
	int               slots[POINTERS_MAX]          = {0};
	int               start_register[POINTERS_MAX] = {0};
	bool              extended[POINTERS_MAX]       = {};
	int               pointers_num                 = 0;
	int               binding_index                = 0;
};

struct ShaderExtendedResources
{
	bool                   used           = false;
	int                    slot           = 0;
	int                    start_register = 0;
	ShaderExtendedResource data;
};

struct ShaderBindResources
{
	uint32_t                push_constant_offset = 0;
	uint32_t                push_constant_size   = 0;
	uint32_t                descriptor_set_slot  = 0;
	ShaderStorageResources  storage_buffers;
	ShaderTextureResources  textures2D;
	ShaderSamplerResources  samplers;
	ShaderGdsResources      gds_pointers;
	ShaderExtendedResources extended;
};

struct ShaderBindParameters
{
	bool textures2D_without_sampler = false;
};

struct ShaderVertexInputInfo
{
	static constexpr int RES_MAX = 16;

	ShaderBufferResource    resources[RES_MAX];
	ShaderVertexDestination resources_dst[RES_MAX];
	int                     resources_num = 0;
	bool                    fetch         = false;
	ShaderVertexInputBuffer buffers[RES_MAX];
	int                     buffers_num  = 0;
	int                     export_count = 0;
	ShaderBindResources     bind;
};

struct ShaderComputeInputInfo
{
	uint32_t            threads_num[3]     = {0, 0, 0};
	bool                group_id[3]        = {false, false, false};
	int                 thread_ids_num     = 0;
	int                 workgroup_register = 0;
	ShaderBindResources bind;
};

struct ShaderPixelInputInfo
{
	uint32_t            interpolator_settings[32] = {0};
	uint32_t            input_num                 = 0;
	uint8_t             target_output_mode[8]     = {};
	bool                ps_pos_xy                 = false;
	bool                ps_pixel_kill_enable      = false;
	ShaderBindResources bind;
};

void                 ShaderGetInputInfoVS(const VertexShaderInfo* regs, ShaderVertexInputInfo* info);
void                 ShaderGetInputInfoPS(const PixelShaderInfo* regs, const ShaderVertexInputInfo* vs_info, ShaderPixelInputInfo* ps_info);
void                 ShaderGetInputInfoCS(const ComputeShaderInfo* regs, ShaderComputeInputInfo* info);
void                 ShaderDbgDumpInputInfo(const ShaderVertexInputInfo* info);
void                 ShaderDbgDumpInputInfo(const ShaderPixelInputInfo* info);
void                 ShaderDbgDumpInputInfo(const ShaderComputeInputInfo* info);
ShaderId             ShaderGetIdVS(const VertexShaderInfo* regs, const ShaderVertexInputInfo* input_info);
ShaderId             ShaderGetIdPS(const PixelShaderInfo* regs, const ShaderPixelInputInfo* input_info);
ShaderId             ShaderGetIdCS(const ComputeShaderInfo* regs, const ShaderComputeInputInfo* input_info);
ShaderCode           ShaderParseVS(const VertexShaderInfo* regs);
ShaderCode           ShaderParsePS(const PixelShaderInfo* regs);
ShaderCode           ShaderParseCS(const ComputeShaderInfo* regs);
ShaderBindParameters ShaderGetBindParametersVS(const ShaderCode& code, const ShaderVertexInputInfo* input_info);
ShaderBindParameters ShaderGetBindParametersPS(const ShaderCode& code, const ShaderPixelInputInfo* input_info);
ShaderBindParameters ShaderGetBindParametersCS(const ShaderCode& code, const ShaderComputeInputInfo* input_info);
Vector<uint32_t>     ShaderRecompileVS(const ShaderCode& code, const ShaderVertexInputInfo* input_info);
Vector<uint32_t>     ShaderRecompilePS(const ShaderCode& code, const ShaderPixelInputInfo* input_info);
Vector<uint32_t>     ShaderRecompileCS(const ShaderCode& code, const ShaderComputeInputInfo* input_info);
bool                 ShaderIsDisabled(uint64_t addr);
void                 ShaderDisable(uint64_t id);
void                 ShaderInjectDebugPrintf(uint64_t id, const ShaderDebugPrintf& cmd);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_SHADER_H_ */
