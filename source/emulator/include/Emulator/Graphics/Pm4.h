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
	(0xC0000000u | (((static_cast<uint16_t>(len) - 2u) & 0x3fffu) << 16u) | (((op)&0xffu) << 8u) | (((r) & (Pm4::R_NUM - 1u)) << 2u))

#define KYTY_PM4_R(cmd_id)   (((cmd_id) >> 2u) & (Pm4::R_NUM - 1u))
#define KYTY_PM4_LEN(cmd_id) ((((cmd_id) >> 16u) & 0x3fffu) + 2u)

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

/* Custom codes. Implemented via IT_NOP */

constexpr uint32_t R_ZERO             = 0x00;
constexpr uint32_t R_VS               = 0x01;
constexpr uint32_t R_PS               = 0x02;
constexpr uint32_t R_DRAW_INDEX       = 0x03;
constexpr uint32_t R_DRAW_INDEX_AUTO  = 0x04;
constexpr uint32_t R_DRAW_RESET       = 0x05;
constexpr uint32_t R_WAIT_FLIP_DONE   = 0x06;
constexpr uint32_t R_CS               = 0x07;
constexpr uint32_t R_DISPATCH_DIRECT  = 0x08;
constexpr uint32_t R_DISPATCH_RESET   = 0x09;
constexpr uint32_t R_WAIT_MEM_32      = 0x0A;
constexpr uint32_t R_PUSH_MARKER      = 0x0B;
constexpr uint32_t R_POP_MARKER       = 0x0C;
constexpr uint32_t R_VS_EMBEDDED      = 0x0D;
constexpr uint32_t R_PS_EMBEDDED      = 0x0E;
constexpr uint32_t R_VS_UPDATE        = 0x0F;
constexpr uint32_t R_PS_UPDATE        = 0x10;
constexpr uint32_t R_SH_REGS_INDIRECT = 0x11;
constexpr uint32_t R_CX_REGS_INDIRECT = 0x12;
constexpr uint32_t R_UC_REGS_INDIRECT = 0x13;
constexpr uint32_t R_ACQUIRE_MEM      = 0x14;
constexpr uint32_t R_WRITE_DATA       = 0x15;
constexpr uint32_t R_WAIT_MEM_64      = 0x16;
constexpr uint32_t R_FLIP             = 0x17;
constexpr uint32_t R_RELEASE_MEM      = 0x18;

constexpr uint32_t R_NUM = 0x3F + 1;

/* Context registers */

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

constexpr uint32_t DB_COUNT_CONTROL = 0x1;

constexpr uint32_t DB_DEPTH_VIEW                         = 0x2;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_START_SHIFT       = 0;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_START_MASK        = 0x7FF;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_START_HI_SHIFT    = 11;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_START_HI_MASK     = 0x3;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_MAX_SHIFT         = 13;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_MAX_MASK          = 0x7FF;
constexpr uint32_t DB_DEPTH_VIEW_Z_READ_ONLY_SHIFT       = 24;
constexpr uint32_t DB_DEPTH_VIEW_Z_READ_ONLY_MASK        = 0x1;
constexpr uint32_t DB_DEPTH_VIEW_STENCIL_READ_ONLY_SHIFT = 25;
constexpr uint32_t DB_DEPTH_VIEW_STENCIL_READ_ONLY_MASK  = 0x1;
constexpr uint32_t DB_DEPTH_VIEW_MIPID_SHIFT             = 26;
constexpr uint32_t DB_DEPTH_VIEW_MIPID_MASK              = 0xF;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_MAX_HI_SHIFT      = 30;
constexpr uint32_t DB_DEPTH_VIEW_SLICE_MAX_HI_MASK       = 0x3;

constexpr uint32_t DB_RENDER_OVERRIDE              = 0x3;
constexpr uint32_t DB_RENDER_OVERRIDE2             = 0x4;
constexpr uint32_t DB_HTILE_DATA_BASE              = 0x5;
constexpr uint32_t PS_SHADER_SAMPLE_EXCLUSION_MASK = 0x6;

constexpr uint32_t DB_DEPTH_SIZE_XY             = 0x7;
constexpr uint32_t DB_DEPTH_SIZE_XY_X_MAX_SHIFT = 0;
constexpr uint32_t DB_DEPTH_SIZE_XY_X_MAX_MASK  = 0x3FFF;
constexpr uint32_t DB_DEPTH_SIZE_XY_Y_MAX_SHIFT = 16;
constexpr uint32_t DB_DEPTH_SIZE_XY_Y_MAX_MASK  = 0x3FFF;

constexpr uint32_t DB_DEPTH_BOUNDS_MIN = 0x8;
constexpr uint32_t DB_DEPTH_BOUNDS_MAX = 0x9;

constexpr uint32_t DB_STENCIL_CLEAR             = 0xA;
constexpr uint32_t DB_STENCIL_CLEAR_CLEAR_SHIFT = 0;
constexpr uint32_t DB_STENCIL_CLEAR_CLEAR_MASK  = 0xFF;

constexpr uint32_t DB_DEPTH_CLEAR                   = 0xB;
constexpr uint32_t DB_DEPTH_CLEAR_DEPTH_CLEAR_SHIFT = 0;
constexpr uint32_t DB_DEPTH_CLEAR_DEPTH_CLEAR_MASK  = 0xFFFFFFFF;

constexpr uint32_t PA_SC_SCREEN_SCISSOR_TL            = 0xC;
constexpr uint32_t PA_SC_SCREEN_SCISSOR_TL_TL_X_SHIFT = 0;
constexpr uint32_t PA_SC_SCREEN_SCISSOR_TL_TL_X_MASK  = 0xFFFF;
constexpr uint32_t PA_SC_SCREEN_SCISSOR_TL_TL_Y_SHIFT = 16;
constexpr uint32_t PA_SC_SCREEN_SCISSOR_TL_TL_Y_MASK  = 0xFFFF;

constexpr uint32_t PA_SC_SCREEN_SCISSOR_BR            = 0xD;
constexpr uint32_t PA_SC_SCREEN_SCISSOR_BR_BR_X_SHIFT = 0;
constexpr uint32_t PA_SC_SCREEN_SCISSOR_BR_BR_X_MASK  = 0xFFFF;
constexpr uint32_t PA_SC_SCREEN_SCISSOR_BR_BR_Y_SHIFT = 16;
constexpr uint32_t PA_SC_SCREEN_SCISSOR_BR_BR_Y_MASK  = 0xFFFF;

constexpr uint32_t DB_DFSM_CONTROL = 0xE;

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

constexpr uint32_t DB_Z_INFO                               = 0x10;
constexpr uint32_t DB_Z_INFO_FORMAT_SHIFT                  = 0;
constexpr uint32_t DB_Z_INFO_FORMAT_MASK                   = 0x3;
constexpr uint32_t DB_Z_INFO_NUM_SAMPLES_SHIFT             = 2;
constexpr uint32_t DB_Z_INFO_NUM_SAMPLES_MASK              = 0x3;
constexpr uint32_t DB_Z_INFO_ITERATE_FLUSH_SHIFT           = 11;
constexpr uint32_t DB_Z_INFO_ITERATE_FLUSH_MASK            = 0x1;
constexpr uint32_t DB_Z_INFO_PARTIALLY_RESIDENT_SHIFT      = 12;
constexpr uint32_t DB_Z_INFO_PARTIALLY_RESIDENT_MASK       = 0x1;
constexpr uint32_t DB_Z_INFO_MAXMIP_SHIFT                  = 16;
constexpr uint32_t DB_Z_INFO_MAXMIP_MASK                   = 0xF;
constexpr uint32_t DB_Z_INFO_TILE_MODE_INDEX_SHIFT         = 20;
constexpr uint32_t DB_Z_INFO_TILE_MODE_INDEX_MASK          = 0x7;
constexpr uint32_t DB_Z_INFO_DECOMPRESS_ON_N_ZPLANES_SHIFT = 23;
constexpr uint32_t DB_Z_INFO_DECOMPRESS_ON_N_ZPLANES_MASK  = 0xF;
constexpr uint32_t DB_Z_INFO_ALLOW_EXPCLEAR_SHIFT          = 27;
constexpr uint32_t DB_Z_INFO_ALLOW_EXPCLEAR_MASK           = 0x1;
constexpr uint32_t DB_Z_INFO_TILE_SURFACE_ENABLE_SHIFT     = 29;
constexpr uint32_t DB_Z_INFO_TILE_SURFACE_ENABLE_MASK      = 0x1;
constexpr uint32_t DB_Z_INFO_ZRANGE_PRECISION_SHIFT        = 31;
constexpr uint32_t DB_Z_INFO_ZRANGE_PRECISION_MASK         = 0x1;

constexpr uint32_t DB_STENCIL_INFO                            = 0x11;
constexpr uint32_t DB_STENCIL_INFO_FORMAT_SHIFT               = 0;
constexpr uint32_t DB_STENCIL_INFO_FORMAT_MASK                = 0x1;
constexpr uint32_t DB_STENCIL_INFO_ITERATE_FLUSH_SHIFT        = 11;
constexpr uint32_t DB_STENCIL_INFO_ITERATE_FLUSH_MASK         = 0x1;
constexpr uint32_t DB_STENCIL_INFO_PARTIALLY_RESIDENT_SHIFT   = 12;
constexpr uint32_t DB_STENCIL_INFO_PARTIALLY_RESIDENT_MASK    = 0x1;
constexpr uint32_t DB_STENCIL_INFO_RESERVED_FIELD_1_SHIFT     = 13;
constexpr uint32_t DB_STENCIL_INFO_RESERVED_FIELD_1_MASK      = 0x7;
constexpr uint32_t DB_STENCIL_INFO_TILE_MODE_INDEX_SHIFT      = 20;
constexpr uint32_t DB_STENCIL_INFO_TILE_MODE_INDEX_MASK       = 0x7;
constexpr uint32_t DB_STENCIL_INFO_ALLOW_EXPCLEAR_SHIFT       = 27;
constexpr uint32_t DB_STENCIL_INFO_ALLOW_EXPCLEAR_MASK        = 0x1;
constexpr uint32_t DB_STENCIL_INFO_TILE_STENCIL_DISABLE_SHIFT = 29;
constexpr uint32_t DB_STENCIL_INFO_TILE_STENCIL_DISABLE_MASK  = 0x1;

constexpr uint32_t DB_Z_READ_BASE        = 0x12;
constexpr uint32_t DB_STENCIL_READ_BASE  = 0x13;
constexpr uint32_t DB_Z_WRITE_BASE       = 0x14;
constexpr uint32_t DB_STENCIL_WRITE_BASE = 0x15;

constexpr uint32_t DB_DEPTH_SIZE                       = 0x16;
constexpr uint32_t DB_DEPTH_SIZE_PITCH_TILE_MAX_SHIFT  = 0;
constexpr uint32_t DB_DEPTH_SIZE_PITCH_TILE_MAX_MASK   = 0x7FF;
constexpr uint32_t DB_DEPTH_SIZE_HEIGHT_TILE_MAX_SHIFT = 11;
constexpr uint32_t DB_DEPTH_SIZE_HEIGHT_TILE_MAX_MASK  = 0x7FF;

constexpr uint32_t DB_DEPTH_SLICE                      = 0x17;
constexpr uint32_t DB_DEPTH_SLICE_SLICE_TILE_MAX_SHIFT = 0;
constexpr uint32_t DB_DEPTH_SLICE_SLICE_TILE_MAX_MASK  = 0x3FFFFF;

constexpr uint32_t DB_Z_READ_BASE_HI        = 0x1A;
constexpr uint32_t DB_STENCIL_READ_BASE_HI  = 0x1B;
constexpr uint32_t DB_Z_WRITE_BASE_HI       = 0x1C;
constexpr uint32_t DB_STENCIL_WRITE_BASE_HI = 0x1D;
constexpr uint32_t DB_HTILE_DATA_BASE_HI    = 0x1E;
constexpr uint32_t DB_RMI_L2_CACHE_CONTROL  = 0x1F;
constexpr uint32_t TA_BC_BASE_ADDR          = 0x20;
constexpr uint32_t TA_BC_BASE_ADDR_HI       = 0x21;
constexpr uint32_t PA_SC_WINDOW_OFFSET      = 0x80;
constexpr uint32_t PA_SC_WINDOW_SCISSOR_TL  = 0x81;
constexpr uint32_t PA_SC_WINDOW_SCISSOR_BR  = 0x82;
constexpr uint32_t PA_SC_CLIPRECT_RULE      = 0x83;
constexpr uint32_t PA_SC_CLIPRECT_0_TL      = 0x84;
constexpr uint32_t PA_SC_CLIPRECT_0_BR      = 0x85;

constexpr uint32_t PA_SU_HARDWARE_SCREEN_OFFSET                          = 0x8D;
constexpr uint32_t PA_SU_HARDWARE_SCREEN_OFFSET_HW_SCREEN_OFFSET_X_SHIFT = 0;
constexpr uint32_t PA_SU_HARDWARE_SCREEN_OFFSET_HW_SCREEN_OFFSET_X_MASK  = 0x1FF;
constexpr uint32_t PA_SU_HARDWARE_SCREEN_OFFSET_HW_SCREEN_OFFSET_Y_SHIFT = 16;
constexpr uint32_t PA_SU_HARDWARE_SCREEN_OFFSET_HW_SCREEN_OFFSET_Y_MASK  = 0x1FF;

constexpr uint32_t CB_TARGET_MASK = 0x8E;
constexpr uint32_t CB_SHADER_MASK = 0x8F;

constexpr uint32_t PA_SC_GENERIC_SCISSOR_TL                             = 0x90;
constexpr uint32_t PA_SC_GENERIC_SCISSOR_TL_TL_X_SHIFT                  = 0;
constexpr uint32_t PA_SC_GENERIC_SCISSOR_TL_TL_X_MASK                   = 0x7FFF;
constexpr uint32_t PA_SC_GENERIC_SCISSOR_TL_TL_Y_SHIFT                  = 16;
constexpr uint32_t PA_SC_GENERIC_SCISSOR_TL_TL_Y_MASK                   = 0x7FFF;
constexpr uint32_t PA_SC_GENERIC_SCISSOR_TL_WINDOW_OFFSET_DISABLE_SHIFT = 31;
constexpr uint32_t PA_SC_GENERIC_SCISSOR_TL_WINDOW_OFFSET_DISABLE_MASK  = 0x1;

constexpr uint32_t PA_SC_GENERIC_SCISSOR_BR            = 0x91;
constexpr uint32_t PA_SC_GENERIC_SCISSOR_BR_BR_X_SHIFT = 0;
constexpr uint32_t PA_SC_GENERIC_SCISSOR_BR_BR_X_MASK  = 0x7FFF;
constexpr uint32_t PA_SC_GENERIC_SCISSOR_BR_BR_Y_SHIFT = 16;
constexpr uint32_t PA_SC_GENERIC_SCISSOR_BR_BR_Y_MASK  = 0x7FFF;

constexpr uint32_t PA_SC_VPORT_SCISSOR_0_TL                             = 0x94;
constexpr uint32_t PA_SC_VPORT_SCISSOR_0_TL_TL_X_SHIFT                  = 0;
constexpr uint32_t PA_SC_VPORT_SCISSOR_0_TL_TL_X_MASK                   = 0x7FFF;
constexpr uint32_t PA_SC_VPORT_SCISSOR_0_TL_TL_Y_SHIFT                  = 16;
constexpr uint32_t PA_SC_VPORT_SCISSOR_0_TL_TL_Y_MASK                   = 0x7FFF;
constexpr uint32_t PA_SC_VPORT_SCISSOR_0_TL_WINDOW_OFFSET_DISABLE_SHIFT = 31;
constexpr uint32_t PA_SC_VPORT_SCISSOR_0_TL_WINDOW_OFFSET_DISABLE_MASK  = 0x1;

constexpr uint32_t PA_SC_VPORT_SCISSOR_0_BR            = 0x95;
constexpr uint32_t PA_SC_VPORT_SCISSOR_0_BR_BR_X_SHIFT = 0;
constexpr uint32_t PA_SC_VPORT_SCISSOR_0_BR_BR_X_MASK  = 0x7FFF;
constexpr uint32_t PA_SC_VPORT_SCISSOR_0_BR_BR_Y_SHIFT = 16;
constexpr uint32_t PA_SC_VPORT_SCISSOR_0_BR_BR_Y_MASK  = 0x7FFF;

constexpr uint32_t PA_SC_VPORT_SCISSOR_15_TL = 0xB2;
constexpr uint32_t PA_SC_VPORT_SCISSOR_15_BR = 0xB3;
constexpr uint32_t PA_SC_VPORT_ZMIN_0        = 0xB4;
constexpr uint32_t PA_SC_VPORT_ZMAX_0        = 0xB5;
constexpr uint32_t PA_SC_VPORT_ZMIN_15       = 0xD2;
constexpr uint32_t PA_SC_VPORT_ZMAX_15       = 0xD3;
constexpr uint32_t PA_SC_RIGHT_VERT_GRID     = 0xE8;
constexpr uint32_t PA_SC_LEFT_VERT_GRID      = 0xE9;
constexpr uint32_t PA_SC_HORIZ_GRID          = 0xEA;
constexpr uint32_t PA_SC_FOV_WINDOW_LR       = 0xEB;
constexpr uint32_t PA_SC_FOV_WINDOW_TB       = 0xEC;
constexpr uint32_t CB_RMI_GL2_CACHE_CONTROL  = 0x104;
constexpr uint32_t CB_BLEND_RED              = 0x105;
constexpr uint32_t CB_BLEND_GREEN            = 0x106;
constexpr uint32_t CB_BLEND_BLUE             = 0x107;
constexpr uint32_t CB_BLEND_ALPHA            = 0x108;
constexpr uint32_t CB_DCC_CONTROL            = 0x109;

constexpr uint32_t DB_STENCIL_CONTROL                       = 0x10B;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILFAIL_SHIFT     = 0;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILFAIL_MASK      = 0xF;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILZPASS_SHIFT    = 4;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILZPASS_MASK     = 0xF;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILZFAIL_SHIFT    = 8;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILZFAIL_MASK     = 0xF;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILFAIL_BF_SHIFT  = 12;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILFAIL_BF_MASK   = 0xF;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILZPASS_BF_SHIFT = 16;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILZPASS_BF_MASK  = 0xF;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILZFAIL_BF_SHIFT = 20;
constexpr uint32_t DB_STENCIL_CONTROL_STENCILZFAIL_BF_MASK  = 0xF;

constexpr uint32_t DB_STENCILREFMASK                        = 0x10C;
constexpr uint32_t DB_STENCILREFMASK_STENCILTESTVAL_SHIFT   = 0;
constexpr uint32_t DB_STENCILREFMASK_STENCILTESTVAL_MASK    = 0xFF;
constexpr uint32_t DB_STENCILREFMASK_STENCILMASK_SHIFT      = 8;
constexpr uint32_t DB_STENCILREFMASK_STENCILMASK_MASK       = 0xFF;
constexpr uint32_t DB_STENCILREFMASK_STENCILWRITEMASK_SHIFT = 16;
constexpr uint32_t DB_STENCILREFMASK_STENCILWRITEMASK_MASK  = 0xFF;
constexpr uint32_t DB_STENCILREFMASK_STENCILOPVAL_SHIFT     = 24;
constexpr uint32_t DB_STENCILREFMASK_STENCILOPVAL_MASK      = 0xFF;

constexpr uint32_t DB_STENCILREFMASK_BF                           = 0x10D;
constexpr uint32_t DB_STENCILREFMASK_BF_STENCILTESTVAL_BF_SHIFT   = 0;
constexpr uint32_t DB_STENCILREFMASK_BF_STENCILTESTVAL_BF_MASK    = 0xFF;
constexpr uint32_t DB_STENCILREFMASK_BF_STENCILMASK_BF_SHIFT      = 8;
constexpr uint32_t DB_STENCILREFMASK_BF_STENCILMASK_BF_MASK       = 0xFF;
constexpr uint32_t DB_STENCILREFMASK_BF_STENCILWRITEMASK_BF_SHIFT = 16;
constexpr uint32_t DB_STENCILREFMASK_BF_STENCILWRITEMASK_BF_MASK  = 0xFF;
constexpr uint32_t DB_STENCILREFMASK_BF_STENCILOPVAL_BF_SHIFT     = 24;
constexpr uint32_t DB_STENCILREFMASK_BF_STENCILOPVAL_BF_MASK      = 0xFF;

constexpr uint32_t PA_CL_VPORT_XSCALE     = 0x10F;
constexpr uint32_t PA_CL_VPORT_XOFFSET    = 0x110;
constexpr uint32_t PA_CL_VPORT_YSCALE     = 0x111;
constexpr uint32_t PA_CL_VPORT_YOFFSET    = 0x112;
constexpr uint32_t PA_CL_VPORT_ZSCALE     = 0x113;
constexpr uint32_t PA_CL_VPORT_ZOFFSET    = 0x114;
constexpr uint32_t PA_CL_VPORT_XSCALE_15  = 0x169;
constexpr uint32_t PA_CL_VPORT_XOFFSET_15 = 0x16A;
constexpr uint32_t PA_CL_VPORT_YSCALE_15  = 0x16B;
constexpr uint32_t PA_CL_VPORT_YOFFSET_15 = 0x16C;
constexpr uint32_t PA_CL_VPORT_ZSCALE_15  = 0x16D;
constexpr uint32_t PA_CL_VPORT_ZOFFSET_15 = 0x16E;
constexpr uint32_t PA_CL_UCP_0_X          = 0x16F;
constexpr uint32_t PA_CL_UCP_0_Y          = 0x170;
constexpr uint32_t PA_CL_UCP_0_Z          = 0x171;
constexpr uint32_t PA_CL_UCP_0_W          = 0x172;
constexpr uint32_t SPI_PS_INPUT_CNTL_0    = 0x191;
constexpr uint32_t SPI_PS_INPUT_CNTL_31   = 0x1B0;
constexpr uint32_t SPI_VS_OUT_CONFIG      = 0x1B1;
constexpr uint32_t SPI_PS_INPUT_ENA       = 0x1B3;
constexpr uint32_t SPI_PS_INPUT_ADDR      = 0x1B4;
constexpr uint32_t SPI_INTERP_CONTROL_0   = 0x1B5;
constexpr uint32_t SPI_PS_IN_CONTROL      = 0x1B6;
constexpr uint32_t SPI_BARYC_CNTL         = 0x1B8;
constexpr uint32_t SPI_TMPRING_SIZE       = 0x1BA;
constexpr uint32_t SPI_SHADER_IDX_FORMAT  = 0x1C2;
constexpr uint32_t SPI_SHADER_POS_FORMAT  = 0x1C3;
constexpr uint32_t SPI_SHADER_Z_FORMAT    = 0x1C4;
constexpr uint32_t SPI_SHADER_COL_FORMAT  = 0x1C5;

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

constexpr uint32_t GE_MAX_OUTPUT_PER_SUBGROUP = 0x1FF;

constexpr uint32_t DB_DEPTH_CONTROL                                          = 0x200;
constexpr uint32_t DB_DEPTH_CONTROL_STENCIL_ENABLE_SHIFT                     = 0;
constexpr uint32_t DB_DEPTH_CONTROL_STENCIL_ENABLE_MASK                      = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_Z_ENABLE_SHIFT                           = 1;
constexpr uint32_t DB_DEPTH_CONTROL_Z_ENABLE_MASK                            = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_Z_WRITE_ENABLE_SHIFT                     = 2;
constexpr uint32_t DB_DEPTH_CONTROL_Z_WRITE_ENABLE_MASK                      = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_DEPTH_BOUNDS_ENABLE_SHIFT                = 3;
constexpr uint32_t DB_DEPTH_CONTROL_DEPTH_BOUNDS_ENABLE_MASK                 = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_ZFUNC_SHIFT                              = 4;
constexpr uint32_t DB_DEPTH_CONTROL_ZFUNC_MASK                               = 0x7;
constexpr uint32_t DB_DEPTH_CONTROL_BACKFACE_ENABLE_SHIFT                    = 7;
constexpr uint32_t DB_DEPTH_CONTROL_BACKFACE_ENABLE_MASK                     = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_STENCILFUNC_SHIFT                        = 8;
constexpr uint32_t DB_DEPTH_CONTROL_STENCILFUNC_MASK                         = 0x7;
constexpr uint32_t DB_DEPTH_CONTROL_STENCILFUNC_BF_SHIFT                     = 20;
constexpr uint32_t DB_DEPTH_CONTROL_STENCILFUNC_BF_MASK                      = 0x7;
constexpr uint32_t DB_DEPTH_CONTROL_ENABLE_COLOR_WRITES_ON_DEPTH_FAIL_SHIFT  = 30;
constexpr uint32_t DB_DEPTH_CONTROL_ENABLE_COLOR_WRITES_ON_DEPTH_FAIL_MASK   = 0x1;
constexpr uint32_t DB_DEPTH_CONTROL_DISABLE_COLOR_WRITES_ON_DEPTH_PASS_SHIFT = 31;
constexpr uint32_t DB_DEPTH_CONTROL_DISABLE_COLOR_WRITES_ON_DEPTH_PASS_MASK  = 0x1;

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

constexpr uint32_t DB_SHADER_CONTROL                             = 0x203;
constexpr uint32_t DB_SHADER_CONTROL_Z_EXPORT_ENABLE_SHIFT       = 0;
constexpr uint32_t DB_SHADER_CONTROL_Z_EXPORT_ENABLE_MASK        = 0x1;
constexpr uint32_t DB_SHADER_CONTROL_Z_ORDER_SHIFT               = 4;
constexpr uint32_t DB_SHADER_CONTROL_Z_ORDER_MASK                = 0x3;
constexpr uint32_t DB_SHADER_CONTROL_KILL_ENABLE_SHIFT           = 6;
constexpr uint32_t DB_SHADER_CONTROL_KILL_ENABLE_MASK            = 0x1;
constexpr uint32_t DB_SHADER_CONTROL_EXEC_ON_NOOP_SHIFT          = 10;
constexpr uint32_t DB_SHADER_CONTROL_EXEC_ON_NOOP_MASK           = 0x1;
constexpr uint32_t DB_SHADER_CONTROL_CONSERVATIVE_Z_EXPORT_SHIFT = 13;
constexpr uint32_t DB_SHADER_CONTROL_CONSERVATIVE_Z_EXPORT_MASK  = 0x3;

constexpr uint32_t CB_COLOR_CONTROL            = 0x202;
constexpr uint32_t CB_COLOR_CONTROL_MODE_SHIFT = 4;
constexpr uint32_t CB_COLOR_CONTROL_MODE_MASK  = 0x7;
constexpr uint32_t CB_COLOR_CONTROL_ROP3_SHIFT = 16;
constexpr uint32_t CB_COLOR_CONTROL_ROP3_MASK  = 0xFF;

constexpr uint32_t PA_CL_CLIP_CNTL                                 = 0x204;
constexpr uint32_t PA_CL_CLIP_CNTL_UCP_ENA_SHIFT                   = 0;
constexpr uint32_t PA_CL_CLIP_CNTL_UCP_ENA_MASK                    = 0x3F;
constexpr uint32_t PA_CL_CLIP_CNTL_PS_UCP_Y_SCALE_NEG_SHIFT        = 13;
constexpr uint32_t PA_CL_CLIP_CNTL_PS_UCP_Y_SCALE_NEG_MASK         = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_PS_UCP_MODE_SHIFT               = 14;
constexpr uint32_t PA_CL_CLIP_CNTL_PS_UCP_MODE_MASK                = 0x3;
constexpr uint32_t PA_CL_CLIP_CNTL_CLIP_DISABLE_SHIFT              = 16;
constexpr uint32_t PA_CL_CLIP_CNTL_CLIP_DISABLE_MASK               = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_UCP_CULL_ONLY_ENA_SHIFT         = 17;
constexpr uint32_t PA_CL_CLIP_CNTL_UCP_CULL_ONLY_ENA_MASK          = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_BOUNDARY_EDGE_FLAG_ENA_SHIFT    = 18;
constexpr uint32_t PA_CL_CLIP_CNTL_BOUNDARY_EDGE_FLAG_ENA_MASK     = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_DX_CLIP_SPACE_DEF_SHIFT         = 19;
constexpr uint32_t PA_CL_CLIP_CNTL_DX_CLIP_SPACE_DEF_MASK          = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_DIS_CLIP_ERR_DETECT_SHIFT       = 20;
constexpr uint32_t PA_CL_CLIP_CNTL_DIS_CLIP_ERR_DETECT_MASK        = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_VTX_KILL_OR_SHIFT               = 21;
constexpr uint32_t PA_CL_CLIP_CNTL_VTX_KILL_OR_MASK                = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_DX_RASTERIZATION_KILL_SHIFT     = 22;
constexpr uint32_t PA_CL_CLIP_CNTL_DX_RASTERIZATION_KILL_MASK      = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_DX_LINEAR_ATTR_CLIP_ENA_SHIFT   = 24;
constexpr uint32_t PA_CL_CLIP_CNTL_DX_LINEAR_ATTR_CLIP_ENA_MASK    = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_VTE_VPORT_PROVOKE_DISABLE_SHIFT = 25;
constexpr uint32_t PA_CL_CLIP_CNTL_VTE_VPORT_PROVOKE_DISABLE_MASK  = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_ZCLIP_NEAR_DISABLE_SHIFT        = 26;
constexpr uint32_t PA_CL_CLIP_CNTL_ZCLIP_NEAR_DISABLE_MASK         = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_ZCLIP_FAR_DISABLE_SHIFT         = 27;
constexpr uint32_t PA_CL_CLIP_CNTL_ZCLIP_FAR_DISABLE_MASK          = 0x1;
constexpr uint32_t PA_CL_CLIP_CNTL_ZCLIP_PROG_NEAR_ENA_SHIFT       = 28;
constexpr uint32_t PA_CL_CLIP_CNTL_ZCLIP_PROG_NEAR_ENA_MASK        = 0x1;

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

constexpr uint32_t PA_CL_VTE_CNTL               = 0x206;
constexpr uint32_t PA_CL_VS_OUT_CNTL            = 0x207;
constexpr uint32_t PA_SU_SMALL_PRIM_FILTER_CNTL = 0x20C;
constexpr uint32_t PA_CL_OBJPRIM_ID_CNTL        = 0x20D;
constexpr uint32_t PA_STEREO_CNTL               = 0x210;
constexpr uint32_t PA_STATE_STEREO_X            = 0x211;
constexpr uint32_t PA_SU_POINT_SIZE             = 0x280;
constexpr uint32_t PA_SU_POINT_MINMAX           = 0x281;

constexpr uint32_t PA_SU_LINE_CNTL             = 0x282;
constexpr uint32_t PA_SU_LINE_CNTL_WIDTH_SHIFT = 0;
constexpr uint32_t PA_SU_LINE_CNTL_WIDTH_MASK  = 0xFFFF;

constexpr uint32_t VGT_GS_ONCHIP_CNTL = 0x291;

constexpr uint32_t PA_SC_MODE_CNTL_0                            = 0x292;
constexpr uint32_t PA_SC_MODE_CNTL_0_MSAA_ENABLE_SHIFT          = 0;
constexpr uint32_t PA_SC_MODE_CNTL_0_MSAA_ENABLE_MASK           = 0x1;
constexpr uint32_t PA_SC_MODE_CNTL_0_VPORT_SCISSOR_ENABLE_SHIFT = 1;
constexpr uint32_t PA_SC_MODE_CNTL_0_VPORT_SCISSOR_ENABLE_MASK  = 0x1;
constexpr uint32_t PA_SC_MODE_CNTL_0_LINE_STIPPLE_ENABLE_SHIFT  = 2;
constexpr uint32_t PA_SC_MODE_CNTL_0_LINE_STIPPLE_ENABLE_MASK   = 0x1;

constexpr uint32_t PA_SC_MODE_CNTL_1      = 0x293;
constexpr uint32_t VGT_GS_OUT_PRIM_TYPE   = 0x29B;
constexpr uint32_t VGT_PRIMITIVEID_EN     = 0x2A1;
constexpr uint32_t VGT_PRIMITIVEID_RESET  = 0x2A3;
constexpr uint32_t VGT_DRAW_PAYLOAD_CNTL  = 0x2A6;
constexpr uint32_t VGT_ESGS_RING_ITEMSIZE = 0x2AB;
constexpr uint32_t VGT_REUSE_OFF          = 0x2AD;

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

constexpr uint32_t DB_SRESULTS_COMPARE_STATE0     = 0x2B0;
constexpr uint32_t DB_SRESULTS_COMPARE_STATE1     = 0x2B1;
constexpr uint32_t VGT_GS_MAX_VERT_OUT            = 0x2CE;
constexpr uint32_t GE_NGG_SUBGRP_CNTL             = 0x2D3;
constexpr uint32_t VGT_TESS_DISTRIBUTION          = 0x2D4;
constexpr uint32_t VGT_SHADER_STAGES_EN           = 0x2D5;
constexpr uint32_t VGT_LS_HS_CONFIG               = 0x2D6;
constexpr uint32_t VGT_TF_PARAM                   = 0x2DB;
constexpr uint32_t DB_ALPHA_TO_MASK               = 0x2DC;
constexpr uint32_t PA_SU_POLY_OFFSET_DB_FMT_CNTL  = 0x2DE;
constexpr uint32_t PA_SU_POLY_OFFSET_CLAMP        = 0x2DF;
constexpr uint32_t PA_SU_POLY_OFFSET_FRONT_SCALE  = 0x2E0;
constexpr uint32_t PA_SU_POLY_OFFSET_FRONT_OFFSET = 0x2E1;
constexpr uint32_t PA_SU_POLY_OFFSET_BACK_SCALE   = 0x2E2;
constexpr uint32_t PA_SU_POLY_OFFSET_BACK_OFFSET  = 0x2E3;
constexpr uint32_t VGT_GS_INSTANCE_CNT            = 0x2E4;
constexpr uint32_t PA_SC_CENTROID_PRIORITY_0      = 0x2F5;
constexpr uint32_t PA_SC_CENTROID_PRIORITY_1      = 0x2F6;

constexpr uint32_t PA_SC_AA_CONFIG                             = 0x2F8;
constexpr uint32_t PA_SC_AA_CONFIG_MSAA_NUM_SAMPLES_SHIFT      = 0;
constexpr uint32_t PA_SC_AA_CONFIG_MSAA_NUM_SAMPLES_MASK       = 0x7;
constexpr uint32_t PA_SC_AA_CONFIG_AA_MASK_CENTROID_DTMN_SHIFT = 4;
constexpr uint32_t PA_SC_AA_CONFIG_AA_MASK_CENTROID_DTMN_MASK  = 0x1;
constexpr uint32_t PA_SC_AA_CONFIG_MAX_SAMPLE_DIST_SHIFT       = 13;
constexpr uint32_t PA_SC_AA_CONFIG_MAX_SAMPLE_DIST_MASK        = 0xF;
constexpr uint32_t PA_SC_AA_CONFIG_MSAA_EXPOSED_SAMPLES_SHIFT  = 20;
constexpr uint32_t PA_SC_AA_CONFIG_MSAA_EXPOSED_SAMPLES_MASK   = 0x7;

constexpr uint32_t PA_SU_VTX_CNTL                        = 0x2F9;
constexpr uint32_t PA_CL_GB_VERT_CLIP_ADJ                = 0x2FA;
constexpr uint32_t PA_CL_GB_VERT_DISC_ADJ                = 0x2FB;
constexpr uint32_t PA_CL_GB_HORZ_CLIP_ADJ                = 0x2FC;
constexpr uint32_t PA_CL_GB_HORZ_DISC_ADJ                = 0x2FD;
constexpr uint32_t PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0     = 0x2FE;
constexpr uint32_t PA_SC_AA_MASK_X0Y0_X1Y0               = 0x30E;
constexpr uint32_t PA_SC_AA_MASK_X0Y1_X1Y1               = 0x30F;
constexpr uint32_t PA_SC_SHADER_CONTROL                  = 0x310;
constexpr uint32_t PA_SC_BINNER_CNTL_0                   = 0x311;
constexpr uint32_t PA_SC_BINNER_CNTL_1                   = 0x312;
constexpr uint32_t PA_SC_CONSERVATIVE_RASTERIZATION_CNTL = 0x313;
constexpr uint32_t PA_SC_NGG_MODE_CNTL                   = 0x314;
constexpr uint32_t CB_COLOR0_BASE                        = 0x318;

constexpr uint32_t CB_COLOR0_VIEW                   = 0x31B;
constexpr uint32_t CB_COLOR0_VIEW_SLICE_START_SHIFT = 0;
constexpr uint32_t CB_COLOR0_VIEW_SLICE_START_MASK  = 0x1FFF;
constexpr uint32_t CB_COLOR0_VIEW_SLICE_MAX_SHIFT   = 13;
constexpr uint32_t CB_COLOR0_VIEW_SLICE_MAX_MASK    = 0x1FFF;
constexpr uint32_t CB_COLOR0_VIEW_MIP_LEVEL_SHIFT   = 26;
constexpr uint32_t CB_COLOR0_VIEW_MIP_LEVEL_MASK    = 0xF;

constexpr uint32_t CB_COLOR0_INFO                                 = 0x31C;
constexpr uint32_t CB_COLOR0_INFO_FORMAT_SHIFT                    = 2;
constexpr uint32_t CB_COLOR0_INFO_FORMAT_MASK                     = 0x1F;
constexpr uint32_t CB_COLOR0_INFO_NUMBER_TYPE_SHIFT               = 8;
constexpr uint32_t CB_COLOR0_INFO_NUMBER_TYPE_MASK                = 0x7;
constexpr uint32_t CB_COLOR0_INFO_COMP_SWAP_SHIFT                 = 11;
constexpr uint32_t CB_COLOR0_INFO_COMP_SWAP_MASK                  = 0x3;
constexpr uint32_t CB_COLOR0_INFO_FAST_CLEAR_SHIFT                = 13;
constexpr uint32_t CB_COLOR0_INFO_FAST_CLEAR_MASK                 = 0x1;
constexpr uint32_t CB_COLOR0_INFO_COMPRESSION_SHIFT               = 14;
constexpr uint32_t CB_COLOR0_INFO_COMPRESSION_MASK                = 0x1;
constexpr uint32_t CB_COLOR0_INFO_BLEND_CLAMP_SHIFT               = 15;
constexpr uint32_t CB_COLOR0_INFO_BLEND_CLAMP_MASK                = 0x1;
constexpr uint32_t CB_COLOR0_INFO_BLEND_BYPASS_SHIFT              = 16;
constexpr uint32_t CB_COLOR0_INFO_BLEND_BYPASS_MASK               = 0x1;
constexpr uint32_t CB_COLOR0_INFO_ROUND_MODE_SHIFT                = 18;
constexpr uint32_t CB_COLOR0_INFO_ROUND_MODE_MASK                 = 0x1;
constexpr uint32_t CB_COLOR0_INFO_CMASK_IS_LINEAR_SHIFT           = 19;
constexpr uint32_t CB_COLOR0_INFO_CMASK_IS_LINEAR_MASK            = 0x1;
constexpr uint32_t CB_COLOR0_INFO_FMASK_COMPRESSION_DISABLE_SHIFT = 26;
constexpr uint32_t CB_COLOR0_INFO_FMASK_COMPRESSION_DISABLE_MASK  = 0x1;
constexpr uint32_t CB_COLOR0_INFO_FMASK_COMPRESS_1FRAG_ONLY_SHIFT = 27;
constexpr uint32_t CB_COLOR0_INFO_FMASK_COMPRESS_1FRAG_ONLY_MASK  = 0x1;
constexpr uint32_t CB_COLOR0_INFO_DCC_ENABLE_SHIFT                = 28;
constexpr uint32_t CB_COLOR0_INFO_DCC_ENABLE_MASK                 = 0x1;
constexpr uint32_t CB_COLOR0_INFO_CMASK_ADDR_TYPE_SHIFT           = 29;
constexpr uint32_t CB_COLOR0_INFO_CMASK_ADDR_TYPE_MASK            = 0x3;
constexpr uint32_t CB_COLOR0_INFO_ALT_TILE_MODE_SHIFT             = 31;
constexpr uint32_t CB_COLOR0_INFO_ALT_TILE_MODE_MASK              = 0x1;

constexpr uint32_t CB_COLOR0_ATTRIB                             = 0x31D;
constexpr uint32_t CB_COLOR0_ATTRIB_TILE_MODE_INDEX_SHIFT       = 0;
constexpr uint32_t CB_COLOR0_ATTRIB_TILE_MODE_INDEX_MASK        = 0x1F;
constexpr uint32_t CB_COLOR0_ATTRIB_FMASK_TILE_MODE_INDEX_SHIFT = 5;
constexpr uint32_t CB_COLOR0_ATTRIB_FMASK_TILE_MODE_INDEX_MASK  = 0x1F;
constexpr uint32_t CB_COLOR0_ATTRIB_NUM_SAMPLES_SHIFT           = 12;
constexpr uint32_t CB_COLOR0_ATTRIB_NUM_SAMPLES_MASK            = 0x7;
constexpr uint32_t CB_COLOR0_ATTRIB_NUM_FRAGMENTS_SHIFT         = 15;
constexpr uint32_t CB_COLOR0_ATTRIB_NUM_FRAGMENTS_MASK          = 0x3;
constexpr uint32_t CB_COLOR0_ATTRIB_FORCE_DST_ALPHA_1_SHIFT     = 17;
constexpr uint32_t CB_COLOR0_ATTRIB_FORCE_DST_ALPHA_1_MASK      = 0x1;

constexpr uint32_t CB_COLOR0_DCC_CONTROL                                        = 0x31E;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_OVERWRITE_COMBINER_DISABLE_SHIFT       = 0;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_OVERWRITE_COMBINER_DISABLE_MASK        = 0x1;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_KEY_CLEAR_ENABLE_SHIFT                 = 1;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_KEY_CLEAR_ENABLE_MASK                  = 0x1;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_MAX_UNCOMPRESSED_BLOCK_SIZE_SHIFT      = 2;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_MAX_UNCOMPRESSED_BLOCK_SIZE_MASK       = 0x3;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_MIN_COMPRESSED_BLOCK_SIZE_SHIFT        = 4;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_MIN_COMPRESSED_BLOCK_SIZE_MASK         = 0x1;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_MAX_COMPRESSED_BLOCK_SIZE_SHIFT        = 5;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_MAX_COMPRESSED_BLOCK_SIZE_MASK         = 0x3;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_COLOR_TRANSFORM_SHIFT                  = 7;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_COLOR_TRANSFORM_MASK                   = 0x3;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_INDEPENDENT_64B_BLOCKS_SHIFT           = 9;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_INDEPENDENT_64B_BLOCKS_MASK            = 0x1;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_ENABLE_CONSTANT_ENCODE_REG_WRITE_SHIFT = 19;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_ENABLE_CONSTANT_ENCODE_REG_WRITE_MASK  = 0x1;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_INDEPENDENT_128B_BLOCKS_SHIFT          = 20;
constexpr uint32_t CB_COLOR0_DCC_CONTROL_INDEPENDENT_128B_BLOCKS_MASK           = 0x1;

constexpr uint32_t CB_COLOR0_CMASK          = 0x31F;
constexpr uint32_t CB_COLOR0_FMASK          = 0x321;
constexpr uint32_t CB_COLOR0_CLEAR_WORD0    = 0x323;
constexpr uint32_t CB_COLOR0_CLEAR_WORD1    = 0x324;
constexpr uint32_t CB_COLOR0_DCC_BASE       = 0x325;
constexpr uint32_t CB_COLOR7_BASE           = 0x381;
constexpr uint32_t CB_COLOR7_VIEW           = 0x384;
constexpr uint32_t CB_COLOR7_INFO           = 0x385;
constexpr uint32_t CB_COLOR7_ATTRIB         = 0x386;
constexpr uint32_t CB_COLOR7_DCC_CONTROL    = 0x387;
constexpr uint32_t CB_COLOR7_CMASK          = 0x388;
constexpr uint32_t CB_COLOR7_FMASK          = 0x38A;
constexpr uint32_t CB_COLOR7_CLEAR_WORD0    = 0x38C;
constexpr uint32_t CB_COLOR7_CLEAR_WORD1    = 0x38D;
constexpr uint32_t CB_COLOR7_DCC_BASE       = 0x38E;
constexpr uint32_t CB_COLOR0_BASE_EXT       = 0x390;
constexpr uint32_t CB_COLOR7_BASE_EXT       = 0x397;
constexpr uint32_t CB_COLOR0_CMASK_BASE_EXT = 0x398;
constexpr uint32_t CB_COLOR7_CMASK_BASE_EXT = 0x39F;
constexpr uint32_t CB_COLOR0_FMASK_BASE_EXT = 0x3A0;
constexpr uint32_t CB_COLOR7_FMASK_BASE_EXT = 0x3A7;
constexpr uint32_t CB_COLOR0_DCC_BASE_EXT   = 0x3A8;
constexpr uint32_t CB_COLOR7_DCC_BASE_EXT   = 0x3AF;

constexpr uint32_t CB_COLOR0_ATTRIB2                   = 0x3B0;
constexpr uint32_t CB_COLOR0_ATTRIB2_MIP0_HEIGHT_SHIFT = 0;
constexpr uint32_t CB_COLOR0_ATTRIB2_MIP0_HEIGHT_MASK  = 0x3FFF;
constexpr uint32_t CB_COLOR0_ATTRIB2_MIP0_WIDTH_SHIFT  = 14;
constexpr uint32_t CB_COLOR0_ATTRIB2_MIP0_WIDTH_MASK   = 0x3FFF;
constexpr uint32_t CB_COLOR0_ATTRIB2_MAX_MIP_SHIFT     = 28;
constexpr uint32_t CB_COLOR0_ATTRIB2_MAX_MIP_MASK      = 0xF;

constexpr uint32_t CB_COLOR7_ATTRIB2 = 0x3B7;

constexpr uint32_t CB_COLOR0_ATTRIB3                          = 0x3B8;
constexpr uint32_t CB_COLOR0_ATTRIB3_MIP0_DEPTH_SHIFT         = 0;
constexpr uint32_t CB_COLOR0_ATTRIB3_MIP0_DEPTH_MASK          = 0x1FFF;
constexpr uint32_t CB_COLOR0_ATTRIB3_COLOR_SW_MODE_SHIFT      = 14;
constexpr uint32_t CB_COLOR0_ATTRIB3_COLOR_SW_MODE_MASK       = 0x1F;
constexpr uint32_t CB_COLOR0_ATTRIB3_RESOURCE_TYPE_SHIFT      = 24;
constexpr uint32_t CB_COLOR0_ATTRIB3_RESOURCE_TYPE_MASK       = 0x3;
constexpr uint32_t CB_COLOR0_ATTRIB3_CMASK_PIPE_ALIGNED_SHIFT = 26;
constexpr uint32_t CB_COLOR0_ATTRIB3_CMASK_PIPE_ALIGNED_MASK  = 0x1;
constexpr uint32_t CB_COLOR0_ATTRIB3_DCC_PIPE_ALIGNED_SHIFT   = 30;
constexpr uint32_t CB_COLOR0_ATTRIB3_DCC_PIPE_ALIGNED_MASK    = 0x1;

constexpr uint32_t CB_COLOR7_ATTRIB3 = 0x3BF;

/* Fake codes. Don't exist on real HW */

constexpr uint32_t FSR_RECURSIONS0  = 0x800003FC;
constexpr uint32_t FSR_RECURSIONS1  = 0x800003FD;
constexpr uint32_t PA_SC_FSR_ENABLE = 0x800003FE;
constexpr uint32_t CX_NOP           = 0x800003FF;

constexpr uint32_t CX_NUM = 0x3FF + 1;

/* Shader registers */

constexpr uint32_t SPI_SHADER_PGM_RSRC4_PS  = 0x1;
constexpr uint32_t SPI_SHADER_PGM_CHKSUM_PS = 0x6;
constexpr uint32_t SPI_SHADER_PGM_RSRC3_PS  = 0x7;
constexpr uint32_t SPI_SHADER_PGM_LO_PS     = 0x8;
constexpr uint32_t SPI_SHADER_PGM_HI_PS     = 0x9;

constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS                        = 0xA;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_VGPRS_SHIFT            = 0;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_VGPRS_MASK             = 0x3F;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_SGPRS_SHIFT            = 6;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_SGPRS_MASK             = 0xF;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_PRIORITY_SHIFT         = 10;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_PRIORITY_MASK          = 0x3;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_FLOAT_MODE_SHIFT       = 12;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_FLOAT_MODE_MASK        = 0xFF;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_DX10_CLAMP_SHIFT       = 21;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_DX10_CLAMP_MASK        = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_DEBUG_MODE_SHIFT       = 22;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_DEBUG_MODE_MASK        = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_IEEE_MODE_SHIFT        = 23;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_IEEE_MODE_MASK         = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_CU_GROUP_DISABLE_SHIFT = 24;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_CU_GROUP_DISABLE_MASK  = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_FWD_PROGRESS_SHIFT     = 26;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_FWD_PROGRESS_MASK      = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_FP16_OVFL_SHIFT        = 29;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_PS_FP16_OVFL_MASK         = 0x1;

constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS                                = 0xB;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_SCRATCH_EN_SHIFT               = 0;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_SCRATCH_EN_MASK                = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_USER_SGPR_SHIFT                = 1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_USER_SGPR_MASK                 = 0x1F;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_WAVE_CNT_EN_SHIFT              = 7;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_WAVE_CNT_EN_MASK               = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_EXTRA_LDS_SIZE_SHIFT           = 8;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_EXTRA_LDS_SIZE_MASK            = 0xFF;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_LOAD_INTRAWAVE_COLLISION_SHIFT = 26;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_LOAD_INTRAWAVE_COLLISION_MASK  = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_USER_SGPR_MSB_SHIFT            = 27;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_USER_SGPR_MSB_MASK             = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_SHARED_VGPR_CNT_SHIFT          = 28;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_PS_SHARED_VGPR_CNT_MASK           = 0xF;

constexpr uint32_t SPI_SHADER_USER_DATA_PS_0  = 0xC;
constexpr uint32_t SPI_SHADER_USER_DATA_PS_15 = 0x1B;
constexpr uint32_t SPI_SHADER_USER_ACCUM_PS_0 = 0x32;
constexpr uint32_t SPI_SHADER_PGM_LO_VS       = 0x48;
constexpr uint32_t SPI_SHADER_PGM_HI_VS       = 0x49;

constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS                       = 0x4A;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_VGPRS_SHIFT           = 0;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_VGPRS_MASK            = 0x3F;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_SGPRS_SHIFT           = 6;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_SGPRS_MASK            = 0xF;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_PRIORITY_SHIFT        = 10;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_PRIORITY_MASK         = 0x3;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_FLOAT_MODE_SHIFT      = 12;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_FLOAT_MODE_MASK       = 0xFF;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_DX10_CLAMP_SHIFT      = 21;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_DX10_CLAMP_MASK       = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_IEEE_MODE_SHIFT       = 23;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_IEEE_MODE_MASK        = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_VGPR_COMP_CNT_SHIFT   = 24;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_VGPR_COMP_CNT_MASK    = 0x3;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_CU_GROUP_ENABLE_SHIFT = 26;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_CU_GROUP_ENABLE_MASK  = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_FWD_PROGRESS_SHIFT    = 28;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_FWD_PROGRESS_MASK     = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_FP16_OVFL_SHIFT       = 31;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_VS_FP16_OVFL_MASK        = 0x1;

constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS                       = 0x4B;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_SCRATCH_EN_SHIFT      = 0;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_SCRATCH_EN_MASK       = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_USER_SGPR_SHIFT       = 1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_USER_SGPR_MASK        = 0x1F;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_OC_LDS_EN_SHIFT       = 7;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_OC_LDS_EN_MASK        = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_SO_EN_SHIFT           = 12;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_SO_EN_MASK            = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_USER_SGPR_MSB_SHIFT   = 27;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_USER_SGPR_MSB_MASK    = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_SHARED_VGPR_CNT_SHIFT = 28;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_VS_SHARED_VGPR_CNT_MASK  = 0xF;

constexpr uint32_t SPI_SHADER_USER_DATA_VS_0       = 0x4C;
constexpr uint32_t SPI_SHADER_USER_DATA_VS_15      = 0x5B;
constexpr uint32_t SPI_SHADER_PGM_CHKSUM_GS        = 0x80;
constexpr uint32_t SPI_SHADER_PGM_RSRC4_GS         = 0x81;
constexpr uint32_t SPI_SHADER_USER_DATA_ADDR_LO_GS = 0x82;
constexpr uint32_t SPI_SHADER_USER_DATA_ADDR_HI_GS = 0x83;
constexpr uint32_t SPI_SHADER_PGM_RSRC3_GS         = 0x87;
constexpr uint32_t SPI_SHADER_PGM_LO_GS            = 0x88;
constexpr uint32_t SPI_SHADER_PGM_HI_GS            = 0x89;

constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS                        = 0x8A;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_VGPRS_SHIFT            = 0;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_VGPRS_MASK             = 0x3F;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_SGPRS_SHIFT            = 6;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_SGPRS_MASK             = 0xF;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_PRIORITY_SHIFT         = 10;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_PRIORITY_MASK          = 0x3;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_FLOAT_MODE_SHIFT       = 12;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_FLOAT_MODE_MASK        = 0xFF;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_DX10_CLAMP_SHIFT       = 21;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_DX10_CLAMP_MASK        = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_DEBUG_MODE_SHIFT       = 22;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_DEBUG_MODE_MASK        = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_IEEE_MODE_SHIFT        = 23;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_IEEE_MODE_MASK         = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_CU_GROUP_ENABLE_SHIFT  = 24;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_CU_GROUP_ENABLE_MASK   = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_FWD_PROGRESS_SHIFT     = 26;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_FWD_PROGRESS_MASK      = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_WGP_MODE_SHIFT         = 27;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_WGP_MODE_MASK          = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_GS_VGPR_COMP_CNT_SHIFT = 29;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_GS_VGPR_COMP_CNT_MASK  = 0x3;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_FP16_OVFL_SHIFT        = 31;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_GS_FP16_OVFL_MASK         = 0x1;

constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS                        = 0x8B;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_SCRATCH_EN_SHIFT       = 0;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_SCRATCH_EN_MASK        = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_USER_SGPR_SHIFT        = 1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_USER_SGPR_MASK         = 0x1F;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_ES_VGPR_COMP_CNT_SHIFT = 16;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_ES_VGPR_COMP_CNT_MASK  = 0x3;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_OC_LDS_EN_SHIFT        = 18;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_OC_LDS_EN_MASK         = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_LDS_SIZE_SHIFT         = 19;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_LDS_SIZE_MASK          = 0xFF;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_USER_SGPR_MSB_SHIFT    = 27;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_USER_SGPR_MSB_MASK     = 0x1;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_SHARED_VGPR_CNT_SHIFT  = 28;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_GS_SHARED_VGPR_CNT_MASK   = 0xF;

constexpr uint32_t SPI_SHADER_USER_DATA_GS_0       = 0x8C;
constexpr uint32_t SPI_SHADER_USER_DATA_GS_15      = 0x9B;
constexpr uint32_t SPI_SHADER_USER_ACCUM_ESGS_0    = 0xB2;
constexpr uint32_t SPI_SHADER_PGM_LO_ES            = 0xC8;
constexpr uint32_t SPI_SHADER_PGM_HI_ES            = 0xC9;
constexpr uint32_t SPI_SHADER_PGM_CHKSUM_HS        = 0x100;
constexpr uint32_t SPI_SHADER_PGM_RSRC4_HS         = 0x101;
constexpr uint32_t SPI_SHADER_USER_DATA_ADDR_LO_HS = 0x102;
constexpr uint32_t SPI_SHADER_USER_DATA_ADDR_HI_HS = 0x103;
constexpr uint32_t SPI_SHADER_PGM_RSRC3_HS         = 0x107;
constexpr uint32_t SPI_SHADER_PGM_LO_HS            = 0x108;
constexpr uint32_t SPI_SHADER_PGM_HI_HS            = 0x109;
constexpr uint32_t SPI_SHADER_PGM_RSRC1_HS         = 0x10A;
constexpr uint32_t SPI_SHADER_PGM_RSRC2_HS         = 0x10B;
constexpr uint32_t SPI_SHADER_USER_DATA_HS_0       = 0x10C;
constexpr uint32_t SPI_SHADER_USER_ACCUM_LSHS_0    = 0x132;
constexpr uint32_t SPI_SHADER_PGM_LO_LS            = 0x148;
constexpr uint32_t SPI_SHADER_PGM_HI_LS            = 0x149;
constexpr uint32_t COMPUTE_START_X                 = 0x204;
constexpr uint32_t COMPUTE_START_Y                 = 0x205;
constexpr uint32_t COMPUTE_START_Z                 = 0x206;
constexpr uint32_t COMPUTE_PGM_LO                  = 0x20C;
constexpr uint32_t COMPUTE_PGM_HI                  = 0x20D;

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

constexpr uint32_t COMPUTE_RESOURCE_LIMITS    = 0x215;
constexpr uint32_t COMPUTE_DESTINATION_EN_SE0 = 0x216;
constexpr uint32_t COMPUTE_DESTINATION_EN_SE1 = 0x217;
constexpr uint32_t COMPUTE_TMPRING_SIZE       = 0x218;
constexpr uint32_t COMPUTE_DESTINATION_EN_SE2 = 0x219;
constexpr uint32_t COMPUTE_DESTINATION_EN_SE3 = 0x21A;
constexpr uint32_t COMPUTE_USER_ACCUM_0       = 0x224;
constexpr uint32_t COMPUTE_PGM_RSRC3          = 0x228;
constexpr uint32_t COMPUTE_SHADER_CHKSUM      = 0x22A;
constexpr uint32_t COMPUTE_USER_DATA_0        = 0x240;
constexpr uint32_t COMPUTE_USER_DATA_15       = 0x24F;
constexpr uint32_t COMPUTE_DISPATCH_TUNNEL    = 0x27D;

/* Fake codes. Don't exist on real HW */

constexpr uint32_t SH_NOP = 0x800002FF;

constexpr uint32_t SH_NUM = 0x2FF + 1;

/* User config registers */

constexpr uint32_t VGT_PRIMITIVE_TYPE                 = 0x242;
constexpr uint32_t VGT_PRIMITIVE_TYPE_PRIM_TYPE_SHIFT = 0;
constexpr uint32_t VGT_PRIMITIVE_TYPE_PRIM_TYPE_MASK  = 0x3F;

constexpr uint32_t VGT_OBJECT_ID             = 0x248;
constexpr uint32_t GE_INDX_OFFSET            = 0x24A;
constexpr uint32_t GE_MULTI_PRIM_IB_RESET_EN = 0x24B;
constexpr uint32_t VGT_HS_OFFCHIP_PARAM      = 0x24F;
constexpr uint32_t VGT_TF_MEMORY_BASE        = 0x250;

constexpr uint32_t GE_CNTL                     = 0x25B;
constexpr uint32_t GE_CNTL_PRIM_GRP_SIZE_SHIFT = 0;
constexpr uint32_t GE_CNTL_PRIM_GRP_SIZE_MASK  = 0x1FF;
constexpr uint32_t GE_CNTL_VERT_GRP_SIZE_SHIFT = 9;
constexpr uint32_t GE_CNTL_VERT_GRP_SIZE_MASK  = 0x1FF;

constexpr uint32_t GE_USER_VGPR1  = 0x25C;
constexpr uint32_t GE_USER_VGPR2  = 0x25D;
constexpr uint32_t GE_USER_VGPR3  = 0x25E;
constexpr uint32_t GE_STEREO_CNTL = 0x25F;

constexpr uint32_t GE_USER_VGPR_EN                     = 0x262;
constexpr uint32_t GE_USER_VGPR_EN_EN_USER_VGPR1_SHIFT = 0;
constexpr uint32_t GE_USER_VGPR_EN_EN_USER_VGPR1_MASK  = 0x1;
constexpr uint32_t GE_USER_VGPR_EN_EN_USER_VGPR2_SHIFT = 1;
constexpr uint32_t GE_USER_VGPR_EN_EN_USER_VGPR2_MASK  = 0x1;
constexpr uint32_t GE_USER_VGPR_EN_EN_USER_VGPR3_SHIFT = 2;
constexpr uint32_t GE_USER_VGPR_EN_EN_USER_VGPR3_MASK  = 0x1;

constexpr uint32_t TA_CS_BC_BASE_ADDR       = 0x380;
constexpr uint32_t TA_CS_BC_BASE_ADDR_HI    = 0x381;
constexpr uint32_t TEXTURE_GRADIENT_FACTORS = 0x382;
constexpr uint32_t GDS_OA_CNTL              = 0x41D;
constexpr uint32_t GDS_OA_COUNTER           = 0x41E;
constexpr uint32_t GDS_OA_ADDRESS           = 0x41F;

/* Fake codes. Don't exist on real HW */

constexpr uint32_t FSR_EXTEND_SUBPIXEL_ROUNDING = 0x80003FF4;
constexpr uint32_t FSR_ALPHA_VALUE0             = 0x80003FF5;
constexpr uint32_t FSR_ALPHA_VALUE1             = 0x80003FF6;
constexpr uint32_t FSR_CONTROL_POINT0           = 0x80003FF7;
constexpr uint32_t FSR_CONTROL_POINT1           = 0x80003FF8;
constexpr uint32_t FSR_CONTROL_POINT2           = 0x80003FF9;
constexpr uint32_t FSR_CONTROL_POINT3           = 0x80003FFA;
constexpr uint32_t FSR_WINDOW0                  = 0x80003FFB;
constexpr uint32_t FSR_WINDOW1                  = 0x80003FFC;
constexpr uint32_t TEXTURE_GRADIENT_CONTROL     = 0x80003FFD;
constexpr uint32_t MEMORY_MAPPING_MASK          = 0x80003FFE;
constexpr uint32_t UC_NOP                       = 0x80003FFF;

constexpr uint32_t UC_NUM = 0x3FFF + 1;

void DumpPm4PacketStream(Core::File* file, uint32_t* cmd_buffer, uint32_t start_dw, uint32_t num_dw);

} // namespace Kyty::Libs::Graphics::Pm4

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_PM4_H_ */
