#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_HARDWARECONTEXT_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_HARDWARECONTEXT_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics::HW {

struct ColorBase
{
	uint64_t addr = 0;
};

struct ColorPitch
{
	uint32_t pitch_div8_minus1       = 0;
	uint32_t fmask_pitch_div8_minus1 = 0;
};

struct ColorSlice
{
	uint32_t slice_div64_minus1 = 0;
};

struct ColorView
{
	uint32_t base_array_slice_index = 0;
	uint32_t last_array_slice_index = 0;
	uint32_t current_mip_level      = 0;
};

struct ColorInfo
{
	bool fmask_compression_enable = false;
	// uint32_t fmask_compression_mode   = 0;
	bool     fmask_data_compression_disable = false;
	bool     fmask_one_frag_mode            = false;
	bool     cmask_fast_clear_enable        = false;
	bool     dcc_compression_enable         = false;
	bool     neo_mode                       = false;
	bool     blend_clamp                    = false;
	bool     blend_bypass                   = false;
	bool     round_mode                     = false;
	uint32_t cmask_tile_mode                = 0;
	uint32_t cmask_tile_mode_neo            = 0;
	uint32_t format                         = 0;
	uint32_t channel_type                   = 0;
	uint32_t channel_order                  = 0;
};

struct ColorAttrib
{
	bool     force_dest_alpha_to_one = false;
	uint32_t tile_mode               = 0;
	uint32_t fmask_tile_mode         = 0;
	uint32_t num_samples             = 0;
	uint32_t num_fragments           = 0;
};

struct ColorAttrib2
{
	uint32_t height         = 0;
	uint32_t width          = 0;
	uint32_t num_mip_levels = 0;
};

struct ColorAttrib3
{
	uint32_t depth              = 0;
	uint32_t tile_mode          = 0;
	uint32_t dimension          = 0;
	bool     cmask_pipe_aligned = false;
	bool     dcc_pipe_aligned   = false;
};

struct ColorDccControl
{
	uint32_t max_uncompressed_block_size    = 0;
	uint32_t max_compressed_block_size      = 0;
	uint32_t min_compressed_block_size      = 0;
	uint32_t color_transform                = 0;
	bool     dcc_clear_key_enable           = false;
	bool     overwrite_combiner_disable     = false;
	bool     independent_64b_blocks         = false;
	bool     independent_128b_blocks        = false;
	bool     data_write_on_dcc_clear_to_reg = false;
};

struct ColorCmask
{
	uint64_t addr = 0;
};

struct ColorCmaskSlice
{
	uint32_t slice_minus1 = 0;
};

struct ColorFmask
{
	uint64_t addr = 0;
};

struct ColorFmaskSlice
{
	uint32_t slice_minus1 = 0;
};

struct ColorClearWord0
{
	uint32_t word0 = 0;
};

struct ColorClearWord1
{
	uint32_t word1 = 0;
};

struct ColorDccAddr
{
	uint64_t addr = 0;
};

struct ColorSize
{
	uint32_t width  = 0;
	uint32_t height = 0;
};

struct RenderTarget
{
	ColorBase       base;
	ColorPitch      pitch;
	ColorSlice      slice;
	ColorView       view;
	ColorInfo       info;
	ColorAttrib     attrib;
	ColorAttrib2    attrib2;
	ColorAttrib3    attrib3;
	ColorDccControl dcc;
	ColorCmask      cmask;
	ColorCmaskSlice cmask_slice;
	ColorFmask      fmask;
	ColorFmaskSlice fmask_slice;
	ColorClearWord0 clear_word0;
	ColorClearWord1 clear_word1;
	ColorDccAddr    dcc_addr;
	ColorSize       size;
};

struct DepthZInfo
{
	uint32_t format                    = 0;
	uint32_t tile_mode_index           = 0;
	uint32_t num_samples               = 0;
	uint32_t zrange_precision          = 0;
	bool     tile_surface_enable       = false;
	bool     expclear_enabled          = false;
	bool     embedded_sample_locations = false;
	bool     partially_resident        = false;
	uint8_t  num_mip_levels            = 0;
	uint8_t  plane_compression         = 0;
};

struct DepthStencilInfo
{
	uint32_t format                     = 0;
	uint32_t tile_mode_index            = 0;
	uint32_t tile_split                 = 0;
	bool     expclear_enabled           = false;
	bool     tile_stencil_disable       = false;
	bool     texture_compatible_stencil = false;
	bool     partially_resident         = false;
};

struct DepthRenderTargetDepthInfo
{
	uint32_t addr5_swizzle_mask = 0;
	uint32_t array_mode         = 0;
	uint32_t pipe_config        = 0;
	uint32_t bank_width         = 0;
	uint32_t bank_height        = 0;
	uint32_t macro_tile_aspect  = 0;
	uint32_t num_banks          = 0;
};

struct DepthDepthView
{
	uint32_t slice_start           = 0;
	uint32_t slice_max             = 0;
	uint8_t  current_mip_level     = 0;
	bool     depth_write_disable   = false;
	bool     stencil_write_disable = false;
};

struct DepthDepthSizeXY
{
	uint16_t x_max = 0;
	uint16_t y_max = 0;
};

struct DepthRenderTargetHTileSurface
{
	uint32_t linear                  = 0;
	uint32_t full_cache              = 0;
	uint32_t htile_uses_preload_win  = 0;
	uint32_t preload                 = 0;
	uint32_t prefetch_width          = 0;
	uint32_t prefetch_height         = 0;
	uint32_t dst_outside_zero_to_one = 0;
};

struct DepthRenderTarget
{
	DepthZInfo                    z_info;
	DepthStencilInfo              stencil_info;
	DepthDepthView                depth_view;
	DepthDepthSizeXY              size;
	DepthRenderTargetDepthInfo    depth_info;
	DepthRenderTargetHTileSurface htile_surface;

	uint64_t z_read_base_addr        = 0;
	uint64_t stencil_read_base_addr  = 0;
	uint64_t z_write_base_addr       = 0;
	uint64_t stencil_write_base_addr = 0;
	uint32_t pitch_div8_minus1       = 0;
	uint32_t height_div8_minus1      = 0;
	uint32_t slice_div64_minus1      = 0;
	uint64_t htile_data_base_addr    = 0;
	uint32_t width                   = 0;
	uint32_t height                  = 0;
};

struct RenderControl
{
	bool    depth_clear_enable       = false;
	bool    stencil_clear_enable     = false;
	bool    resummarize_enable       = false;
	bool    stencil_compress_disable = false;
	bool    depth_compress_disable   = false;
	bool    copy_centroid            = false;
	uint8_t copy_sample              = 0;
};

struct ClipControl
{
	uint8_t user_clip_planes                    = 0;
	uint8_t user_clip_plane_mode                = 0;
	bool    dx_clip_space                       = false;
	bool    vertex_kill_any                     = false;
	bool    min_z_clip_disable                  = false;
	bool    max_z_clip_disable                  = false;
	bool    user_clip_plane_negate_y            = false;
	bool    clip_disable                        = false;
	bool    user_clip_plane_cull_only           = false;
	bool    cull_on_clipping_error_disable      = false;
	bool    linear_attribute_clip_enable        = false;
	bool    force_viewport_index_from_vs_enable = false;
};

struct DepthControl
{
	bool    stencil_enable                     = false;
	bool    z_enable                           = false;
	bool    z_write_enable                     = false;
	bool    depth_bounds_enable                = false;
	uint8_t zfunc                              = 0;
	bool    backface_enable                    = false;
	uint8_t stencilfunc                        = 0;
	uint8_t stencilfunc_bf                     = 0;
	bool    color_writes_on_depth_fail_enable  = false;
	bool    color_writes_on_depth_pass_disable = false;
};

struct StencilControl
{
	uint8_t stencil_fail     = 0;
	uint8_t stencil_zpass    = 0;
	uint8_t stencil_zfail    = 0;
	uint8_t stencil_fail_bf  = 0;
	uint8_t stencil_zpass_bf = 0;
	uint8_t stencil_zfail_bf = 0;
};

struct StencilMask
{
	uint8_t stencil_testval      = 0;
	uint8_t stencil_mask         = 0;
	uint8_t stencil_writemask    = 0;
	uint8_t stencil_opval        = 0;
	uint8_t stencil_testval_bf   = 0;
	uint8_t stencil_mask_bf      = 0;
	uint8_t stencil_writemask_bf = 0;
	uint8_t stencil_opval_bf     = 0;
};

struct ModeControl
{
	bool    cull_front               = false;
	bool    cull_back                = false;
	bool    face                     = false;
	uint8_t poly_mode                = 0;
	uint8_t polymode_front_ptype     = 0;
	uint8_t polymode_back_ptype      = 0;
	bool    poly_offset_front_enable = false;
	bool    poly_offset_back_enable  = false;
	bool    vtx_window_offset_enable = false;
	bool    provoking_vtx_last       = false;
	bool    persp_corr_dis           = false;
};

struct BlendControl
{
	uint8_t color_srcblend       = 0;
	uint8_t color_comb_fcn       = 0;
	uint8_t color_destblend      = 0;
	uint8_t alpha_srcblend       = 0;
	uint8_t alpha_comb_fcn       = 0;
	uint8_t alpha_destblend      = 0;
	bool    separate_alpha_blend = false;
	bool    enable               = false;
};

struct BlendColor
{
	float red   = 0.0f;
	float green = 0.0f;
	float blue  = 0.0f;
	float alpha = 0.0f;
};

struct EqaaControl
{
	uint8_t max_anchor_samples         = 0;
	uint8_t ps_iter_samples            = 0;
	uint8_t mask_export_num_samples    = 0;
	uint8_t alpha_to_mask_num_samples  = 0;
	bool    high_quality_intersections = false;
	bool    incoherent_eqaa_reads      = false;
	bool    interpolate_comp_z         = false;
	bool    static_anchor_associations = false;
};

struct ColorControl
{
	uint8_t mode = 1;
	uint8_t op   = 0xCC;
};

struct ScanModeControl
{
	bool msaa_enable          = false;
	bool vport_scissor_enable = true;
	bool line_stipple_enable  = false;
};

struct AaSampleControl
{
	uint64_t centroid_priority = 0;
	uint32_t locations[16]     = {};
};

struct DepthShaderControl
{
	uint32_t other_bits                  = 0;
	uint8_t  conservative_z_export_value = 0;
	uint8_t  shader_z_behavior           = 0;
	bool     shader_kill_enable          = false;
	bool     shader_z_export_enable      = false;
	bool     shader_execute_on_noop      = false;
};

struct AaConfig
{
	uint8_t msaa_num_samples      = 0;
	bool    aa_mask_centroid_dtmn = false;
	uint8_t max_sample_dist       = 0;
	uint8_t msaa_exposed_samples  = 0;
};

struct Viewport
{
	float zmin                                  = 0.0f;
	float zmax                                  = 0.0f;
	float xscale                                = 0.0f;
	float xoffset                               = 0.0f;
	float yscale                                = 0.0f;
	float yoffset                               = 0.0f;
	float zscale                                = 0.0f;
	float zoffset                               = 0.0f;
	int   viewport_scissor_left                 = 0;
	int   viewport_scissor_top                  = 0;
	int   viewport_scissor_right                = 0;
	int   viewport_scissor_bottom               = 0;
	bool  viewport_scissor_window_offset_enable = false;
};

struct ScreenViewport
{
	Viewport viewports[15];
	uint32_t transform_control                    = 1087;
	int      screen_scissor_left                  = 0;
	int      screen_scissor_top                   = 0;
	int      screen_scissor_right                 = 0;
	int      screen_scissor_bottom                = 0;
	int      generic_scissor_left                 = 0;
	int      generic_scissor_top                  = 0;
	int      generic_scissor_right                = 0;
	int      generic_scissor_bottom               = 0;
	bool     generic_scissor_window_offset_enable = false;
	uint32_t hw_offset_x                          = 0;
	uint32_t hw_offset_y                          = 0;
	float    guard_band_horz_clip                 = 0.0f;
	float    guard_band_vert_clip                 = 0.0f;
	float    guard_band_horz_discard              = 0.0f;
	float    guard_band_vert_discard              = 0.0f;
};

struct VsShaderResource1
{
	uint8_t vgprs                    = 0;
	uint8_t sgprs                    = 0;
	uint8_t priority                 = 0;
	uint8_t float_mode               = 0;
	bool    dx10_clamp               = false;
	bool    ieee_mode                = false;
	uint8_t vgpr_component_count     = 0;
	bool    cu_group_enable          = false;
	bool    require_forward_progress = false;
	bool    fp16_overflow            = false;
};

struct VsShaderResource2
{
	bool    scratch_en        = false;
	uint8_t user_sgpr         = 0;
	bool    offchip_lds       = false;
	bool    streamout_enabled = false;
	uint8_t shared_vgprs      = 0;
};

struct VsStageRegisters
{
	uint64_t          data_addr = 0;
	VsShaderResource1 rsrc1;
	VsShaderResource2 rsrc2;
};

struct PsShaderResource1
{
	uint8_t vgprs                    = 0;
	uint8_t sgprs                    = 0;
	uint8_t priority                 = 0;
	uint8_t float_mode               = 0;
	bool    dx10_clamp               = false;
	bool    debug_mode               = false;
	bool    ieee_mode                = false;
	bool    cu_group_disable         = false;
	bool    require_forward_progress = false;
	bool    fp16_overflow            = false;
};

struct PsShaderResource2
{
	bool    scratch_en             = false;
	uint8_t user_sgpr              = 0;
	bool    wave_cnt_en            = false;
	uint8_t extra_lds_size         = 0;
	bool    raster_ordered_shading = false;
	uint8_t shared_vgprs           = 0;
};

struct PsStageRegisters
{
	uint64_t          data_addr = 0;
	PsShaderResource1 rsrc1;
	PsShaderResource2 rsrc2;
	uint64_t          chksum = 0;
};

struct CsStageRegisters
{

	uint64_t data_addr      = 0;
	uint32_t num_thread_x   = 0;
	uint32_t num_thread_y   = 0;
	uint32_t num_thread_z   = 0;
	uint8_t  vgprs          = 0;
	uint8_t  sgprs          = 0;
	uint8_t  bulky          = 0;
	uint8_t  scratch_en     = 0;
	uint8_t  user_sgpr      = 0;
	uint8_t  tgid_x_en      = 0;
	uint8_t  tgid_y_en      = 0;
	uint8_t  tgid_z_en      = 0;
	uint8_t  tg_size_en     = 0;
	uint8_t  tidig_comp_cnt = 0;
	uint8_t  lds_size       = 0;
};

struct EsStageRegisters
{
	uint64_t data_addr = 0;
};

struct GsShaderResource1
{
	uint8_t vgprs                    = 0;
	uint8_t sgprs                    = 0;
	uint8_t priority                 = 0;
	uint8_t float_mode               = 0;
	bool    dx10_clamp               = false;
	bool    debug_mode               = false;
	bool    ieee_mode                = false;
	bool    cu_group_enable          = false;
	bool    require_forward_progress = false;
	bool    lds_configuration        = false;
	uint8_t gs_vgpr_component_count  = 0;
	bool    fp16_overflow            = false;
};

struct GsShaderResource2
{
	bool    scratch_en              = false;
	uint8_t user_sgpr               = 0;
	uint8_t es_vgpr_component_count = 0;
	bool    offchip_lds             = false;
	uint8_t lds_size                = 0;
	uint8_t shared_vgprs            = 0;
};

struct GsStageRegisters
{
	uint64_t          data_addr = 0;
	GsShaderResource1 rsrc1;
	GsShaderResource2 rsrc2;
	uint64_t          chksum = 0;
};

struct ShaderRegisters
{
	uint32_t m_spiVsOutConfig     = 0;
	uint32_t m_spiShaderPosFormat = 0;
	uint32_t m_paClVsOutCntl      = 0;

	uint32_t ps_interpolator_settings[32] = {0};
	// uint32_t ps_input_num                 = 0;

	uint32_t m_spiShaderIdxFormat     = 0;
	uint32_t m_geNggSubgrpCntl        = 0;
	uint32_t m_vgtGsInstanceCnt       = 0;
	uint32_t m_vgtGsOnchipCntl        = 0;
	uint32_t m_geMaxOutputPerSubgroup = 0;
	uint32_t m_vgtEsgsRingItemsize    = 0;
	uint32_t m_vgtGsMaxVertOut        = 0;
	uint32_t m_vgtGsOutPrimType       = 0;

	uint32_t shader_z_format       = 0;
	uint8_t  target_output_mode[8] = {};
	uint32_t ps_input_ena          = 0;
	uint32_t ps_input_addr         = 0;
	uint32_t ps_in_control         = 0;
	uint32_t baryc_cntl            = 0;
	uint32_t m_cbShaderMask        = 0;

	uint32_t m_paScShaderControl = 0;

	DepthShaderControl db_shader_control;

	[[nodiscard]] uint32_t GetExportCount() const { return 1u + ((m_spiVsOutConfig >> 1u) & 0x1Fu); }
	[[nodiscard]] uint32_t GetEsVertsPerSubgrp() const { return (m_vgtGsOnchipCntl >> 0u) & 0x7FFu; }
	[[nodiscard]] uint32_t GetGsPrimsPerSubgrp() const { return (m_vgtGsOnchipCntl >> 11u) & 0x7FFu; }
	[[nodiscard]] uint32_t GetGsInstPrimsInSubgrp() const { return (m_vgtGsOnchipCntl >> 22u) & 0x3FFu; }
};

enum class UserSgprType
{
	Unknown,
	Region,
	Vsharp
};

struct UserSgprInfo
{
	static constexpr int SGPRS_MAX = 16;

	uint32_t     value[SGPRS_MAX] = {0};
	UserSgprType type[SGPRS_MAX]  = {};
	uint32_t     count            = 0;
};

struct VertexShaderInfo
{
	VsStageRegisters vs_regs;
	EsStageRegisters es_regs;
	GsStageRegisters gs_regs;
	uint32_t         vs_shader_modifier = 0;
	uint32_t         vs_embedded_id     = 0;
	UserSgprInfo     vs_user_sgpr;
	UserSgprInfo     gs_user_sgpr;
	bool             vs_embedded = false;
};

struct PixelShaderInfo
{
	PsStageRegisters ps_regs;
	UserSgprInfo     ps_user_sgpr;
	uint32_t         ps_embedded_id = 0;
	bool             ps_embedded    = false;
};

struct ComputeShaderInfo
{
	CsStageRegisters cs_regs;
	uint32_t         cs_shader_modifier = 0;
	UserSgprInfo     cs_user_sgpr;
};

struct GeControl
{
	uint16_t primitive_group_size = 0;
	uint16_t vertex_group_size    = 0;
};

struct GeUserVgprEn
{
	bool vgpr1 = false;
	bool vgpr2 = false;
	bool vgpr3 = false;
};

class Context
{
public:
	Context()          = default;
	virtual ~Context() = default;

	KYTY_CLASS_DEFAULT_COPY(Context);

	void Reset() { *this = Context(); }

	void SetColorBase(uint32_t slot, const ColorBase& base) { m_render_targets[slot].base = base; }
	void SetColorPitch(uint32_t slot, const ColorPitch& pitch) { m_render_targets[slot].pitch = pitch; }
	void SetColorSlice(uint32_t slot, const ColorSlice& slice) { m_render_targets[slot].slice = slice; }
	void SetColorView(uint32_t slot, const ColorView& view) { m_render_targets[slot].view = view; }
	void SetColorInfo(uint32_t slot, const ColorInfo& info) { m_render_targets[slot].info = info; }
	void SetColorAttrib(uint32_t slot, const ColorAttrib& attrib) { m_render_targets[slot].attrib = attrib; }
	void SetColorAttrib2(uint32_t slot, const ColorAttrib2& attrib2) { m_render_targets[slot].attrib2 = attrib2; }
	void SetColorAttrib3(uint32_t slot, const ColorAttrib3& attrib3) { m_render_targets[slot].attrib3 = attrib3; }
	void SetColorDccControl(uint32_t slot, const ColorDccControl& dcc) { m_render_targets[slot].dcc = dcc; }
	void SetColorCmask(uint32_t slot, const ColorCmask& cmask) { m_render_targets[slot].cmask = cmask; }
	void SetColorCmaskSlice(uint32_t slot, const ColorCmaskSlice& cmask_slice) { m_render_targets[slot].cmask_slice = cmask_slice; }
	void SetColorFmask(uint32_t slot, const ColorFmask& fmask) { m_render_targets[slot].fmask = fmask; }
	void SetColorFmaskSlice(uint32_t slot, const ColorFmaskSlice& fmask_slice) { m_render_targets[slot].fmask_slice = fmask_slice; }
	void SetColorClearWord0(uint32_t slot, const ColorClearWord0& clear_word0) { m_render_targets[slot].clear_word0 = clear_word0; }
	void SetColorClearWord1(uint32_t slot, const ColorClearWord1& clear_word1) { m_render_targets[slot].clear_word1 = clear_word1; }
	void SetColorDccAddr(uint32_t slot, const ColorDccAddr& dcc_addr) { m_render_targets[slot].dcc_addr = dcc_addr; }
	void SetColorSize(uint32_t slot, const ColorSize& size) { m_render_targets[slot].size = size; }
	[[nodiscard]] const RenderTarget& GetRenderTarget(uint32_t slot) const { return m_render_targets[slot]; }

	void                              SetBlendControl(uint32_t slot, const BlendControl& control) { m_blend_control[slot] = control; }
	[[nodiscard]] const BlendControl& GetBlendControl(uint32_t slot) const { return m_blend_control[slot]; }

	void                   SetRenderTargetMask(uint32_t mask) { m_render_target_mask = mask; }
	[[nodiscard]] uint32_t GetRenderTargetMask() const { return m_render_target_mask; }

	void                   SetShaderStages(uint32_t flags) { m_shader_stages = flags; }
	[[nodiscard]] uint32_t GetShaderStages() const { return m_shader_stages; }

	void                                   SetDepthRenderTarget(const DepthRenderTarget& target) { m_depth_render_target = target; }
	[[nodiscard]] const DepthRenderTarget& GetDepthRenderTarget() const { return m_depth_render_target; }
	void                                   SetDepthZInfo(const DepthZInfo& info) { m_depth_render_target.z_info = info; }
	[[nodiscard]] const DepthZInfo&        GetDepthZInfo() const { return m_depth_render_target.z_info; }
	void                                   SetDepthStencilInfo(const DepthStencilInfo& info) { m_depth_render_target.stencil_info = info; }
	[[nodiscard]] const DepthStencilInfo&  GetDepthStencilInfo() const { return m_depth_render_target.stencil_info; }
	void                                   SetDepthZReadBase(uint64_t addr) { m_depth_render_target.z_read_base_addr = addr; }
	void                                   SetDepthStencilReadBase(uint64_t addr) { m_depth_render_target.stencil_read_base_addr = addr; }
	void                                   SetDepthZWriteBase(uint64_t addr) { m_depth_render_target.z_write_base_addr = addr; }
	void                                   SetDepthStencilWriteBase(uint64_t addr) { m_depth_render_target.stencil_write_base_addr = addr; }
	void                                   SetDepthHTileDataBase(uint64_t addr) { m_depth_render_target.htile_data_base_addr = addr; }
	void                                   SetDepthDepthView(const DepthDepthView& view) { m_depth_render_target.depth_view = view; }
	[[nodiscard]] const DepthDepthView&    GetDepthDepthView() const { return m_depth_render_target.depth_view; }
	void                                   SetDepthDepthSizeXY(const DepthDepthSizeXY& size) { m_depth_render_target.size = size; }
	[[nodiscard]] const DepthDepthSizeXY&  GetDepthDepthSizeXY() const { return m_depth_render_target.size; }

	void SetViewportZ(uint32_t viewport_id, float zmin, float zmax)
	{
		m_screen_viewport.viewports[viewport_id].zmin = zmin;
		m_screen_viewport.viewports[viewport_id].zmax = zmax;
	}
	void SetViewportZMin(uint32_t viewport_id, float zmin) { m_screen_viewport.viewports[viewport_id].zmin = zmin; }
	void SetViewportZMax(uint32_t viewport_id, float zmax) { m_screen_viewport.viewports[viewport_id].zmax = zmax; }
	void SetViewportScaleOffset(uint32_t viewport_id, float xscale, float xoffset, float yscale, float yoffset, float zscale, float zoffset)
	{
		m_screen_viewport.viewports[viewport_id].xscale  = xscale;
		m_screen_viewport.viewports[viewport_id].xoffset = xoffset;
		m_screen_viewport.viewports[viewport_id].yscale  = yscale;
		m_screen_viewport.viewports[viewport_id].yoffset = yoffset;
		m_screen_viewport.viewports[viewport_id].zscale  = zscale;
		m_screen_viewport.viewports[viewport_id].zoffset = zoffset;
	}
	void SetViewportXScale(uint32_t viewport_id, float xscale) { m_screen_viewport.viewports[viewport_id].xscale = xscale; }
	void SetViewportXOffset(uint32_t viewport_id, float xoffset) { m_screen_viewport.viewports[viewport_id].xoffset = xoffset; }
	void SetViewportYScale(uint32_t viewport_id, float yscale) { m_screen_viewport.viewports[viewport_id].yscale = yscale; }
	void SetViewportYOffset(uint32_t viewport_id, float yoffset) { m_screen_viewport.viewports[viewport_id].yoffset = yoffset; }
	void SetViewportZScale(uint32_t viewport_id, float zscale) { m_screen_viewport.viewports[viewport_id].zscale = zscale; }
	void SetViewportZOffset(uint32_t viewport_id, float zoffset) { m_screen_viewport.viewports[viewport_id].zoffset = zoffset; }
	void SetViewportScissor(uint32_t viewport_id, int left, int top, int right, int bottom, bool window_offset_enable)
	{
		m_screen_viewport.viewports[viewport_id].viewport_scissor_left                 = left;
		m_screen_viewport.viewports[viewport_id].viewport_scissor_top                  = top;
		m_screen_viewport.viewports[viewport_id].viewport_scissor_right                = right;
		m_screen_viewport.viewports[viewport_id].viewport_scissor_bottom               = bottom;
		m_screen_viewport.viewports[viewport_id].viewport_scissor_window_offset_enable = window_offset_enable;
	}
	void SetViewportScissorTL(uint32_t viewport_id, int left, int top, bool window_offset_enable)
	{
		m_screen_viewport.viewports[viewport_id].viewport_scissor_left                 = left;
		m_screen_viewport.viewports[viewport_id].viewport_scissor_top                  = top;
		m_screen_viewport.viewports[viewport_id].viewport_scissor_window_offset_enable = window_offset_enable;
	}
	void SetViewportScissorBR(uint32_t viewport_id, int right, int bottom)
	{
		m_screen_viewport.viewports[viewport_id].viewport_scissor_right  = right;
		m_screen_viewport.viewports[viewport_id].viewport_scissor_bottom = bottom;
	}
	void SetViewportTransformControl(uint32_t control) { m_screen_viewport.transform_control = control; }
	void SetScreenScissor(int left, int top, int right, int bottom)
	{
		m_screen_viewport.screen_scissor_left   = left;
		m_screen_viewport.screen_scissor_top    = top;
		m_screen_viewport.screen_scissor_right  = right;
		m_screen_viewport.screen_scissor_bottom = bottom;
	}
	void SetGenericScissor(int left, int top, int right, int bottom, bool window_offset_enable)
	{
		m_screen_viewport.generic_scissor_left                 = left;
		m_screen_viewport.generic_scissor_top                  = top;
		m_screen_viewport.generic_scissor_right                = right;
		m_screen_viewport.generic_scissor_bottom               = bottom;
		m_screen_viewport.generic_scissor_window_offset_enable = window_offset_enable;
	}
	void SetHardwareScreenOffset(uint32_t offset_x, uint32_t offset_y)
	{
		m_screen_viewport.hw_offset_x = offset_x;
		m_screen_viewport.hw_offset_y = offset_y;
	}
	void SetGuardBands(float horz_clip, float vert_clip, float horz_discard, float vert_discard)
	{
		m_screen_viewport.guard_band_horz_clip    = horz_clip;
		m_screen_viewport.guard_band_vert_clip    = vert_clip;
		m_screen_viewport.guard_band_horz_discard = horz_discard;
		m_screen_viewport.guard_band_vert_discard = vert_discard;
	}
	[[nodiscard]] const ScreenViewport& GetScreenViewport() const { return m_screen_viewport; }

	[[nodiscard]] const BlendColor&      GetBlendColor() const { return m_blend_color; }
	void                                 SetBlendColor(const BlendColor& color) { m_blend_color = color; }
	[[nodiscard]] const ClipControl&     GetClipControl() const { return m_clip_control; }
	void                                 SetClipControl(const ClipControl& control) { m_clip_control = control; }
	[[nodiscard]] const RenderControl&   GetRenderControl() const { return m_render_control; }
	void                                 SetRenderControl(const RenderControl& control) { m_render_control = control; }
	[[nodiscard]] const DepthControl&    GetDepthControl() const { return m_depth_control; }
	void                                 SetDepthControl(const DepthControl& control) { m_depth_control = control; }
	[[nodiscard]] const ModeControl&     GetModeControl() const { return m_mode_control; }
	void                                 SetModeControl(const ModeControl& control) { m_mode_control = control; }
	[[nodiscard]] const EqaaControl&     GetEqaaControl() const { return m_eqaa_control; }
	void                                 SetEqaaControl(const EqaaControl& control) { m_eqaa_control = control; }
	[[nodiscard]] const StencilControl&  GetStencilControl() const { return m_stencil_control; }
	void                                 SetStencilControl(const StencilControl& control) { m_stencil_control = control; }
	[[nodiscard]] const StencilMask&     GetStencilMask() const { return m_stencil_mask; }
	void                                 SetStencilMask(const StencilMask& mask) { m_stencil_mask = mask; }
	[[nodiscard]] const ColorControl&    GetColorControl() const { return m_color_control; }
	void                                 SetColorControl(const ColorControl& control) { m_color_control = control; }
	[[nodiscard]] const ScanModeControl& GetScanModeControl() const { return m_scan_mode_control; }
	void                                 SetScanModeControl(const ScanModeControl& control) { m_scan_mode_control = control; }
	[[nodiscard]] const AaSampleControl& GetAaSampleControl() const { return m_aa_sample_control; }
	void                                 SetAaSampleControl(const AaSampleControl& control) { m_aa_sample_control = control; }
	[[nodiscard]] const AaConfig&        GetAaConfig() const { return m_aa_config; }
	void                                 SetAaConfig(const AaConfig& config) { m_aa_config = config; }

	[[nodiscard]] float   GetDepthClearValue() const { return m_depth_clear_value; }
	void                  SetDepthClearValue(float clear_value) { m_depth_clear_value = clear_value; }
	[[nodiscard]] uint8_t GetStencilClearValue() const { return m_stencil_clear_value; }
	void                  SetStencilClearValue(uint8_t clear_value) { m_stencil_clear_value = clear_value; }

	[[nodiscard]] float GetLineWidth() const { return m_line_width; }
	void                SetLineWidth(float width) { m_line_width = width; }

	[[nodiscard]] const ShaderRegisters& GetShaderRegisters() const { return m_sh_regs; }

	void SetVsOutConfig(uint32_t value) { m_sh_regs.m_spiVsOutConfig = value; }
	void SetShaderPosFormat(uint32_t value) { m_sh_regs.m_spiShaderPosFormat = value; }
	void SetClVsOutCntl(uint32_t value) { m_sh_regs.m_paClVsOutCntl = value; }
	void SetShaderIdxFormat(uint32_t value) { m_sh_regs.m_spiShaderIdxFormat = value; }
	void SetNggSubgrpCntl(uint32_t value) { m_sh_regs.m_geNggSubgrpCntl = value; }
	void SetGsInstanceCnt(uint32_t value) { m_sh_regs.m_vgtGsInstanceCnt = value; }
	void SetGsOnchipCntl(uint32_t value) { m_sh_regs.m_vgtGsOnchipCntl = value; }
	void SetMaxOutputPerSubgroup(uint32_t value) { m_sh_regs.m_geMaxOutputPerSubgroup = value; }
	void SetEsgsRingItemsize(uint32_t value) { m_sh_regs.m_vgtEsgsRingItemsize = value; }
	void SetGsMaxVertOut(uint32_t value) { m_sh_regs.m_vgtGsMaxVertOut = value; }
	void SetGsOutPrimType(uint32_t value) { m_sh_regs.m_vgtGsOutPrimType = value; }

	void SetPsInputSettings(uint32_t id, uint32_t value)
	{
		m_sh_regs.ps_interpolator_settings[id] = value;
		// m_sh_regs.ps_input_num                 = ((id + 1) > m_sh_regs.ps_input_num ? (id + 1) : m_sh_regs.ps_input_num);
	}

	void SetShaderZFormat(uint32_t value) { m_sh_regs.shader_z_format = value; }
	void SetTargetOutputMode(uint32_t slot, uint8_t value) { m_sh_regs.target_output_mode[slot] = value; }
	void SetPsInputEna(uint32_t value) { m_sh_regs.ps_input_ena = value; }
	void SetPsInputAddr(uint32_t value) { m_sh_regs.ps_input_addr = value; }
	void SetPsInControl(uint32_t value) { m_sh_regs.ps_in_control = value; }
	void SetBarycCntl(uint32_t value) { m_sh_regs.baryc_cntl = value; }
	void SetShaderMask(uint32_t value) { m_sh_regs.m_cbShaderMask = value; }
	void SetDepthShaderControl(const DepthShaderControl& value) { m_sh_regs.db_shader_control = value; }

	void SetScShaderControl(uint32_t value) { m_sh_regs.m_paScShaderControl = value; }

private:
	float m_line_width = 1.0f;

	BlendControl    m_blend_control[8];
	BlendColor      m_blend_color;
	RenderTarget    m_render_targets[8];
	uint32_t        m_render_target_mask = 0;
	ScreenViewport  m_screen_viewport;
	ClipControl     m_clip_control;
	ColorControl    m_color_control;
	ScanModeControl m_scan_mode_control;

	AaSampleControl m_aa_sample_control;
	AaConfig        m_aa_config;

	uint32_t m_shader_stages = 0;

	DepthRenderTarget m_depth_render_target;
	RenderControl     m_render_control;
	DepthControl      m_depth_control;
	StencilControl    m_stencil_control;
	StencilMask       m_stencil_mask;
	float             m_depth_clear_value   = 0.0f;
	uint8_t           m_stencil_clear_value = 0;

	ModeControl m_mode_control;
	EqaaControl m_eqaa_control;

	ShaderRegisters m_sh_regs;
};

class UserConfig
{
public:
	UserConfig()          = default;
	virtual ~UserConfig() = default;

	KYTY_CLASS_DEFAULT_COPY(UserConfig);

	void Reset() { *this = UserConfig(); }

	void                   SetPrimitiveType(uint32_t prim_type) { m_prim_type = prim_type; }
	[[nodiscard]] uint32_t GetPrimType() const { return m_prim_type; }

	[[nodiscard]] const GeControl&    GetGeControl() const { return m_ge_cntl; }
	void                              SetGeControl(const GeControl& control) { m_ge_cntl = control; }
	[[nodiscard]] const GeUserVgprEn& GetGeUserVgprEn() const { return m_ge_user_vgpr_en; }
	void                              SetGeUserVgprEn(const GeUserVgprEn& en) { m_ge_user_vgpr_en = en; }

private:
	uint32_t m_prim_type = 0;

	GeControl    m_ge_cntl;
	GeUserVgprEn m_ge_user_vgpr_en;
};

class Shader
{
public:
	Shader()          = default;
	virtual ~Shader() = default;

	KYTY_CLASS_DEFAULT_COPY(Shader);

	void Reset() { *this = Shader(); }

	void SetVsShaderModifier(uint32_t shader_modifier) { m_vs.vs_shader_modifier = shader_modifier; }
	void SetVsShaderBase(uint64_t addr)
	{
		m_vs.vs_regs.data_addr = addr;
		m_vs.vs_embedded       = false;
	}
	void SetVsShaderResource1(const VsShaderResource1& rsrc1)
	{
		m_vs.vs_regs.rsrc1 = rsrc1;
		m_vs.vs_embedded   = false;
	}
	void SetVsShaderResource2(const VsShaderResource2& rsrc2)
	{
		m_vs.vs_regs.rsrc2 = rsrc2;
		m_vs.vs_embedded   = false;
	}

	void SetVsEmbedded(uint32_t id, uint32_t shader_modifier)
	{
		m_vs.vs_embedded_id     = id;
		m_vs.vs_shader_modifier = shader_modifier;
		m_vs.vs_embedded        = true;
	}
	void SetEsShaderBase(uint64_t addr)
	{
		m_vs.es_regs.data_addr = addr;
		m_vs.vs_embedded       = false;
	}
	void SetGsShaderResource1(const GsShaderResource1& rsrc1)
	{
		m_vs.gs_regs.rsrc1 = rsrc1;
		m_vs.vs_embedded   = false;
	}
	void SetGsShaderResource2(const GsShaderResource2& rsrc2)
	{
		m_vs.gs_regs.rsrc2 = rsrc2;
		m_vs.vs_embedded   = false;
	}
	void SetGsShaderChksum(uint32_t value)
	{
		m_vs.gs_regs.chksum <<= 32u;
		m_vs.gs_regs.chksum |= value;
	}

	void SetPsShaderBase(uint64_t addr)
	{
		m_ps.ps_regs.data_addr = addr;
		m_ps.ps_embedded       = false;
	}
	void SetPsShaderResource1(const PsShaderResource1& rsrc1)
	{
		m_ps.ps_regs.rsrc1 = rsrc1;
		m_ps.ps_embedded   = false;
	}
	void SetPsShaderResource2(const PsShaderResource2& rsrc2)
	{
		m_ps.ps_regs.rsrc2 = rsrc2;
		m_ps.ps_embedded   = false;
	}
	void SetPsEmbedded(uint32_t id)
	{
		m_ps.ps_embedded_id = id;
		m_ps.ps_embedded    = true;
	}
	void SetPsShaderChksum(uint32_t value)
	{
		m_ps.ps_regs.chksum <<= 32u;
		m_ps.ps_regs.chksum |= value;
	}

	void SetCsShader(const CsStageRegisters& cs_regs, uint32_t shader_modifier)
	{
		m_cs.cs_regs            = cs_regs;
		m_cs.cs_shader_modifier = shader_modifier;
	}

	void SetVsUserSgpr(uint32_t id, uint32_t value, UserSgprType type)
	{
		m_vs.vs_user_sgpr.value[id] = value;
		m_vs.vs_user_sgpr.type[id]  = type;
		m_vs.vs_user_sgpr.count     = ((id + 1) > m_vs.vs_user_sgpr.count ? (id + 1) : m_vs.vs_user_sgpr.count);
	}
	void SetPsUserSgpr(uint32_t id, uint32_t value, UserSgprType type)
	{
		m_ps.ps_user_sgpr.value[id] = value;
		m_ps.ps_user_sgpr.type[id]  = type;
		m_ps.ps_user_sgpr.count     = ((id + 1) > m_ps.ps_user_sgpr.count ? (id + 1) : m_ps.ps_user_sgpr.count);
	}
	void SetCsUserSgpr(uint32_t id, uint32_t value, UserSgprType type)
	{
		m_cs.cs_user_sgpr.value[id] = value;
		m_cs.cs_user_sgpr.type[id]  = type;
		m_cs.cs_user_sgpr.count     = ((id + 1) > m_cs.cs_user_sgpr.count ? (id + 1) : m_cs.cs_user_sgpr.count);
	}
	void SetGsUserSgpr(uint32_t id, uint32_t value, UserSgprType type)
	{
		m_vs.gs_user_sgpr.value[id] = value;
		m_vs.gs_user_sgpr.type[id]  = type;
		m_vs.gs_user_sgpr.count     = ((id + 1) > m_vs.gs_user_sgpr.count ? (id + 1) : m_vs.gs_user_sgpr.count);
	}

	[[nodiscard]] const PixelShaderInfo&   GetPs() const { return m_ps; }
	[[nodiscard]] const VertexShaderInfo&  GetVs() const { return m_vs; }
	[[nodiscard]] const ComputeShaderInfo& GetCs() const { return m_cs; }

private:
	VertexShaderInfo  m_vs;
	PixelShaderInfo   m_ps;
	ComputeShaderInfo m_cs;
};

} // namespace Kyty::Libs::Graphics::HW

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_HARDWARECONTEXT_H_ */
