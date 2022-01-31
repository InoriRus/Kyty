#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_PM4_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_PM4_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Core {
class File;
} // namespace Kyty::Core

#define KYTY_PM4_GET(u, r, f) (((u) >> Pm4::r##_##f##_SHIFT) & Pm4::r##_##f##_MASK)

#define KYTY_PM4(len, op, r)                                                                                                               \
	(0xC0000000u | (((static_cast<uint16_t>(len) - 2u) & 0x3fffu) << 16u) | (((op)&0xffu) << 8u) | (((r)&0x3fu) << 2u))

namespace Kyty::Libs::Graphics::Pm4 {

constexpr uint32_t IT_NOP                       = 0x10;
constexpr uint32_t IT_SET_BASE                  = 0x11;
constexpr uint32_t IT_CLEAR_STATE               = 0x12;
constexpr uint32_t IT_INDEX_BUFFER_SIZE         = 0x13;
constexpr uint32_t IT_DISPATCH_DIRECT           = 0x15;
constexpr uint32_t IT_DISPATCH_INDIRECT         = 0x16;
constexpr uint32_t IT_SET_PREDICATION           = 0x20;
constexpr uint32_t IT_COND_EXEC                 = 0x22;
constexpr uint32_t IT_DRAW_INDIRECT             = 0x24;
constexpr uint32_t IT_DRAW_INDEX_INDIRECT       = 0x25;
constexpr uint32_t IT_INDEX_BASE                = 0x26;
constexpr uint32_t IT_DRAW_INDEX_2              = 0x27;
constexpr uint32_t IT_CONTEXT_CONTROL           = 0x28;
constexpr uint32_t IT_INDEX_TYPE                = 0x2A;
constexpr uint32_t IT_DRAW_INDIRECT_MULTI       = 0x2C;
constexpr uint32_t IT_DRAW_INDEX_AUTO           = 0x2D;
constexpr uint32_t IT_NUM_INSTANCES             = 0x2F;
constexpr uint32_t IT_INDIRECT_BUFFER_CNST      = 0x33;
constexpr uint32_t IT_DRAW_INDEX_OFFSET_2       = 0x35;
constexpr uint32_t IT_WRITE_DATA                = 0x37;
constexpr uint32_t IT_MEM_SEMAPHORE             = 0x39;
constexpr uint32_t IT_DRAW_INDEX_INDIRECT_MULTI = 0x38;
constexpr uint32_t IT_WAIT_REG_MEM              = 0x3C;
constexpr uint32_t IT_INDIRECT_BUFFER           = 0x3F;
constexpr uint32_t IT_COPY_DATA                 = 0x40;
constexpr uint32_t IT_CP_DMA                    = 0x41;
constexpr uint32_t IT_PFP_SYNC_ME               = 0x42;
constexpr uint32_t IT_SURFACE_SYNC              = 0x43;
constexpr uint32_t IT_EVENT_WRITE               = 0x46;
constexpr uint32_t IT_EVENT_WRITE_EOP           = 0x47;
constexpr uint32_t IT_EVENT_WRITE_EOS           = 0x48;
constexpr uint32_t IT_RELEASE_MEM               = 0x49;
constexpr uint32_t IT_DMA_DATA                  = 0x50;
constexpr uint32_t IT_ACQUIRE_MEM               = 0x58;
constexpr uint32_t IT_REWIND                    = 0x59;
constexpr uint32_t IT_SET_CONFIG_REG            = 0x68;
constexpr uint32_t IT_SET_CONTEXT_REG           = 0x69;
constexpr uint32_t IT_SET_SH_REG                = 0x76;
constexpr uint32_t IT_SET_QUEUE_REG             = 0x78;
constexpr uint32_t IT_SET_UCONFIG_REG           = 0x79;
constexpr uint32_t IT_WRITE_CONST_RAM           = 0x81;
constexpr uint32_t IT_DUMP_CONST_RAM            = 0x83;
constexpr uint32_t IT_INCREMENT_CE_COUNTER      = 0x84;
constexpr uint32_t IT_INCREMENT_DE_COUNTER      = 0x85;
constexpr uint32_t IT_WAIT_ON_CE_COUNTER        = 0x86;
constexpr uint32_t IT_WAIT_ON_DE_COUNTER_DIFF   = 0x88;
constexpr uint32_t IT_DISPATCH_DRAW_PREAMBLE    = 0x8C;
constexpr uint32_t IT_DISPATCH_DRAW             = 0x8D;

constexpr uint32_t R_ZERO              = 0x00;
constexpr uint32_t R_VS                = 0x01;
constexpr uint32_t R_PS                = 0x02;
constexpr uint32_t R_DRAW_INDEX        = 0x03;
constexpr uint32_t R_DRAW_INDEX_AUTO   = 0x04;
constexpr uint32_t R_DRAW_RESET        = 0x05;
constexpr uint32_t R_WAIT_FLIP_DONE    = 0x06;
constexpr uint32_t R_CS                = 0x07;
constexpr uint32_t R_DISPATCH_DIRECT   = 0x08;
constexpr uint32_t R_DISPATCH_RESET    = 0x09;
constexpr uint32_t R_DISPATCH_WAIT_MEM = 0x0A;
constexpr uint32_t R_PUSH_MARKER       = 0x0B;
constexpr uint32_t R_POP_MARKER        = 0x0C;
constexpr uint32_t R_VS_EMBEDDED       = 0x0D;
constexpr uint32_t R_PS_EMBEDDED       = 0x0E;
constexpr uint32_t R_VS_UPDATE         = 0x0F;
constexpr uint32_t R_PS_UPDATE         = 0x10;

constexpr uint32_t DB_RENDER_CONTROL                                = 0x0;
constexpr uint32_t DB_RENDER_CONTROL_DEPTH_CLEAR_ENABLE_SHIFT       = 0;
constexpr uint32_t DB_RENDER_CONTROL_DEPTH_CLEAR_ENABLE_MASK        = 0x1;
constexpr uint32_t DB_RENDER_CONTROL_STENCIL_CLEAR_ENABLE_SHIFT     = 1;
constexpr uint32_t DB_RENDER_CONTROL_STENCIL_CLEAR_ENABLE_MASK      = 0x1;
constexpr uint32_t DB_RENDER_CONTROL_RESUMMARIZE_ENABLE_SHIFT       = 4;
constexpr uint32_t DB_RENDER_CONTROL_RESUMMARIZE_ENABLE_MASK        = 0x1;
constexpr uint32_t DB_RENDER_CONTROL_STENCIL_COMPRESS_DISABLE_SHIFT = 5;
constexpr uint32_t DB_RENDER_CONTROL_STENCIL_COMPRESS_DISABLE_MASK  = 0x1;
constexpr uint32_t DB_RENDER_CONTROL_DEPTH_COMPRESS_DISABLE_SHIFT   = 6;
constexpr uint32_t DB_RENDER_CONTROL_DEPTH_COMPRESS_DISABLE_MASK    = 0x1;
constexpr uint32_t DB_RENDER_CONTROL_COPY_CENTROID_SHIFT            = 7;
constexpr uint32_t DB_RENDER_CONTROL_COPY_CENTROID_MASK             = 0x1;
constexpr uint32_t DB_RENDER_CONTROL_COPY_SAMPLE_SHIFT              = 8;
constexpr uint32_t DB_RENDER_CONTROL_COPY_SAMPLE_MASK               = 0xF;

constexpr uint32_t DB_DEPTH_VIEW                   = 0x2;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_START_SHIFT = 0;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_START_MASK  = 0x7FF;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_MAX_SHIFT   = 13;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_MAX_MASK    = 0x7FF;

constexpr uint32_t DB_HTILE_DATA_BASE                 = 0x5;
constexpr uint32_t DB_HTILE_DATA_BASE_BASE_256B_SHIFT = 0;
constexpr uint32_t DB_HTILE_DATA_BASE_BASE_256B_MASK  = 0xFFFFFFFF;

constexpr uint32_t DB_DEPTH_CLEAR                   = 0xB;
constexpr uint32_t DB_DEPTH_CLEAR_DEPTH_CLEAR_SHIFT = 0;
constexpr uint32_t DB_DEPTH_CLEAR_DEPTH_CLEAR_MASK  = 0xFFFFFFFF;

constexpr uint32_t DB_DEPTH_INFO                          = 0xF;
constexpr uint32_t DB_DEPTH_INFO_ADDR5_SWIZZLE_MASK_SHIFT = 0;
constexpr uint32_t DB_DEPTH_INFO_ADDR5_SWIZZLE_MASK_MASK  = 0xF;
constexpr uint32_t DB_DEPTH_INFO_ARRAY_MODE_SHIFT         = 4;
constexpr uint32_t DB_DEPTH_INFO_ARRAY_MODE_MASK          = 0xF;
constexpr uint32_t DB_DEPTH_INFO_PIPE_CONFIG_SHIFT        = 8;
constexpr uint32_t DB_DEPTH_INFO_PIPE_CONFIG_MASK         = 0x1F;
constexpr uint32_t DB_DEPTH_INFO_BANK_WIDTH_SHIFT         = 13;
constexpr uint32_t DB_DEPTH_INFO_BANK_WIDTH_MASK          = 0x3;
constexpr uint32_t DB_DEPTH_INFO_BANK_HEIGHT_SHIFT        = 15;
constexpr uint32_t DB_DEPTH_INFO_BANK_HEIGHT_MASK         = 0x3;
constexpr uint32_t DB_DEPTH_INFO_MACRO_TILE_ASPECT_SHIFT  = 17;
constexpr uint32_t DB_DEPTH_INFO_MACRO_TILE_ASPECT_MASK   = 0x3;
constexpr uint32_t DB_DEPTH_INFO_NUM_BANKS_SHIFT          = 19;
constexpr uint32_t DB_DEPTH_INFO_NUM_BANKS_MASK           = 0x3;

constexpr uint32_t DB_Z_INFO                           = 0x10;
constexpr uint32_t DB_Z_INFO_FORMAT_SHIFT              = 0;
constexpr uint32_t DB_Z_INFO_FORMAT_MASK               = 0x3;
constexpr uint32_t DB_Z_INFO_NUM_SAMPLES_SHIFT         = 2;
constexpr uint32_t DB_Z_INFO_NUM_SAMPLES_MASK          = 0x3;
constexpr uint32_t DB_Z_INFO_TILE_MODE_INDEX_SHIFT     = 20;
constexpr uint32_t DB_Z_INFO_TILE_MODE_INDEX_MASK      = 0x7;
constexpr uint32_t DB_Z_INFO_TILE_SURFACE_ENABLE_SHIFT = 29;
constexpr uint32_t DB_Z_INFO_TILE_SURFACE_ENABLE_MASK  = 0x1;
constexpr uint32_t DB_Z_INFO_ZRANGE_PRECISION_SHIFT    = 31;
constexpr uint32_t DB_Z_INFO_ZRANGE_PRECISION_MASK     = 0x1;

constexpr uint32_t DB_STENCIL_INFO                            = 0x11;
constexpr uint32_t DB_STENCIL_INFO_FORMAT_SHIFT               = 0;
constexpr uint32_t DB_STENCIL_INFO_FORMAT_MASK                = 0x1;
constexpr uint32_t DB_STENCIL_INFO_TILE_MODE_INDEX_SHIFT      = 20;
constexpr uint32_t DB_STENCIL_INFO_TILE_MODE_INDEX_MASK       = 0x7;
constexpr uint32_t DB_STENCIL_INFO_TILE_STENCIL_DISABLE_SHIFT = 29;
constexpr uint32_t DB_STENCIL_INFO_TILE_STENCIL_DISABLE_MASK  = 0x1;

constexpr uint32_t DB_Z_READ_BASE                        = 0x12;
constexpr uint32_t DB_Z_READ_BASE_BASE_256B_SHIFT        = 0;
constexpr uint32_t DB_Z_READ_BASE_BASE_256B_MASK         = 0xFFFFFFFF;
constexpr uint32_t DB_STENCIL_READ_BASE                  = 0x13;
constexpr uint32_t DB_STENCIL_READ_BASE_BASE_256B_SHIFT  = 0;
constexpr uint32_t DB_STENCIL_READ_BASE_BASE_256B_MASK   = 0xFFFFFFFF;
constexpr uint32_t DB_Z_WRITE_BASE                       = 0x14;
constexpr uint32_t DB_Z_WRITE_BASE_BASE_256B_SHIFT       = 0;
constexpr uint32_t DB_Z_WRITE_BASE_BASE_256B_MASK        = 0xFFFFFFFF;
constexpr uint32_t DB_STENCIL_WRITE_BASE                 = 0x15;
constexpr uint32_t DB_STENCIL_WRITE_BASE_BASE_256B_SHIFT = 0;
constexpr uint32_t DB_STENCIL_WRITE_BASE_BASE_256B_MASK  = 0xFFFFFFFF;

constexpr uint32_t DB_DEPTH_SIZE                       = 0x16;
constexpr uint32_t DB_DEPTH_SIZE_PITCH_TILE_MAX_SHIFT  = 0;
constexpr uint32_t DB_DEPTH_SIZE_PITCH_TILE_MAX_MASK   = 0x7FF;
constexpr uint32_t DB_DEPTH_SIZE_HEIGHT_TILE_MAX_SHIFT = 11;
constexpr uint32_t DB_DEPTH_SIZE_HEIGHT_TILE_MAX_MASK  = 0x7FF;

constexpr uint32_t DB_DEPTH_SLICE                      = 0x17;
constexpr uint32_t DB_DEPTH_SLICE_SLICE_TILE_MAX_SHIFT = 0;
constexpr uint32_t DB_DEPTH_SLICE_SLICE_TILE_MAX_MASK  = 0x3FFFFF;

constexpr uint32_t PA_SC_VPORT_ZMIN_0 = 0xB4;

constexpr uint32_t CB_BLEND_RED   = 0x105;
constexpr uint32_t CB_BLEND_GREEN = 0x106;
constexpr uint32_t CB_BLEND_BLUE  = 0x107;
constexpr uint32_t CB_BLEND_ALPHA = 0x108;

constexpr uint32_t PA_CL_VPORT_XSCALE = 0x10F;

constexpr uint32_t SPI_PS_INPUT_CNTL_0 = 0x191;

constexpr uint32_t CB_BLEND0_CONTROL                            = 0x1E0;
constexpr uint32_t CB_BLEND0_CONTROL_COLOR_SRCBLEND_SHIFT       = 0;
constexpr uint32_t CB_BLEND0_CONTROL_COLOR_SRCBLEND_MASK        = 0x1F;
constexpr uint32_t CB_BLEND0_CONTROL_COLOR_COMB_FCN_SHIFT       = 5;
constexpr uint32_t CB_BLEND0_CONTROL_COLOR_COMB_FCN_MASK        = 0x7;
constexpr uint32_t CB_BLEND0_CONTROL_COLOR_DESTBLEND_SHIFT      = 8;
constexpr uint32_t CB_BLEND0_CONTROL_COLOR_DESTBLEND_MASK       = 0x1F;
constexpr uint32_t CB_BLEND0_CONTROL_ALPHA_SRCBLEND_SHIFT       = 16;
constexpr uint32_t CB_BLEND0_CONTROL_ALPHA_SRCBLEND_MASK        = 0x1F;
constexpr uint32_t CB_BLEND0_CONTROL_ALPHA_COMB_FCN_SHIFT       = 21;
constexpr uint32_t CB_BLEND0_CONTROL_ALPHA_COMB_FCN_MASK        = 0x7;
constexpr uint32_t CB_BLEND0_CONTROL_ALPHA_DESTBLEND_SHIFT      = 24;
constexpr uint32_t CB_BLEND0_CONTROL_ALPHA_DESTBLEND_MASK       = 0x1F;
constexpr uint32_t CB_BLEND0_CONTROL_SEPARATE_ALPHA_BLEND_SHIFT = 29;
constexpr uint32_t CB_BLEND0_CONTROL_SEPARATE_ALPHA_BLEND_MASK  = 0x1;
constexpr uint32_t CB_BLEND0_CONTROL_ENABLE_SHIFT               = 30;
constexpr uint32_t CB_BLEND0_CONTROL_ENABLE_MASK                = 0x1;

constexpr uint32_t DB_DEPTH_CONTROL                           = 0x200;
constexpr uint32_t DB_DEPTH_CONTROL_STENCIL_ENABLE_SHIFT      = 0;
constexpr uint32_t DB_DEPTH_CONTROL_STENCIL_ENABLE_MASK       = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_Z_ENABLE_SHIFT            = 1;
constexpr uint32_t DB_DEPTH_CONTROL_Z_ENABLE_MASK             = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_Z_WRITE_ENABLE_SHIFT      = 2;
constexpr uint32_t DB_DEPTH_CONTROL_Z_WRITE_ENABLE_MASK       = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_DEPTH_BOUNDS_ENABLE_SHIFT = 3;
constexpr uint32_t DB_DEPTH_CONTROL_DEPTH_BOUNDS_ENABLE_MASK  = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_ZFUNC_SHIFT               = 4;
constexpr uint32_t DB_DEPTH_CONTROL_ZFUNC_MASK                = 0x7;
constexpr uint32_t DB_DEPTH_CONTROL_BACKFACE_ENABLE_SHIFT     = 7;
constexpr uint32_t DB_DEPTH_CONTROL_BACKFACE_ENABLE_MASK      = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_STENCILFUNC_SHIFT         = 8;
constexpr uint32_t DB_DEPTH_CONTROL_STENCILFUNC_MASK          = 0x7;
constexpr uint32_t DB_DEPTH_CONTROL_STENCILFUNC_BF_SHIFT      = 20;
constexpr uint32_t DB_DEPTH_CONTROL_STENCILFUNC_BF_MASK       = 0x7;

constexpr uint32_t DB_EQAA                                  = 0x201;
constexpr uint32_t DB_EQAA_MAX_ANCHOR_SAMPLES_SHIFT         = 0;
constexpr uint32_t DB_EQAA_MAX_ANCHOR_SAMPLES_MASK          = 0x7;
constexpr uint32_t DB_EQAA_PS_ITER_SAMPLES_SHIFT            = 4;
constexpr uint32_t DB_EQAA_PS_ITER_SAMPLES_MASK             = 0x7;
constexpr uint32_t DB_EQAA_MASK_EXPORT_NUM_SAMPLES_SHIFT    = 8;
constexpr uint32_t DB_EQAA_MASK_EXPORT_NUM_SAMPLES_MASK     = 0x7;
constexpr uint32_t DB_EQAA_ALPHA_TO_MASK_NUM_SAMPLES_SHIFT  = 12;
constexpr uint32_t DB_EQAA_ALPHA_TO_MASK_NUM_SAMPLES_MASK   = 0x7;
constexpr uint32_t DB_EQAA_HIGH_QUALITY_INTERSECTIONS_SHIFT = 16;
constexpr uint32_t DB_EQAA_HIGH_QUALITY_INTERSECTIONS_MASK  = 0x1;
constexpr uint32_t DB_EQAA_INCOHERENT_EQAA_READS_SHIFT      = 17;
constexpr uint32_t DB_EQAA_INCOHERENT_EQAA_READS_MASK       = 0x1;
constexpr uint32_t DB_EQAA_INTERPOLATE_COMP_Z_SHIFT         = 18;
constexpr uint32_t DB_EQAA_INTERPOLATE_COMP_Z_MASK          = 0x1;
constexpr uint32_t DB_EQAA_STATIC_ANCHOR_ASSOCIATIONS_SHIFT = 20;
constexpr uint32_t DB_EQAA_STATIC_ANCHOR_ASSOCIATIONS_MASK  = 0x1;

constexpr uint32_t PA_SU_SC_MODE_CNTL                                = 0x205;
constexpr uint32_t PA_SU_SC_MODE_CNTL_CULL_FRONT_SHIFT               = 0;
constexpr uint32_t PA_SU_SC_MODE_CNTL_CULL_FRONT_MASK                = 0x1;
constexpr uint32_t PA_SU_SC_MODE_CNTL_CULL_BACK_SHIFT                = 1;
constexpr uint32_t PA_SU_SC_MODE_CNTL_CULL_BACK_MASK                 = 0x1;
constexpr uint32_t PA_SU_SC_MODE_CNTL_FACE_SHIFT                     = 2;
constexpr uint32_t PA_SU_SC_MODE_CNTL_FACE_MASK                      = 0x1;
constexpr uint32_t PA_SU_SC_MODE_CNTL_POLY_MODE_SHIFT                = 3;
constexpr uint32_t PA_SU_SC_MODE_CNTL_POLY_MODE_MASK                 = 0x3;
constexpr uint32_t PA_SU_SC_MODE_CNTL_POLYMODE_FRONT_PTYPE_SHIFT     = 5;
constexpr uint32_t PA_SU_SC_MODE_CNTL_POLYMODE_FRONT_PTYPE_MASK      = 0x7;
constexpr uint32_t PA_SU_SC_MODE_CNTL_POLYMODE_BACK_PTYPE_SHIFT      = 8;
constexpr uint32_t PA_SU_SC_MODE_CNTL_POLYMODE_BACK_PTYPE_MASK       = 0x7;
constexpr uint32_t PA_SU_SC_MODE_CNTL_POLY_OFFSET_FRONT_ENABLE_SHIFT = 11;
constexpr uint32_t PA_SU_SC_MODE_CNTL_POLY_OFFSET_FRONT_ENABLE_MASK  = 0x1;
constexpr uint32_t PA_SU_SC_MODE_CNTL_POLY_OFFSET_BACK_ENABLE_SHIFT  = 12;
constexpr uint32_t PA_SU_SC_MODE_CNTL_POLY_OFFSET_BACK_ENABLE_MASK   = 0x1;
constexpr uint32_t PA_SU_SC_MODE_CNTL_VTX_WINDOW_OFFSET_ENABLE_SHIFT = 16;
constexpr uint32_t PA_SU_SC_MODE_CNTL_VTX_WINDOW_OFFSET_ENABLE_MASK  = 0x1;
constexpr uint32_t PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST_SHIFT       = 19;
constexpr uint32_t PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST_MASK        = 0x1;
constexpr uint32_t PA_SU_SC_MODE_CNTL_PERSP_CORR_DIS_SHIFT           = 20;
constexpr uint32_t PA_SU_SC_MODE_CNTL_PERSP_CORR_DIS_MASK            = 0x1;

constexpr uint32_t DB_HTILE_SURFACE                               = 0x2AF;
constexpr uint32_t DB_HTILE_SURFACE_LINEAR_SHIFT                  = 0;
constexpr uint32_t DB_HTILE_SURFACE_LINEAR_MASK                   = 0x1;
constexpr uint32_t DB_HTILE_SURFACE_FULL_CACHE_SHIFT              = 1;
constexpr uint32_t DB_HTILE_SURFACE_FULL_CACHE_MASK               = 0x1;
constexpr uint32_t DB_HTILE_SURFACE_HTILE_USES_PRELOAD_WIN_SHIFT  = 2;
constexpr uint32_t DB_HTILE_SURFACE_HTILE_USES_PRELOAD_WIN_MASK   = 0x1;
constexpr uint32_t DB_HTILE_SURFACE_PRELOAD_SHIFT                 = 3;
constexpr uint32_t DB_HTILE_SURFACE_PRELOAD_MASK                  = 0x1;
constexpr uint32_t DB_HTILE_SURFACE_PREFETCH_WIDTH_SHIFT          = 4;
constexpr uint32_t DB_HTILE_SURFACE_PREFETCH_WIDTH_MASK           = 0x3F;
constexpr uint32_t DB_HTILE_SURFACE_PREFETCH_HEIGHT_SHIFT         = 10;
constexpr uint32_t DB_HTILE_SURFACE_PREFETCH_HEIGHT_MASK          = 0x3F;
constexpr uint32_t DB_HTILE_SURFACE_DST_OUTSIDE_ZERO_TO_ONE_SHIFT = 16;
constexpr uint32_t DB_HTILE_SURFACE_DST_OUTSIDE_ZERO_TO_ONE_MASK  = 0x1;

constexpr uint32_t VGT_SHADER_STAGES_EN = 0x2D5;

constexpr uint32_t CB_COLOR0_BASE = 0x318;

constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS             = 0xA;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_VGPRS_SHIFT = 0;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_VGPRS_MASK  = 0x3F;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_SGPRS_SHIFT = 6;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_SGPRS_MASK  = 0xF;

constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS                   = 0xB;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_SCRATCH_EN_SHIFT  = 0;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_SCRATCH_EN_MASK   = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_USER_SGPR_SHIFT   = 1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_USER_SGPR_MASK    = 0x1F;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_WAVE_CNT_EN_SHIFT = 7;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_WAVE_CNT_EN_MASK  = 0x0;

constexpr uint32_t SPI_SHADER_USER_DATA_PS_0  = 0xC;
constexpr uint32_t SPI_SHADER_USER_DATA_PS_15 = 0x1B;

constexpr uint32_t SPI_SHADER_USER_DATA_VS_0  = 0x4C;
constexpr uint32_t SPI_SHADER_USER_DATA_VS_15 = 0x5B;

constexpr uint32_t COMPUTE_PGM_RSRC1             = 0x212;
constexpr uint32_t COMPUTE_PGM_RSRC1_VGPRS_SHIFT = 0;
constexpr uint32_t COMPUTE_PGM_RSRC1_VGPRS_MASK  = 0x3F;
constexpr uint32_t COMPUTE_PGM_RSRC1_SGPRS_SHIFT = 6;
constexpr uint32_t COMPUTE_PGM_RSRC1_SGPRS_MASK  = 0xF;
constexpr uint32_t COMPUTE_PGM_RSRC1_BULKY_SHIFT = 24;
constexpr uint32_t COMPUTE_PGM_RSRC1_BULKY_MASK  = 0x1;

constexpr uint32_t COMPUTE_PGM_RSRC2                      = 0x213;
constexpr uint32_t COMPUTE_PGM_RSRC2_SCRATCH_EN_SHIFT     = 0;
constexpr uint32_t COMPUTE_PGM_RSRC2_SCRATCH_EN_MASK      = 0x1;
constexpr uint32_t COMPUTE_PGM_RSRC2_USER_SGPR_SHIFT      = 1;
constexpr uint32_t COMPUTE_PGM_RSRC2_USER_SGPR_MASK       = 0x1F;
constexpr uint32_t COMPUTE_PGM_RSRC2_TGID_X_EN_SHIFT      = 7;
constexpr uint32_t COMPUTE_PGM_RSRC2_TGID_X_EN_MASK       = 0x1;
constexpr uint32_t COMPUTE_PGM_RSRC2_TGID_Y_EN_SHIFT      = 8;
constexpr uint32_t COMPUTE_PGM_RSRC2_TGID_Y_EN_MASK       = 0x1;
constexpr uint32_t COMPUTE_PGM_RSRC2_TGID_Z_EN_SHIFT      = 9;
constexpr uint32_t COMPUTE_PGM_RSRC2_TGID_Z_EN_MASK       = 0x1;
constexpr uint32_t COMPUTE_PGM_RSRC2_TG_SIZE_EN_SHIFT     = 10;
constexpr uint32_t COMPUTE_PGM_RSRC2_TG_SIZE_EN_MASK      = 0x1;
constexpr uint32_t COMPUTE_PGM_RSRC2_TIDIG_COMP_CNT_SHIFT = 11;
constexpr uint32_t COMPUTE_PGM_RSRC2_TIDIG_COMP_CNT_MASK  = 0x3;
constexpr uint32_t COMPUTE_PGM_RSRC2_LDS_SIZE_SHIFT       = 15;
constexpr uint32_t COMPUTE_PGM_RSRC2_LDS_SIZE_MASK        = 0x1FF;

constexpr uint32_t COMPUTE_USER_DATA_0  = 0x240;
constexpr uint32_t COMPUTE_USER_DATA_15 = 0x24F;

void DumpPm4PacketStream(Core::File* file, uint32_t* cmd_buffer, uint32_t start_dw, uint32_t num_dw);

} // namespace Kyty::Libs::Graphics::Pm4

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_PM4_H_ */
