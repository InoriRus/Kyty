#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_HARDWARECONTEXT_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_HARDWARECONTEXT_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct ColorInfo
{
	bool     fmask_compression_enable = false;
	uint32_t fmask_compression_mode   = 0;
	bool     cmask_fast_clear_enable  = false;
	bool     dcc_compression_enable   = false;
	bool     neo_mode                 = false;
	uint32_t cmask_tile_mode          = 0;
	uint32_t cmask_tile_mode_neo      = 0;
	uint32_t format                   = 0;
	uint32_t channel_type             = 0;
	uint32_t channel_order            = 0;
};

struct RenderTarget
{
	uint64_t base_addr               = 0;
	uint32_t pitch_div8_minus1       = 0;
	uint32_t fmask_pitch_div8_minus1 = 0;
	uint32_t slice_div64_minus1      = 0;
	uint32_t base_array_slice_index  = 0;
	uint32_t last_array_slice_index  = 0;

	ColorInfo color_info;

	bool     force_dest_alpha_to_one         = false;
	uint32_t tile_mode                       = 0;
	uint32_t fmask_tile_mode                 = 0;
	uint32_t num_samples                     = 0;
	uint32_t num_fragments                   = 0;
	uint32_t dcc_max_uncompressed_block_size = 0;
	uint32_t dcc_max_compressed_block_size   = 0;
	uint32_t dcc_min_compressed_block_size   = 0;
	uint32_t dcc_color_transform             = 0;
	bool     dcc_enable_overwrite_combiner   = false;
	bool     dcc_force_independent_blocks    = false;
	uint64_t cmask_addr                      = 0;
	uint32_t cmask_slice_minus1              = 0;
	uint64_t fmask_addr                      = 0;
	uint32_t fmask_slice_minus1              = 0;
	uint32_t clear_color_word0               = 0;
	uint32_t clear_color_word1               = 0;
	uint64_t dcc_addr                        = 0;
	uint32_t width                           = 0;
	uint32_t height                          = 0;
};

struct DepthRenderTargetZInfo
{
	uint32_t format              = 0;
	uint32_t tile_mode_index     = 0;
	uint32_t num_samples         = 0;
	bool     tile_surface_enable = false;
	bool     expclear_enabled    = false;
	uint32_t zrange_precision    = 0;
};

struct DepthRenderTargetStencilInfo
{
	uint32_t format               = 0;
	uint32_t tile_mode_index      = 0;
	uint32_t tile_split           = 0;
	bool     expclear_enabled     = false;
	bool     tile_stencil_disable = false;
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

struct DepthRenderTargetDepthView
{
	uint32_t slice_start = 0;
	uint32_t slice_max   = 0;
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
	DepthRenderTargetZInfo        z_info;
	DepthRenderTargetStencilInfo  stencil_info;
	DepthRenderTargetDepthInfo    depth_info;
	DepthRenderTargetDepthView    depth_view;
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
	uint32_t user_clip_planes                    = 0;
	uint32_t user_clip_plane_mode                = 0;
	uint32_t clip_space                          = 0;
	uint32_t vertex_kill_mode                    = 0;
	uint32_t min_z_clip_enable                   = 0;
	uint32_t max_z_clip_enable                   = 0;
	bool     user_clip_plane_negate_y            = false;
	bool     clip_enable                         = false;
	bool     user_clip_plane_cull_only           = false;
	bool     cull_on_clipping_error_disable      = false;
	bool     linear_attribute_clip_enable        = false;
	bool     force_viewport_index_from_vs_enable = false;
};

struct DepthControl
{
	bool    stencil_enable      = false;
	bool    z_enable            = false;
	bool    z_write_enable      = false;
	bool    depth_bounds_enable = false;
	uint8_t zfunc               = 0;
	bool    backface_enable     = false;
	uint8_t stencilfunc         = 0;
	uint8_t stencilfunc_bf      = 0;
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

struct Viewport
{
	float zmin    = 0.0f;
	float zmax    = 0.0f;
	float xscale  = 0.0f;
	float xoffset = 0.0f;
	float yscale  = 0.0f;
	float yoffset = 0.0f;
	float zscale  = 0.0f;
	float zoffset = 0.0f;
};

struct ScreenViewport
{
	Viewport viewports[15];
	uint32_t transform_control       = 0;
	int      scissor_left            = 0;
	int      scissor_top             = 0;
	int      scissor_right           = 0;
	int      scissor_bottom          = 0;
	uint32_t hw_offset_x             = 0;
	uint32_t hw_offset_y             = 0;
	float    guard_band_horz_clip    = 0.0f;
	float    guard_band_vert_clip    = 0.0f;
	float    guard_band_horz_discard = 0.0f;
	float    guard_band_vert_discard = 0.0f;
};

struct VsStageRegisters
{
	uint32_t m_spiShaderPgmLoVs    = 0;
	uint32_t m_spiShaderPgmHiVs    = 0;
	uint32_t m_spiShaderPgmRsrc1Vs = 0;
	uint32_t m_spiShaderPgmRsrc2Vs = 0;

	uint32_t m_spiVsOutConfig     = 0;
	uint32_t m_spiShaderPosFormat = 0;
	uint32_t m_paClVsOutCntl      = 0;

	[[nodiscard]] uint64_t GetGpuAddress() const;
	[[nodiscard]] bool     GetStreamoutEnabled() const;
	[[nodiscard]] uint32_t GetSgprCount() const;
	[[nodiscard]] uint32_t GetInputComponentsCount() const;
	[[nodiscard]] uint32_t GetUnknown1() const;
	[[nodiscard]] uint32_t GetUnknown2() const;
};

struct PsStageRegisters
{
	uint64_t data_addr             = 0;
	uint8_t  vgprs                 = 0;
	uint8_t  sgprs                 = 0;
	uint8_t  scratch_en            = 0;
	uint8_t  user_sgpr             = 0;
	uint8_t  wave_cnt_en           = 0;
	uint32_t shader_z_format       = 0;
	uint8_t  target_output_mode[8] = {};
	uint32_t ps_input_ena          = 0;
	uint32_t ps_input_addr         = 0;
	uint32_t ps_in_control         = 0;
	uint32_t baryc_cntl            = 0;

	uint8_t conservative_z_export_value = 0;
	uint8_t shader_z_behavior           = 0;
	bool    shader_kill_enable          = false;
	bool    shader_z_export_enable      = false;
	bool    shader_execute_on_noop      = false;

	uint32_t m_cbShaderMask = 0;
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

enum class UserSgprType
{
	Unknown,
	Region,
	Vsharp
};

struct UserSgprInfo
{
	uint32_t     value[16] = {0};
	UserSgprType type[16]  = {};
	uint32_t     count     = 0;
};

struct VertexShaderInfo
{
	VsStageRegisters vs_regs;
	uint32_t         vs_shader_modifier = 0;
	uint32_t         vs_embedded_id     = 0;
	UserSgprInfo     vs_user_sgpr;
	bool             vs_embedded = false;
};

struct PixelShaderInfo
{
	PsStageRegisters ps_regs;
	uint32_t         ps_interpolator_settings[32] = {0};
	uint32_t         ps_input_num                 = 0;
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

class HardwareContext
{
public:
	HardwareContext()          = default;
	virtual ~HardwareContext() = default;

	KYTY_CLASS_DEFAULT_COPY(HardwareContext);

	void Reset() { *this = HardwareContext(); }

	void SetRenderTarget(uint32_t slot, const RenderTarget& target) { m_render_targets[slot] = target; }
	void SetColorInfo(uint32_t slot, const ColorInfo& color_info) { m_render_targets[slot].color_info = color_info; }
	[[nodiscard]] const RenderTarget& GetRenderTargets(uint32_t slot) const { return m_render_targets[slot]; }

	void                              SetBlendControl(uint32_t slot, const BlendControl& control) { m_blend_control[slot] = control; }
	[[nodiscard]] const BlendControl& GetBlendControl(uint32_t slot) const { return m_blend_control[slot]; }

	void                   SetRenderTargetMask(uint32_t mask) { m_render_target_mask = mask; }
	[[nodiscard]] uint32_t GetRenderTargetMask() const { return m_render_target_mask; }

	void                   SetShaderStages(uint32_t flags) { m_shader_stages = flags; }
	[[nodiscard]] uint32_t GetShaderStages() const { return m_shader_stages; }

	void                                   SetDepthRenderTarget(const DepthRenderTarget& target) { m_depth_render_target = target; }
	[[nodiscard]] const DepthRenderTarget& GetDepthRenderTarget() const { return m_depth_render_target; }
	void SetDepthRenderTargetZInfo(const DepthRenderTargetZInfo& info) { m_depth_render_target.z_info = info; }
	[[nodiscard]] const DepthRenderTargetZInfo& GetDepthRenderTargetZInfo() const { return m_depth_render_target.z_info; }
	void SetDepthRenderTargetStencilInfo(const DepthRenderTargetStencilInfo& info) { m_depth_render_target.stencil_info = info; }
	[[nodiscard]] const DepthRenderTargetStencilInfo& GetDepthRenderTargetStencilInfo() const { return m_depth_render_target.stencil_info; }

	void SetViewportZ(uint32_t viewport_id, float zmin, float zmax)
	{
		m_screen_viewport.viewports[viewport_id].zmin = zmin;
		m_screen_viewport.viewports[viewport_id].zmax = zmax;
	}
	void SetViewportScaleOffset(uint32_t viewport_id, float xscale, float xoffset, float yscale, float yoffset, float zscale, float zoffset)
	{
		m_screen_viewport.viewports[viewport_id].xscale  = xscale;
		m_screen_viewport.viewports[viewport_id].xoffset = xoffset;
		m_screen_viewport.viewports[viewport_id].yscale  = yscale;
		m_screen_viewport.viewports[viewport_id].yoffset = yoffset;
		m_screen_viewport.viewports[viewport_id].zscale  = zscale;
		m_screen_viewport.viewports[viewport_id].zoffset = zoffset;
	}
	void SetViewportTransformControl(uint32_t control) { m_screen_viewport.transform_control = control; }
	void SetScreenScissor(int left, int top, int right, int bottom)
	{
		m_screen_viewport.scissor_left   = left;
		m_screen_viewport.scissor_top    = top;
		m_screen_viewport.scissor_right  = right;
		m_screen_viewport.scissor_bottom = bottom;
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

	void SetVsShader(const VsStageRegisters* vs_regs, uint32_t shader_modifier)
	{
		m_vs.vs_regs            = *vs_regs;
		m_vs.vs_shader_modifier = shader_modifier;
		m_vs.vs_embedded        = false;
	}
	void SetVsEmbedded(uint32_t id, uint32_t shader_modifier)
	{
		m_vs.vs_embedded_id     = id;
		m_vs.vs_shader_modifier = shader_modifier;
		m_vs.vs_embedded        = true;
	}

	void SetPsShader(const PsStageRegisters* ps_regs)
	{
		m_ps.ps_regs     = *ps_regs;
		m_ps.ps_embedded = false;
	}
	void SetPsEmbedded(uint32_t id)
	{
		m_ps.ps_embedded_id = id;
		m_ps.ps_embedded    = true;
	}

	void SetCsShader(const CsStageRegisters* cs_regs, uint32_t shader_modifier)
	{
		m_cs.cs_regs            = *cs_regs;
		m_cs.cs_shader_modifier = shader_modifier;
	}

	[[nodiscard]] const BlendColor&     GetBlendColor() const { return m_blend_color; }
	void                                SetBlendColor(const BlendColor& color) { m_blend_color = color; }
	[[nodiscard]] const ClipControl&    GetClipControl() const { return m_clip_control; }
	void                                SetClipControl(const ClipControl& control) { m_clip_control = control; }
	[[nodiscard]] const RenderControl&  GetRenderControl() const { return m_render_control; }
	void                                SetRenderControl(const RenderControl& control) { m_render_control = control; }
	[[nodiscard]] const DepthControl&   GetDepthControl() const { return m_depth_control; }
	void                                SetDepthControl(const DepthControl& control) { m_depth_control = control; }
	[[nodiscard]] const ModeControl&    GetModeControl() const { return m_mode_control; }
	void                                SetModeControl(const ModeControl& control) { m_mode_control = control; }
	[[nodiscard]] const EqaaControl&    GetEqaaControl() const { return m_eqaa_control; }
	void                                SetEqaaControl(const EqaaControl& control) { m_eqaa_control = control; }
	[[nodiscard]] const StencilControl& GetStencilControl() const { return m_stencil_control; }
	void                                SetStencilControl(const StencilControl& control) { m_stencil_control = control; }
	[[nodiscard]] const StencilMask&    GetStencilMask() const { return m_stencil_mask; }
	void                                SetStencilMask(const StencilMask& mask) { m_stencil_mask = mask; }

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
	void SetPsInputSettings(uint32_t id, uint32_t value)
	{
		m_ps.ps_interpolator_settings[id] = value;
		m_ps.ps_input_num                 = ((id + 1) > m_ps.ps_input_num ? (id + 1) : m_ps.ps_input_num);
	}

	[[nodiscard]] const PixelShaderInfo&   GetPs() const { return m_ps; }
	[[nodiscard]] const VertexShaderInfo&  GetVs() const { return m_vs; }
	[[nodiscard]] const ComputeShaderInfo& GetCs() const { return m_cs; }

	[[nodiscard]] float   GetDepthClearValue() const { return m_depth_clear_value; }
	void                  SetDepthClearValue(float clear_value) { m_depth_clear_value = clear_value; }
	[[nodiscard]] uint8_t GetStencilClearValue() const { return m_stencil_clear_value; }
	void                  SetStencilClearValue(uint8_t clear_value) { m_stencil_clear_value = clear_value; }

private:
	BlendControl   m_blend_control[8];
	BlendColor     m_blend_color;
	RenderTarget   m_render_targets[8];
	uint32_t       m_render_target_mask = 0;
	ScreenViewport m_screen_viewport;
	ClipControl    m_clip_control;

	VertexShaderInfo  m_vs;
	PixelShaderInfo   m_ps;
	ComputeShaderInfo m_cs;
	uint32_t          m_shader_stages = 0;

	DepthRenderTarget m_depth_render_target;
	RenderControl     m_render_control;
	DepthControl      m_depth_control;
	StencilControl    m_stencil_control;
	StencilMask       m_stencil_mask;
	float             m_depth_clear_value   = 0.0f;
	uint8_t           m_stencil_clear_value = 0;

	ModeControl m_mode_control;
	EqaaControl m_eqaa_control;
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

private:
	uint32_t m_prim_type = 0;
};

inline uint64_t VsStageRegisters::GetGpuAddress() const
{
	return (static_cast<uint64_t>(m_spiShaderPgmLoVs) << 8u) | (static_cast<uint64_t>(m_spiShaderPgmHiVs) << 40u);
}

inline bool VsStageRegisters::GetStreamoutEnabled() const
{
	return (m_spiShaderPgmRsrc2Vs & 0x00001000u) != 0u;
}

inline uint32_t VsStageRegisters::GetSgprCount() const
{
	return (m_spiShaderPgmRsrc1Vs >> 6u) & 0xfu;
}

inline uint32_t VsStageRegisters::GetInputComponentsCount() const
{
	return (m_spiShaderPgmRsrc1Vs >> 24u) & 0x3u;
}

inline uint32_t VsStageRegisters::GetUnknown1() const
{
	return m_spiShaderPgmRsrc1Vs & 0xfcfffc3fu;
}

inline uint32_t VsStageRegisters::GetUnknown2() const
{
	return m_spiShaderPgmRsrc2Vs & 0xFFFFEFFFu;
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_HARDWARECONTEXT_H_ */
