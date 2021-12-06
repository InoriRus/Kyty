#include "Emulator/Graphics/ShaderSpirv.h"

#include "Kyty/Core/ArrayWrapper.h"
#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Graphics/Shader.h"

#ifdef KYTY_EMU_ENABLED

#define KYTY_RECOMPILER_ARGS                                                                                                               \
	[[maybe_unused]] uint32_t index, [[maybe_unused]] const ShaderCode &code, [[maybe_unused]] String *dst_source,                         \
	    [[maybe_unused]] Spirv *spirv, [[maybe_unused]] const char32_t **param, [[maybe_unused]] SccCheck scc_check
#define KYTY_RECOMPILER_FUNC(f) static bool f(KYTY_RECOMPILER_ARGS)

namespace Kyty::Libs::Graphics {

constexpr char32_t FUNC_FETCH_4[] = UR"(
       ; Function fetch_f1_f1_f1_f1_vf4_
       ; void fetch(out float p1, out float p2, out float p3, out float p4, in vec4 attr)
       ; {
       ; p1 = attr.x;
       ; p2 = attr.y;
       ; p3 = attr.z;
       ; p4 = attr.w;
       ; }
%fetch_f1_f1_f1_f1_vf4_ = OpFunction %void None %function_fetch4 
 %fetch_p1 = OpFunctionParameter %_ptr_Function_float
 %fetch_p2 = OpFunctionParameter %_ptr_Function_float
 %fetch_p3 = OpFunctionParameter %_ptr_Function_float
 %fetch_p4 = OpFunctionParameter %_ptr_Function_float
%fetch_attr = OpFunctionParameter %_ptr_Function_v4float
%fetch_label = OpLabel
 %fetch_20 = OpAccessChain %_ptr_Function_float %fetch_attr %uint_0 
 %fetch_21 = OpLoad %float %fetch_20
             OpStore %fetch_p1 %fetch_21
 %fetch_23 = OpAccessChain %_ptr_Function_float %fetch_attr %uint_1 
 %fetch_24 = OpLoad %float %fetch_23
             OpStore %fetch_p2 %fetch_24
 %fetch_26 = OpAccessChain %_ptr_Function_float %fetch_attr %uint_2 
 %fetch_27 = OpLoad %float %fetch_26
             OpStore %fetch_p3 %fetch_27 
 %fetch_29 = OpAccessChain %_ptr_Function_float %fetch_attr %uint_3 
 %fetch_30 = OpLoad %float %fetch_29
             OpStore %fetch_p4 %fetch_30
             OpReturn
             OpFunctionEnd
)";

constexpr char32_t FUNC_FETCH_3[] = UR"(
       ; Function fetch_f1_f1_f1_vf3_
       ; void fetch(out float p1, out float p2, out float p3, in vec3 attr)
       ; {
       ; p1 = attr.x;
       ; p2 = attr.y;
       ; p3 = attr.z;
       ; }
%fetch_f1_f1_f1_vf3_ = OpFunction %void None %function_fetch3
       %fetch3_p1_0 = OpFunctionParameter %_ptr_Function_float
       %fetch3_p2_0 = OpFunctionParameter %_ptr_Function_float
       %fetch3_p3_0 = OpFunctionParameter %_ptr_Function_float
     %fetch3_attr_0 = OpFunctionParameter %_ptr_Function_v3float
         %fetch3_26 = OpLabel
         %fetch3_53 = OpAccessChain %_ptr_Function_float %fetch3_attr_0 %uint_0
         %fetch3_54 = OpLoad %float %fetch3_53
               OpStore %fetch3_p1_0 %fetch3_54
         %fetch3_55 = OpAccessChain %_ptr_Function_float %fetch3_attr_0 %uint_1
         %fetch3_56 = OpLoad %float %fetch3_55
               OpStore %fetch3_p2_0 %fetch3_56
         %fetch3_57 = OpAccessChain %_ptr_Function_float %fetch3_attr_0 %uint_2
         %fetch3_58 = OpLoad %float %fetch3_57
               OpStore %fetch3_p3_0 %fetch3_58
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t FUNC_FETCH_2[] = UR"(
       ; Function fetch_f1_f1_vf2_
       ; void fetch(out float p1, out float p2, in vec2 attr)
       ; {
       ; p1 = attr.x;
       ; p2 = attr.y;
       ; }
%fetch_f1_f1_vf2_ = OpFunction %void None %function_fetch2
       %fetch2_p1_1 = OpFunctionParameter %_ptr_Function_float
       %fetch2_p2_1 = OpFunctionParameter %_ptr_Function_float
     %fetch2_attr_1 = OpFunctionParameter %_ptr_Function_v2float
         %fetch2_34 = OpLabel
         %fetch2_59 = OpAccessChain %_ptr_Function_float %fetch2_attr_1 %uint_0
         %fetch2_60 = OpLoad %float %fetch2_59
               OpStore %fetch2_p1_1 %fetch2_60
         %fetch2_61 = OpAccessChain %_ptr_Function_float %fetch2_attr_1 %uint_1
         %fetch2_62 = OpLoad %float %fetch2_61
               OpStore %fetch2_p2_1 %fetch2_62
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t FUNC_FETCH_1[] = UR"(
       ; Function fetch_f1_f1_
       ; void fetch(out float p1, in float attr)
       ; {
       ; p1 = attr;
       ; }
%fetch_f1_f1_ = OpFunction %void None %function_fetch1
       %fetch1_p1_2 = OpFunctionParameter %_ptr_Function_float
     %fetch1_attr_2 = OpFunctionParameter %_ptr_Function_float
         %fetch1_39 = OpLabel
         %fetch1_63 = OpLoad %float %fetch1_attr_2
               OpStore %fetch1_p1_2 %fetch1_63
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t FUNC_ABS_DIFF[] = UR"(
                    ; uint abs_diff(uint u1, uint u2)
                    ; {
                    ; 	return max(u1,u2)-min(u1,u2);	
                    ; }
%abs_diff = OpFunction %uint None %function_u_u
         %abs_diff_18 = OpFunctionParameter %uint
         %abs_diff_19 = OpFunctionParameter %uint
         %abs_diff_21 = OpLabel
         %abs_diff_50 = OpExtInst %uint %GLSL_std_450 UMax %abs_diff_18 %abs_diff_19
         %abs_diff_53 = OpExtInst %uint %GLSL_std_450 UMin %abs_diff_18 %abs_diff_19
         %abs_diff_54 = OpISub %uint %abs_diff_50 %abs_diff_53
               OpReturnValue %abs_diff_54
               OpFunctionEnd
)";

constexpr char32_t FUNC_MUL_EXTENDED[] = UR"(
               ; uint mul_lo_uint(uint u1, uint u2)
               ; {
               ; 	uint r1, r2;
               ; 	umulExtended(u1, u2, r1, r2);
               ; 	return r2;
               ; }
               ; uint mul_hi_uint(uint u1, uint u2)
               ; {
               ; 	uint r1, r2;
               ; 	umulExtended(u1, u2, r1, r2);
               ; 	return r1;
               ; }
               ; int mul_lo_int(int i1, int i2)
               ; {
               ; 	int r1, r2;
               ; 	imulExtended(i1, i2, r1, r2);
               ; 	return r2;
               ; }
               ; int mul_hi_int(int i1, int i2)
               ; {
               ; 	int r1, r2;
               ; 	imulExtended(i1, i2, r1, r2);
               ; 	return r1;
               ; }
         %mul_lo_uint = OpFunction %uint None %function_u_u
         %22 = OpFunctionParameter %uint
         %23 = OpFunctionParameter %uint
         %25 = OpLabel
         %79 = OpUMulExtended %ResTypeU %22 %23
         %80 = OpCompositeExtract %uint %79 0
               OpReturnValue %80
               OpFunctionEnd
         %mul_hi_uint = OpFunction %uint None %function_u_u
         %26 = OpFunctionParameter %uint
         %27 = OpFunctionParameter %uint
         %29 = OpLabel
         %89 = OpUMulExtended %ResTypeU %26 %27
         %91 = OpCompositeExtract %uint %89 1
               OpReturnValue %91
               OpFunctionEnd
         %mul_lo_int = OpFunction %int None %function_i_i
         %31 = OpFunctionParameter %int
         %32 = OpFunctionParameter %int
         %34 = OpLabel
        %100 = OpSMulExtended %ResTypeI %31 %32
        %101 = OpCompositeExtract %int %100 0
               OpReturnValue %101
               OpFunctionEnd
         %mul_hi_int = OpFunction %int None %function_i_i
         %35 = OpFunctionParameter %int
         %36 = OpFunctionParameter %int
         %38 = OpLabel
        %110 = OpSMulExtended %ResTypeI %35 %36
        %112 = OpCompositeExtract %int %110 1
               OpReturnValue %112
               OpFunctionEnd
)";

constexpr char32_t BUFFER_LOAD_FLOAT1[] = UR"(
             ; void buffer_load_float1(out float p1, in int index, in int offset, in int stride, in int buffer_index)
             ; {
             ; 	int addr = (offset + index * stride)/4;
             ; 	p1 = buf[buffer_index].data[addr+0];
             ; }
%buffer_load_float1 = OpFunction %void None %function_buffer_load_store_float1
         %buf_l_f1_11 = OpFunctionParameter %_ptr_Function_float 
         %buf_l_f1_12 = OpFunctionParameter %_ptr_Function_int
         %buf_l_f1_13 = OpFunctionParameter %_ptr_Function_int
         %buf_l_f1_14 = OpFunctionParameter %_ptr_Function_int
         %buf_l_f1_15 = OpFunctionParameter %_ptr_Function_int
         %buf_l_f1_17 = OpLabel
         %buf_l_f1_42 = OpVariable %_ptr_Function_int Function
         %buf_l_f1_43 = OpLoad %int %buf_l_f1_13
         %buf_l_f1_44 = OpLoad %int %buf_l_f1_12
         %buf_l_f1_45 = OpLoad %int %buf_l_f1_14
         %buf_l_f1_46 = OpIMul %int %buf_l_f1_44 %buf_l_f1_45
         %buf_l_f1_47 = OpIAdd %int %buf_l_f1_43 %buf_l_f1_46
         %buf_l_f1_49 = OpSDiv %int %buf_l_f1_47 %int_4
               OpStore %buf_l_f1_42 %buf_l_f1_49
         %buf_l_f1_57 = OpLoad %int %buf_l_f1_15
         %buf_l_f1_62 = OpAccessChain %_ptr_StorageBuffer_float %buf %buf_l_f1_57 %int_0 %buf_l_f1_49
         %buf_l_f1_63 = OpLoad %float %buf_l_f1_62
               OpStore %buf_l_f1_11 %buf_l_f1_63
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t BUFFER_LOAD_FLOAT4[] = UR"(
             ; Function buffer_load_float4
             ;void buffer_load_float4(out float p1, out float p2, out float p3, out float p4, in int index, 
             ;                                in int offset, in int stride, in int buffer_index)
             ;{
             ;	int addr = (offset + index * stride)/4;
             ;	p1 = buf[buffer_index].data[addr+0];
             ;	p2 = buf[buffer_index].data[addr+1];
             ;	p3 = buf[buffer_index].data[addr+2];
             ;	p4 = buf[buffer_index].data[addr+3];
             ;}
%buffer_load_float4 = OpFunction %void None %function_buffer_load_float4
  %buf_l_f4_21 = OpFunctionParameter %_ptr_Function_float
  %buf_l_f4_22 = OpFunctionParameter %_ptr_Function_float
  %buf_l_f4_23 = OpFunctionParameter %_ptr_Function_float
  %buf_l_f4_24 = OpFunctionParameter %_ptr_Function_float
  %buf_l_f4_25 = OpFunctionParameter %_ptr_Function_int
  %buf_l_f4_26 = OpFunctionParameter %_ptr_Function_int
  %buf_l_f4_27 = OpFunctionParameter %_ptr_Function_int
  %buf_l_f4_28 = OpFunctionParameter %_ptr_Function_int
  %buf_l_f4_30 = OpLabel
  %buf_l_f4_44 = OpVariable %_ptr_Function_int Function
  %buf_l_f4_45 = OpLoad %int %buf_l_f4_26
  %buf_l_f4_46 = OpLoad %int %buf_l_f4_25
  %buf_l_f4_47 = OpLoad %int %buf_l_f4_27
  %buf_l_f4_48 = OpIMul %int %buf_l_f4_46 %buf_l_f4_47 
  %buf_l_f4_49 = OpIAdd %int %buf_l_f4_45 %buf_l_f4_48 
  %buf_l_f4_51 = OpSDiv %int %buf_l_f4_49 %int_4 
        OpStore %buf_l_f4_44 %buf_l_f4_51
  %buf_l_f4_58 = OpLoad %int %buf_l_f4_28
  %buf_l_f4_63 = OpAccessChain %_ptr_StorageBuffer_float %buf %buf_l_f4_58 %int_0 %buf_l_f4_51
  %buf_l_f4_64 = OpLoad %float %buf_l_f4_63 
        OpStore %buf_l_f4_21 %buf_l_f4_64
  %buf_l_f4_65 = OpLoad %int %buf_l_f4_28
  %buf_l_f4_68 = OpIAdd %int %buf_l_f4_51 %int_1
  %buf_l_f4_69 = OpAccessChain %_ptr_StorageBuffer_float %buf %buf_l_f4_65 %int_0 %buf_l_f4_68
  %buf_l_f4_70 = OpLoad %float %buf_l_f4_69
        OpStore %buf_l_f4_22 %buf_l_f4_70
  %buf_l_f4_71 = OpLoad %int %buf_l_f4_28
  %buf_l_f4_74 = OpIAdd %int %buf_l_f4_51 %int_2
  %buf_l_f4_75 = OpAccessChain %_ptr_StorageBuffer_float %buf %buf_l_f4_71 %int_0 %buf_l_f4_74
  %buf_l_f4_76 = OpLoad %float %buf_l_f4_75
        OpStore %buf_l_f4_23 %buf_l_f4_76
  %buf_l_f4_77 = OpLoad %int %buf_l_f4_28
  %buf_l_f4_80 = OpIAdd %int %buf_l_f4_51 %int_3
  %buf_l_f4_81 = OpAccessChain %_ptr_StorageBuffer_float %buf %buf_l_f4_77 %int_0 %buf_l_f4_80
  %buf_l_f4_82 = OpLoad %float %buf_l_f4_81
        OpStore %buf_l_f4_24 %buf_l_f4_82
        OpReturn
        OpFunctionEnd 
			)";

constexpr char32_t BUFFER_STORE_FLOAT1[] = UR"(
             ; void buffer_store_float1(in float p1, in int index, in int offset, in int stride, in int buffer_index)
             ; {
             ; 	int addr = (offset + index * stride)/4;
             ; 	buf[buffer_index].data[addr+0] = p1;
             ; }
%buffer_store_float1 = OpFunction %void None %function_buffer_load_store_float1
         %buf_s_f1_18 = OpFunctionParameter %_ptr_Function_float
         %buf_s_f1_19 = OpFunctionParameter %_ptr_Function_int 
         %buf_s_f1_20 = OpFunctionParameter %_ptr_Function_int 
         %buf_s_f1_21 = OpFunctionParameter %_ptr_Function_int 
         %buf_s_f1_22 = OpFunctionParameter %_ptr_Function_int 
         %buf_s_f1_24 = OpLabel
         %buf_s_f1_64 = OpVariable %_ptr_Function_int Function 
         %buf_s_f1_65 = OpLoad %int %buf_s_f1_20
         %buf_s_f1_66 = OpLoad %int %buf_s_f1_19
         %buf_s_f1_67 = OpLoad %int %buf_s_f1_21
         %buf_s_f1_68 = OpIMul %int %buf_s_f1_66 %buf_s_f1_67
         %buf_s_f1_69 = OpIAdd %int %buf_s_f1_65 %buf_s_f1_68
         %buf_s_f1_70 = OpSDiv %int %buf_s_f1_69 %int_4
               OpStore %buf_s_f1_64 %buf_s_f1_70
         %buf_s_f1_71 = OpLoad %int %buf_s_f1_22
         %buf_s_f1_74 = OpLoad %float %buf_s_f1_18
         %buf_s_f1_75 = OpAccessChain %_ptr_StorageBuffer_float %buf %buf_s_f1_71 %int_0 %buf_s_f1_70
               OpStore %buf_s_f1_75 %buf_s_f1_74
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t TBUFFER_LOAD_FORMAT_XYZW[] = UR"(
             ; Function tbuffer_load_format_xyzw
             ; void tbuffer_load_format_xyzw(out float p1, out float p2, out float p3, out float p4, 
             ;                               in int index, in int offset, in int stride, in int buffer_index, in int dfmt_nfmt)
             ; {
             ; 	if (dfmt_nfmt == 119) // dfmt = 14, nfmt = 7
             ; 	{
             ; 		buffer_load_float4(p1, p2, p3, p4, index, offset, stride, buffer_index);
             ; 	}
             ; }
%tbuffer_load_format_xyzw = OpFunction %void None %function_tbuffer_load_format_xyzw
%tbuf_l_f_xyzw_54 = OpFunctionParameter %_ptr_Function_float
%tbuf_l_f_xyzw_55 = OpFunctionParameter %_ptr_Function_float
%tbuf_l_f_xyzw_56 = OpFunctionParameter %_ptr_Function_float
%tbuf_l_f_xyzw_57 = OpFunctionParameter %_ptr_Function_float
%tbuf_l_f_xyzw_58 = OpFunctionParameter %_ptr_Function_int
%tbuf_l_f_xyzw_59 = OpFunctionParameter %_ptr_Function_int
%tbuf_l_f_xyzw_60 = OpFunctionParameter %_ptr_Function_int
%tbuf_l_f_xyzw_61 = OpFunctionParameter %_ptr_Function_int
%tbuf_l_f_xyzw_62 = OpFunctionParameter %_ptr_Function_int
%tbuf_l_f_xyzw_64 = OpLabel
%tbuf_l_f_xyzw_166 = OpVariable %_ptr_Function_float Function
%tbuf_l_f_xyzw_167 = OpVariable %_ptr_Function_float Function
%tbuf_l_f_xyzw_168 = OpVariable %_ptr_Function_float Function
%tbuf_l_f_xyzw_169 = OpVariable %_ptr_Function_float Function
%tbuf_l_f_xyzw_170 = OpVariable %_ptr_Function_int Function
%tbuf_l_f_xyzw_172 = OpVariable %_ptr_Function_int Function
%tbuf_l_f_xyzw_174 = OpVariable %_ptr_Function_int Function
%tbuf_l_f_xyzw_176 = OpVariable %_ptr_Function_int Function
%tbuf_l_f_xyzw_161 = OpLoad %int %tbuf_l_f_xyzw_62
%tbuf_l_f_xyzw_163 = OpIEqual %bool %tbuf_l_f_xyzw_161 %int_119
   OpSelectionMerge %tbuf_l_f_xyzw_165 None
   OpBranchConditional %tbuf_l_f_xyzw_163 %tbuf_l_f_xyzw_164 %tbuf_l_f_xyzw_165
%tbuf_l_f_xyzw_164 = OpLabel
%tbuf_l_f_xyzw_171 = OpLoad %int %tbuf_l_f_xyzw_58
   OpStore %tbuf_l_f_xyzw_170 %tbuf_l_f_xyzw_171
%tbuf_l_f_xyzw_173 = OpLoad %int %tbuf_l_f_xyzw_59
   OpStore %tbuf_l_f_xyzw_172 %tbuf_l_f_xyzw_173
%tbuf_l_f_xyzw_175 = OpLoad %int %tbuf_l_f_xyzw_60
   OpStore %tbuf_l_f_xyzw_174 %tbuf_l_f_xyzw_175
%tbuf_l_f_xyzw_177 = OpLoad %int %tbuf_l_f_xyzw_61
   OpStore %tbuf_l_f_xyzw_176 %tbuf_l_f_xyzw_177
%tbuf_l_f_xyzw_178 = OpFunctionCall %void %buffer_load_float4 %tbuf_l_f_xyzw_166 %tbuf_l_f_xyzw_167 %tbuf_l_f_xyzw_168 %tbuf_l_f_xyzw_169 %tbuf_l_f_xyzw_170 %tbuf_l_f_xyzw_172 %tbuf_l_f_xyzw_174 %tbuf_l_f_xyzw_176
%tbuf_l_f_xyzw_179 = OpLoad %float %tbuf_l_f_xyzw_166
   OpStore %tbuf_l_f_xyzw_54 %tbuf_l_f_xyzw_179
%tbuf_l_f_xyzw_180 = OpLoad %float %tbuf_l_f_xyzw_167
   OpStore %tbuf_l_f_xyzw_55 %tbuf_l_f_xyzw_180
%tbuf_l_f_xyzw_181 = OpLoad %float %tbuf_l_f_xyzw_168
   OpStore %tbuf_l_f_xyzw_56 %tbuf_l_f_xyzw_181
%tbuf_l_f_xyzw_182 = OpLoad %float %tbuf_l_f_xyzw_169
   OpStore %tbuf_l_f_xyzw_57 %tbuf_l_f_xyzw_182
   OpBranch %tbuf_l_f_xyzw_165
%tbuf_l_f_xyzw_165 = OpLabel
   OpReturn
   OpFunctionEnd
			)";

constexpr char32_t TBUFFER_LOAD_FORMAT_X[] = UR"(
             ; void tbuffer_load_format_x(out float p1, in int index, in int offset, in int stride, in int buffer_index, in int dfmt_nfmt)
             ; {
             ; 	if (dfmt_nfmt == 36) // dmft = 4, nfmt = 4
             ; 	{
             ; 		buffer_load_float1(p1, index, offset, stride, buffer_index);
             ; 	}
             ; }
%tbuffer_load_format_x = OpFunction %void None %function_tbuffer_load_store_format_x
         %tbuf_l_f_x_26 = OpFunctionParameter %_ptr_Function_float
         %tbuf_l_f_x_27 = OpFunctionParameter %_ptr_Function_int 
         %tbuf_l_f_x_28 = OpFunctionParameter %_ptr_Function_int 
         %tbuf_l_f_x_29 = OpFunctionParameter %_ptr_Function_int 
         %tbuf_l_f_x_30 = OpFunctionParameter %_ptr_Function_int 
         %tbuf_l_f_x_31 = OpFunctionParameter %_ptr_Function_int 
         %tbuf_l_f_x_33 = OpLabel 
         %tbuf_l_f_x_82 = OpVariable %_ptr_Function_float Function
         %tbuf_l_f_x_83 = OpVariable %_ptr_Function_int Function
         %tbuf_l_f_x_85 = OpVariable %_ptr_Function_int Function
         %tbuf_l_f_x_87 = OpVariable %_ptr_Function_int Function
         %tbuf_l_f_x_89 = OpVariable %_ptr_Function_int Function
         %tbuf_l_f_x_76 = OpLoad %int %tbuf_l_f_x_31
         %tbuf_l_f_x_79 = OpIEqual %bool %tbuf_l_f_x_76 %int_36
               OpSelectionMerge %tbuf_l_f_x_81 None
               OpBranchConditional %tbuf_l_f_x_79 %tbuf_l_f_x_80 %tbuf_l_f_x_81
         %tbuf_l_f_x_80 = OpLabel
         %tbuf_l_f_x_84 = OpLoad %int %tbuf_l_f_x_27
               OpStore %tbuf_l_f_x_83 %tbuf_l_f_x_84
         %tbuf_l_f_x_86 = OpLoad %int %tbuf_l_f_x_28
               OpStore %tbuf_l_f_x_85 %tbuf_l_f_x_86
         %tbuf_l_f_x_88 = OpLoad %int %tbuf_l_f_x_29
               OpStore %tbuf_l_f_x_87 %tbuf_l_f_x_88
         %tbuf_l_f_x_90 = OpLoad %int %tbuf_l_f_x_30
               OpStore %tbuf_l_f_x_89 %tbuf_l_f_x_90
         %tbuf_l_f_x_91 = OpFunctionCall %void %buffer_load_float1 %tbuf_l_f_x_82 %tbuf_l_f_x_83 %tbuf_l_f_x_85 %tbuf_l_f_x_87 %tbuf_l_f_x_89
         %tbuf_l_f_x_92 = OpLoad %float %tbuf_l_f_x_82
               OpStore %tbuf_l_f_x_26 %tbuf_l_f_x_92
               OpBranch %tbuf_l_f_x_81
         %tbuf_l_f_x_81 = OpLabel
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t TBUFFER_STORE_FORMAT_X[] = UR"(
             ; void tbuffer_store_format_x(in float p1, in int index, in int offset, in int stride, in int buffer_index, in int dfmt_nfmt)
             ; {
             ; 	if (dfmt_nfmt == 36) // dmft = 4, nfmt = 4
             ; 	{
             ; 		buffer_store_float1(p1, index, offset, stride, buffer_index);
             ; 	}
             ; }
%tbuffer_store_format_x = OpFunction %void None %function_tbuffer_load_store_format_x
         %tbuf_s_f_x_34 = OpFunctionParameter %_ptr_Function_float 
         %tbuf_s_f_x_35 = OpFunctionParameter %_ptr_Function_int
         %tbuf_s_f_x_36 = OpFunctionParameter %_ptr_Function_int
         %tbuf_s_f_x_37 = OpFunctionParameter %_ptr_Function_int
         %tbuf_s_f_x_38 = OpFunctionParameter %_ptr_Function_int
         %tbuf_s_f_x_39 = OpFunctionParameter %_ptr_Function_int
         %tbuf_s_f_x_41 = OpLabel
         %tbuf_s_f_x_97 = OpVariable %_ptr_Function_float Function
         %tbuf_s_f_x_99 = OpVariable %_ptr_Function_int Function
        %tbuf_s_f_x_101 = OpVariable %_ptr_Function_int Function
        %tbuf_s_f_x_103 = OpVariable %_ptr_Function_int Function
        %tbuf_s_f_x_105 = OpVariable %_ptr_Function_int Function
         %tbuf_s_f_x_93 = OpLoad %int %tbuf_s_f_x_39
         %tbuf_s_f_x_94 = OpIEqual %bool %tbuf_s_f_x_93 %int_36
               OpSelectionMerge %tbuf_s_f_x_96 None
               OpBranchConditional %tbuf_s_f_x_94 %tbuf_s_f_x_95 %tbuf_s_f_x_96
         %tbuf_s_f_x_95 = OpLabel
         %tbuf_s_f_x_98 = OpLoad %float %tbuf_s_f_x_34
               OpStore %tbuf_s_f_x_97 %tbuf_s_f_x_98
        %tbuf_s_f_x_100 = OpLoad %int %tbuf_s_f_x_35
               OpStore %tbuf_s_f_x_99 %tbuf_s_f_x_100
        %tbuf_s_f_x_102 = OpLoad %int %tbuf_s_f_x_36
               OpStore %tbuf_s_f_x_101 %tbuf_s_f_x_102
        %tbuf_s_f_x_104 = OpLoad %int %tbuf_s_f_x_37
               OpStore %tbuf_s_f_x_103 %tbuf_s_f_x_104
        %tbuf_s_f_x_106 = OpLoad %int %tbuf_s_f_x_38
               OpStore %tbuf_s_f_x_105 %tbuf_s_f_x_106
        %tbuf_s_f_x_107 = OpFunctionCall %void %buffer_store_float1 %tbuf_s_f_x_97 %tbuf_s_f_x_99 %tbuf_s_f_x_101 %tbuf_s_f_x_103 %tbuf_s_f_x_105
               OpBranch %tbuf_s_f_x_96
         %tbuf_s_f_x_96 = OpLabel
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t SBUFFER_LOAD_DWORD[] = UR"(
                     ; void sbuffer_load_dword(out uint p1, in int offset, in int buffer_index)
                     ; {
                     ; 	int addr = offset/4;
                     ; 	p1 = floatBitsToUint(buf[buffer_index].data[addr+0]);
                     ; }
%sbuffer_load_dword = OpFunction %void None %function_sbuffer_load_dword
         %sbuf_dw_45 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw_46 = OpFunctionParameter %_ptr_Function_int
         %sbuf_dw_47 = OpFunctionParameter %_ptr_Function_int
         %sbuf_dw_49 = OpLabel
        %sbuf_dw_115 = OpVariable %_ptr_Function_int Function
        %sbuf_dw_116 = OpLoad %int %sbuf_dw_46
        %sbuf_dw_117 = OpSDiv %int %sbuf_dw_116 %int_4
               OpStore %sbuf_dw_115 %sbuf_dw_117
        %sbuf_dw_118 = OpLoad %int %sbuf_dw_47
        %sbuf_dw_121 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw_118 %int_0 %sbuf_dw_117
        %sbuf_dw_122 = OpLoad %float %sbuf_dw_121
        %sbuf_dw_123 = OpBitcast %uint %sbuf_dw_122
               OpStore %sbuf_dw_45 %sbuf_dw_123
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t SBUFFER_LOAD_DWORD_2[] = UR"(
                      ; void sbuffer_load_dwordx2(out uint p1, out uint p2, in int offset, in int buffer_index)
                      ; {
                      ; 	int addr = offset/4;
                      ; 	p1 = floatBitsToUint(buf[buffer_index].data[addr+0]);
                      ; 	p2 = floatBitsToUint(buf[buffer_index].data[addr+1]);
                      ; }
%sbuffer_load_dword_2 = OpFunction %void None %function_sbuffer_load_dword_2
         %sbuf_dw2_11 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw2_12 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw2_13 = OpFunctionParameter %_ptr_Function_int
         %sbuf_dw2_14 = OpFunctionParameter %_ptr_Function_int
         %sbuf_dw2_16 = OpLabel
         %sbuf_dw2_17 = OpVariable %_ptr_Function_int Function
         %sbuf_dw2_18 = OpLoad %int %sbuf_dw2_13
         %sbuf_dw2_20 = OpSDiv %int %sbuf_dw2_18 %int_4
               OpStore %sbuf_dw2_17 %sbuf_dw2_20
         %sbuf_dw2_28 = OpLoad %int %sbuf_dw2_14
         %sbuf_dw2_33 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw2_28 %int_0 %sbuf_dw2_20
         %sbuf_dw2_34 = OpLoad %float %sbuf_dw2_33
         %sbuf_dw2_35 = OpBitcast %uint %sbuf_dw2_34
               OpStore %sbuf_dw2_11 %sbuf_dw2_35
         %sbuf_dw2_36 = OpLoad %int %sbuf_dw2_14
         %sbuf_dw2_39 = OpIAdd %int %sbuf_dw2_20 %int_1
         %sbuf_dw2_40 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw2_36 %int_0 %sbuf_dw2_39
         %sbuf_dw2_41 = OpLoad %float %sbuf_dw2_40
         %sbuf_dw2_42 = OpBitcast %uint %sbuf_dw2_41
               OpStore %sbuf_dw2_12 %sbuf_dw2_42
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t SBUFFER_LOAD_DWORD_4[] = UR"(
                     ; void sbuffer_load_dwordx4(out uint p1, out uint p2, out uint p3, out uint p4, in int offset, in int buffer_index)
                     ; {
                     ; 	int addr = offset/4;
                     ; 	p1 = floatBitsToUint(buf[buffer_index].data[addr+0]);
                     ; 	p2 = floatBitsToUint(buf[buffer_index].data[addr+1]);
                     ; 	p3 = floatBitsToUint(buf[buffer_index].data[addr+2]);
                     ; 	p4 = floatBitsToUint(buf[buffer_index].data[addr+3]);
                     ; }
%sbuffer_load_dword_4 = OpFunction %void None %function_sbuffer_load_dword_4
         %sbuf_dw4_51 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw4_52 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw4_53 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw4_54 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw4_55 = OpFunctionParameter %_ptr_Function_int
         %sbuf_dw4_56 = OpFunctionParameter %_ptr_Function_int
         %sbuf_dw4_58 = OpLabel
        %sbuf_dw4_133 = OpVariable %_ptr_Function_int Function
        %sbuf_dw4_134 = OpLoad %int %sbuf_dw4_55
        %sbuf_dw4_135 = OpSDiv %int %sbuf_dw4_134 %int_4
               OpStore %sbuf_dw4_133 %sbuf_dw4_135
        %sbuf_dw4_136 = OpLoad %int %sbuf_dw4_56
        %sbuf_dw4_139 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw4_136 %int_0 %sbuf_dw4_135
        %sbuf_dw4_140 = OpLoad %float %sbuf_dw4_139
        %sbuf_dw4_141 = OpBitcast %uint %sbuf_dw4_140
               OpStore %sbuf_dw4_51 %sbuf_dw4_141
        %sbuf_dw4_142 = OpLoad %int %sbuf_dw4_56
        %sbuf_dw4_145 = OpIAdd %int %sbuf_dw4_135 %int_1
        %sbuf_dw4_146 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw4_142 %int_0 %sbuf_dw4_145
        %sbuf_dw4_147 = OpLoad %float %sbuf_dw4_146
        %sbuf_dw4_148 = OpBitcast %uint %sbuf_dw4_147
               OpStore %sbuf_dw4_52 %sbuf_dw4_148
        %sbuf_dw4_149 = OpLoad %int %sbuf_dw4_56
        %sbuf_dw4_152 = OpIAdd %int %sbuf_dw4_135 %int_2
        %sbuf_dw4_153 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw4_149 %int_0 %sbuf_dw4_152
        %sbuf_dw4_154 = OpLoad %float %sbuf_dw4_153
        %sbuf_dw4_155 = OpBitcast %uint %sbuf_dw4_154
               OpStore %sbuf_dw4_53 %sbuf_dw4_155
        %sbuf_dw4_156 = OpLoad %int %sbuf_dw4_56
        %sbuf_dw4_159 = OpIAdd %int %sbuf_dw4_135 %int_3
        %sbuf_dw4_160 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw4_156 %int_0 %sbuf_dw4_159
        %sbuf_dw4_161 = OpLoad %float %sbuf_dw4_160
        %sbuf_dw4_162 = OpBitcast %uint %sbuf_dw4_161
               OpStore %sbuf_dw4_54 %sbuf_dw4_162
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t SBUFFER_LOAD_DWORD_8[] = UR"(
                     ; void sbuffer_load_dwordx8(out uint p1, out uint p2, out uint p3, out uint p4, 
                     ;                           out uint p5, out uint p6, out uint p7, out uint p8, in int offset, in int buffer_index)
                     ; {
                     ; 	int addr = offset/4;
                     ; 	p1 = floatBitsToUint(buf[buffer_index].data[addr+0]);
                     ; 	p2 = floatBitsToUint(buf[buffer_index].data[addr+1]);
                     ; 	p3 = floatBitsToUint(buf[buffer_index].data[addr+2]);
                     ; 	p4 = floatBitsToUint(buf[buffer_index].data[addr+3]);
                     ; 	p5 = floatBitsToUint(buf[buffer_index].data[addr+4]);
                     ; 	p6 = floatBitsToUint(buf[buffer_index].data[addr+5]);
                     ; 	p7 = floatBitsToUint(buf[buffer_index].data[addr+6]);
                     ; 	p8 = floatBitsToUint(buf[buffer_index].data[addr+7]);
                     ; }
%sbuffer_load_dword_8 = OpFunction %void None %function_sbuffer_load_dword_8
         %sbuf_dw8_60 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw8_61 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw8_62 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw8_63 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw8_64 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw8_65 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw8_66 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw8_67 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw8_68 = OpFunctionParameter %_ptr_Function_int
         %sbuf_dw8_69 = OpFunctionParameter %_ptr_Function_int
         %sbuf_dw8_71 = OpLabel
        %sbuf_dw8_197 = OpVariable %_ptr_Function_int Function
        %sbuf_dw8_198 = OpLoad %int %sbuf_dw8_68
        %sbuf_dw8_199 = OpSDiv %int %sbuf_dw8_198 %int_4
               OpStore %sbuf_dw8_197 %sbuf_dw8_199
        %sbuf_dw8_200 = OpLoad %int %sbuf_dw8_69
        %sbuf_dw8_203 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw8_200 %int_0 %sbuf_dw8_199
        %sbuf_dw8_204 = OpLoad %float %sbuf_dw8_203
        %sbuf_dw8_205 = OpBitcast %uint %sbuf_dw8_204
               OpStore %sbuf_dw8_60 %sbuf_dw8_205
        %sbuf_dw8_206 = OpLoad %int %sbuf_dw8_69
        %sbuf_dw8_208 = OpIAdd %int %sbuf_dw8_199 %int_1
        %sbuf_dw8_209 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw8_206 %int_0 %sbuf_dw8_208
        %sbuf_dw8_210 = OpLoad %float %sbuf_dw8_209
        %sbuf_dw8_211 = OpBitcast %uint %sbuf_dw8_210
               OpStore %sbuf_dw8_61 %sbuf_dw8_211
        %sbuf_dw8_212 = OpLoad %int %sbuf_dw8_69
        %sbuf_dw8_214 = OpIAdd %int %sbuf_dw8_199 %int_2
        %sbuf_dw8_215 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw8_212 %int_0 %sbuf_dw8_214
        %sbuf_dw8_216 = OpLoad %float %sbuf_dw8_215
        %sbuf_dw8_217 = OpBitcast %uint %sbuf_dw8_216
               OpStore %sbuf_dw8_62 %sbuf_dw8_217
        %sbuf_dw8_218 = OpLoad %int %sbuf_dw8_69
        %sbuf_dw8_220 = OpIAdd %int %sbuf_dw8_199 %int_3
        %sbuf_dw8_221 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw8_218 %int_0 %sbuf_dw8_220
        %sbuf_dw8_222 = OpLoad %float %sbuf_dw8_221
        %sbuf_dw8_223 = OpBitcast %uint %sbuf_dw8_222
               OpStore %sbuf_dw8_63 %sbuf_dw8_223
        %sbuf_dw8_224 = OpLoad %int %sbuf_dw8_69
        %sbuf_dw8_226 = OpIAdd %int %sbuf_dw8_199 %int_4
        %sbuf_dw8_227 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw8_224 %int_0 %sbuf_dw8_226
        %sbuf_dw8_228 = OpLoad %float %sbuf_dw8_227
        %sbuf_dw8_229 = OpBitcast %uint %sbuf_dw8_228
               OpStore %sbuf_dw8_64 %sbuf_dw8_229
        %sbuf_dw8_230 = OpLoad %int %sbuf_dw8_69
        %sbuf_dw8_233 = OpIAdd %int %sbuf_dw8_199 %int_5
        %sbuf_dw8_234 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw8_230 %int_0 %sbuf_dw8_233
        %sbuf_dw8_235 = OpLoad %float %sbuf_dw8_234
        %sbuf_dw8_236 = OpBitcast %uint %sbuf_dw8_235
               OpStore %sbuf_dw8_65 %sbuf_dw8_236
        %sbuf_dw8_237 = OpLoad %int %sbuf_dw8_69
        %sbuf_dw8_240 = OpIAdd %int %sbuf_dw8_199 %int_6
        %sbuf_dw8_241 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw8_237 %int_0 %sbuf_dw8_240
        %sbuf_dw8_242 = OpLoad %float %sbuf_dw8_241
        %sbuf_dw8_243 = OpBitcast %uint %sbuf_dw8_242
               OpStore %sbuf_dw8_66 %sbuf_dw8_243
        %sbuf_dw8_244 = OpLoad %int %sbuf_dw8_69
        %sbuf_dw8_247 = OpIAdd %int %sbuf_dw8_199 %int_7
        %sbuf_dw8_248 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw8_244 %int_0 %sbuf_dw8_247
        %sbuf_dw8_249 = OpLoad %float %sbuf_dw8_248
        %sbuf_dw8_250 = OpBitcast %uint %sbuf_dw8_249
               OpStore %sbuf_dw8_67 %sbuf_dw8_250
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t SBUFFER_LOAD_DWORD_16[] = UR"(
                     ; void sbuffer_load_dwordx16(out uint p1, out uint p2, out uint p3, out uint p4, 
                     ;                            out uint p5, out uint p6, out uint p7, out uint p8,
                     ;                            out uint p9, out uint p10, out uint p11, out uint p12,
                     ;                            out uint p13, out uint p14, out uint p15, out uint p16, in int offset, in int buffer_index)
                     ; {
                     ; 	int addr = offset/4;
                     ; 	p1 = floatBitsToUint(buf[buffer_index].data[addr+0]);
                     ; 	p2 = floatBitsToUint(buf[buffer_index].data[addr+1]);
                     ; 	p3 = floatBitsToUint(buf[buffer_index].data[addr+2]);
                     ; 	p4 = floatBitsToUint(buf[buffer_index].data[addr+3]);
                     ; 	p5 = floatBitsToUint(buf[buffer_index].data[addr+4]);
                     ; 	p6 = floatBitsToUint(buf[buffer_index].data[addr+5]);
                     ; 	p7 = floatBitsToUint(buf[buffer_index].data[addr+6]);
                     ; 	p8 = floatBitsToUint(buf[buffer_index].data[addr+7]);
                     ; 	p9 = floatBitsToUint(buf[buffer_index].data[addr+8]);
                     ; 	p10 = floatBitsToUint(buf[buffer_index].data[addr+9]);
                     ; 	p11 = floatBitsToUint(buf[buffer_index].data[addr+10]);
                     ; 	p12 = floatBitsToUint(buf[buffer_index].data[addr+11]);
                     ; 	p13 = floatBitsToUint(buf[buffer_index].data[addr+12]);
                     ; 	p14 = floatBitsToUint(buf[buffer_index].data[addr+13]);
                     ; 	p15 = floatBitsToUint(buf[buffer_index].data[addr+14]);
                     ; 	p16 = floatBitsToUint(buf[buffer_index].data[addr+15]);
                     ; }
%sbuffer_load_dword_16 = OpFunction %void None %function_sbuffer_load_dword_16
         %sbuf_dw16_60 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_61 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_62 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_63 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_64 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_65 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_66 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_67 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_68 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_69 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_70 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_71 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_72 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_73 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_74 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_75 = OpFunctionParameter %_ptr_Function_uint
         %sbuf_dw16_76 = OpFunctionParameter %_ptr_Function_int
         %sbuf_dw16_77 = OpFunctionParameter %_ptr_Function_int
         %sbuf_dw16_79 = OpLabel
        %sbuf_dw16_184 = OpVariable %_ptr_Function_int Function
        %sbuf_dw16_185 = OpLoad %int %sbuf_dw16_76
        %sbuf_dw16_186 = OpSDiv %int %sbuf_dw16_185 %int_4
               OpStore %sbuf_dw16_184 %sbuf_dw16_186
        %sbuf_dw16_187 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_190 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_187 %int_0 %sbuf_dw16_186
        %sbuf_dw16_191 = OpLoad %float %sbuf_dw16_190
        %sbuf_dw16_192 = OpBitcast %uint %sbuf_dw16_191
               OpStore %sbuf_dw16_60 %sbuf_dw16_192
        %sbuf_dw16_193 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_195 = OpIAdd %int %sbuf_dw16_186 %int_1
        %sbuf_dw16_196 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_193 %int_0 %sbuf_dw16_195
        %sbuf_dw16_197 = OpLoad %float %sbuf_dw16_196
        %sbuf_dw16_198 = OpBitcast %uint %sbuf_dw16_197
               OpStore %sbuf_dw16_61 %sbuf_dw16_198
        %sbuf_dw16_199 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_201 = OpIAdd %int %sbuf_dw16_186 %int_2
        %sbuf_dw16_202 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_199 %int_0 %sbuf_dw16_201
        %sbuf_dw16_203 = OpLoad %float %sbuf_dw16_202
        %sbuf_dw16_204 = OpBitcast %uint %sbuf_dw16_203
               OpStore %sbuf_dw16_62 %sbuf_dw16_204
        %sbuf_dw16_205 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_207 = OpIAdd %int %sbuf_dw16_186 %int_3
        %sbuf_dw16_208 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_205 %int_0 %sbuf_dw16_207
        %sbuf_dw16_209 = OpLoad %float %sbuf_dw16_208
        %sbuf_dw16_210 = OpBitcast %uint %sbuf_dw16_209
               OpStore %sbuf_dw16_63 %sbuf_dw16_210
        %sbuf_dw16_211 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_213 = OpIAdd %int %sbuf_dw16_186 %int_4
        %sbuf_dw16_214 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_211 %int_0 %sbuf_dw16_213
        %sbuf_dw16_215 = OpLoad %float %sbuf_dw16_214
        %sbuf_dw16_216 = OpBitcast %uint %sbuf_dw16_215
               OpStore %sbuf_dw16_64 %sbuf_dw16_216
        %sbuf_dw16_217 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_220 = OpIAdd %int %sbuf_dw16_186 %int_5
        %sbuf_dw16_221 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_217 %int_0 %sbuf_dw16_220
        %sbuf_dw16_222 = OpLoad %float %sbuf_dw16_221
        %sbuf_dw16_223 = OpBitcast %uint %sbuf_dw16_222
               OpStore %sbuf_dw16_65 %sbuf_dw16_223
        %sbuf_dw16_224 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_227 = OpIAdd %int %sbuf_dw16_186 %int_6
        %sbuf_dw16_228 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_224 %int_0 %sbuf_dw16_227
        %sbuf_dw16_229 = OpLoad %float %sbuf_dw16_228
        %sbuf_dw16_230 = OpBitcast %uint %sbuf_dw16_229
               OpStore %sbuf_dw16_66 %sbuf_dw16_230
        %sbuf_dw16_231 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_234 = OpIAdd %int %sbuf_dw16_186 %int_7
        %sbuf_dw16_235 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_231 %int_0 %sbuf_dw16_234
        %sbuf_dw16_236 = OpLoad %float %sbuf_dw16_235
        %sbuf_dw16_237 = OpBitcast %uint %sbuf_dw16_236
               OpStore %sbuf_dw16_67 %sbuf_dw16_237
        %sbuf_dw16_238 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_241 = OpIAdd %int %sbuf_dw16_186 %int_8
        %sbuf_dw16_242 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_238 %int_0 %sbuf_dw16_241
        %sbuf_dw16_243 = OpLoad %float %sbuf_dw16_242
        %sbuf_dw16_244 = OpBitcast %uint %sbuf_dw16_243
               OpStore %sbuf_dw16_68 %sbuf_dw16_244
        %sbuf_dw16_245 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_248 = OpIAdd %int %sbuf_dw16_186 %int_9
        %sbuf_dw16_249 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_245 %int_0 %sbuf_dw16_248
        %sbuf_dw16_250 = OpLoad %float %sbuf_dw16_249
        %sbuf_dw16_251 = OpBitcast %uint %sbuf_dw16_250
               OpStore %sbuf_dw16_69 %sbuf_dw16_251
        %sbuf_dw16_252 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_255 = OpIAdd %int %sbuf_dw16_186 %int_10
        %sbuf_dw16_256 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_252 %int_0 %sbuf_dw16_255
        %sbuf_dw16_257 = OpLoad %float %sbuf_dw16_256
        %sbuf_dw16_258 = OpBitcast %uint %sbuf_dw16_257
               OpStore %sbuf_dw16_70 %sbuf_dw16_258
        %sbuf_dw16_259 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_262 = OpIAdd %int %sbuf_dw16_186 %int_11
        %sbuf_dw16_263 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_259 %int_0 %sbuf_dw16_262
        %sbuf_dw16_264 = OpLoad %float %sbuf_dw16_263
        %sbuf_dw16_265 = OpBitcast %uint %sbuf_dw16_264
               OpStore %sbuf_dw16_71 %sbuf_dw16_265
        %sbuf_dw16_266 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_269 = OpIAdd %int %sbuf_dw16_186 %int_12
        %sbuf_dw16_270 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_266 %int_0 %sbuf_dw16_269
        %sbuf_dw16_271 = OpLoad %float %sbuf_dw16_270
        %sbuf_dw16_272 = OpBitcast %uint %sbuf_dw16_271
               OpStore %sbuf_dw16_72 %sbuf_dw16_272
        %sbuf_dw16_273 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_276 = OpIAdd %int %sbuf_dw16_186 %int_13
        %sbuf_dw16_277 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_273 %int_0 %sbuf_dw16_276
        %sbuf_dw16_278 = OpLoad %float %sbuf_dw16_277
        %sbuf_dw16_279 = OpBitcast %uint %sbuf_dw16_278
               OpStore %sbuf_dw16_73 %sbuf_dw16_279
        %sbuf_dw16_280 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_283 = OpIAdd %int %sbuf_dw16_186 %int_14
        %sbuf_dw16_284 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_280 %int_0 %sbuf_dw16_283
        %sbuf_dw16_285 = OpLoad %float %sbuf_dw16_284
        %sbuf_dw16_286 = OpBitcast %uint %sbuf_dw16_285
               OpStore %sbuf_dw16_74 %sbuf_dw16_286
        %sbuf_dw16_287 = OpLoad %int %sbuf_dw16_77
        %sbuf_dw16_290 = OpIAdd %int %sbuf_dw16_186 %int_15
        %sbuf_dw16_291 = OpAccessChain %_ptr_StorageBuffer_float %buf %sbuf_dw16_287 %int_0 %sbuf_dw16_290
        %sbuf_dw16_292 = OpLoad %float %sbuf_dw16_291
        %sbuf_dw16_293 = OpBitcast %uint %sbuf_dw16_292
               OpStore %sbuf_dw16_75 %sbuf_dw16_293
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t EMBEDDED_SHADER_VS_0[] = UR"(
               ; #version 450
               ; 
               ; void main() 
               ; {
               ; 	float x = gl_VertexIndex == 0 || gl_VertexIndex == 2 ? -1.0 : 1.0;
               ; 	float y = gl_VertexIndex == 2 || gl_VertexIndex == 3 ? -1.0 : 1.0;
               ; 
               ;     gl_Position = vec4(x,y, 0.0, 1.0);
               ; }

               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Vertex %4 "main" %gl_VertexIndex %43

               ; Annotations
               OpDecorate %gl_VertexIndex BuiltIn VertexIndex
               OpMemberDecorate %_struct_41 0 BuiltIn Position
               OpMemberDecorate %_struct_41 1 BuiltIn PointSize
               OpMemberDecorate %_struct_41 2 BuiltIn ClipDistance
               OpMemberDecorate %_struct_41 3 BuiltIn CullDistance
               OpDecorate %_struct_41 Block

               ; Types, variables and constants
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
%_ptr_Function_float = OpTypePointer Function %float
       %bool = OpTypeBool
        %int = OpTypeInt 32 1
%_ptr_Input_int = OpTypePointer Input %int
%gl_VertexIndex = OpVariable %_ptr_Input_int Input
      %int_0 = OpConstant %int 0
      %int_2 = OpConstant %int 2
   %float_n1 = OpConstant %float -1
    %float_1 = OpConstant %float 1
      %int_3 = OpConstant %int 3
    %v4float = OpTypeVector %float 4
       %uint = OpTypeInt 32 0
     %uint_1 = OpConstant %uint 1
%_arr_float_uint_1 = OpTypeArray %float %uint_1
 %_struct_41 = OpTypeStruct %v4float %float %_arr_float_uint_1 %_arr_float_uint_1
%_ptr_Output__struct_41 = OpTypePointer Output %_struct_41
         %43 = OpVariable %_ptr_Output__struct_41 Output
    %float_0 = OpConstant %float 0
%_ptr_Output_v4float = OpTypePointer Output %v4float

               ; Function 4
          %4 = OpFunction %void None %3
          %5 = OpLabel
          %8 = OpVariable %_ptr_Function_float Function
         %26 = OpVariable %_ptr_Function_float Function
         %13 = OpLoad %int %gl_VertexIndex
         %15 = OpIEqual %bool %13 %int_0
         %16 = OpLogicalNot %bool %15
               OpSelectionMerge %18 None
               OpBranchConditional %16 %17 %18
         %17 = OpLabel
         %21 = OpIEqual %bool %13 %int_2
               OpBranch %18
         %18 = OpLabel
         %22 = OpPhi %bool %15 %5 %21 %17
         %25 = OpSelect %float %22 %float_n1 %float_1
               OpStore %8 %25
         %28 = OpIEqual %bool %13 %int_2
         %29 = OpLogicalNot %bool %28
               OpSelectionMerge %31 None
               OpBranchConditional %29 %30 %31
         %30 = OpLabel
         %34 = OpIEqual %bool %13 %int_3
               OpBranch %31
         %31 = OpLabel
         %35 = OpPhi %bool %28 %18 %34 %30
         %36 = OpSelect %float %35 %float_n1 %float_1
               OpStore %26 %36
         %47 = OpCompositeConstruct %v4float %25 %36 %float_0 %float_1
         %49 = OpAccessChain %_ptr_Output_v4float %43 %int_0
               OpStore %49 %47
               OpReturn
               OpFunctionEnd
)";

constexpr char32_t EXECZ[] = UR"(
        %z191_<index> = OpLoad %uint %exec_lo
        %z192_<index> = OpIEqual %bool %z191_<index> %uint_0
        %z193_<index> = OpLoad %uint %exec_hi
        %z194_<index> = OpIEqual %bool %z193_<index> %uint_0
        %z195_<index> = OpLogicalAnd %bool %z192_<index> %z194_<index>
        %z196_<index> = OpSelect %uint %z195_<index> %uint_1 %uint_0
               OpStore %execz %z196_<index>
)";

constexpr char32_t SCC_NZ_1[] = UR"(
        %snz1_118_<index> = OpLoad %uint %<dst>
        %snz1_121_<index> = OpINotEqual %bool %snz1_118_<index> %uint_0
        %snz1_123_<index> = OpSelect %uint %snz1_121_<index> %uint_1 %uint_0
               OpStore %scc %snz1_123_<index>
)";

constexpr char32_t SCC_NZ_2[] = UR"(
        %snz2_124_<index> = OpLoad %uint %<dst0>
        %snz2_125_<index> = OpINotEqual %bool %snz2_124_<index> %uint_0
        %snz2_127_<index> = OpLoad %uint %<dst1>
        %snz2_128_<index> = OpINotEqual %bool %snz2_127_<index> %uint_0
        %snz2_129_<index> = OpLogicalOr %bool %snz2_125_<index> %snz2_128_<index>
        %snz2_130_<index> = OpSelect %uint %snz2_129_<index> %uint_1 %uint_0
               OpStore %scc %snz2_130_<index>
)";

constexpr char32_t SCC_OVERFLOW_1[] = UR"(
        %so1_124_<index> = OpExtInst %int %GLSL_std_450 SSign %t0_<index>
        %so1_127_<index> = OpExtInst %int %GLSL_std_450 SSign %t1_<index>
        %so1_129_<index> = OpLoad %uint %<dst>
        %so1_130_<index> = OpBitcast %int %so1_129_<index>
        %so1_131_<index> = OpExtInst %int %GLSL_std_450 SSign %so1_130_<index>
        %so1_135_<index> = OpIEqual %bool %so1_124_<index> %so1_127_<index>
        %so1_138_<index> = OpINotEqual %bool %so1_131_<index> %so1_124_<index>
        %so1_139_<index> = OpLogicalAnd %bool %so1_135_<index> %so1_138_<index>
        %so1_142_<index> = OpSelect %uint %so1_139_<index> %uint_1 %uint_0
               OpStore %scc %so1_142_<index>
)";

constexpr char32_t CLAMP[] = UR"(
		%c197_<index> = OpLoad %float %<dst>
        %c200_<index> = OpExtInst %float %GLSL_std_450 FClamp %c197_<index> %float_0_000000 %float_1_000000
               OpStore %<dst> %c200_<index>
)";

constexpr char32_t MULTIPLY[] = UR"(
		%m197_<index> = OpLoad %float %<dst>
        %m200_<index> = OpFMul %float %m197_<index> %<mul>
               OpStore %<dst> %m200_<index>
)";

class Spirv;

enum class SccCheck
{
	None,
	NonZero,
	Overflow,
};

using inst_recompile_func_t = bool (*)(KYTY_RECOMPILER_ARGS);

enum class SpirvType
{
	Unknown,
	Float,
	Int,
	Uint
};

struct SpirvValue
{
	SpirvType type = SpirvType::Unknown;
	String    value;
};

class Spirv
{
public:
	Spirv()          = default;
	virtual ~Spirv() = default;
	KYTY_CLASS_DEFAULT_COPY(Spirv);

	[[nodiscard]] const ShaderCode& GetCode() const { return m_code; }
	ShaderCode&                     GetCode() { return m_code; }
	void                            SetCode(const ShaderCode& m_code) { this->m_code = m_code; }

	void GenerateSource();

	[[nodiscard]] const String& GetSource() const { return m_source; }

	void                                       SetVsInputInfo(const ShaderVertexInputInfo* input_info) { m_vs_input_info = input_info; }
	[[nodiscard]] const ShaderVertexInputInfo* GetVsInputInfo() const { return m_vs_input_info; }

	void                                        SetCsInputInfo(const ShaderComputeInputInfo* input_info) { m_cs_input_info = input_info; }
	[[nodiscard]] const ShaderComputeInputInfo* GetCsInputInfo() const { return m_cs_input_info; }

	void                                      SetPsInputInfo(const ShaderPixelInputInfo* input_info) { m_ps_input_info = input_info; }
	[[nodiscard]] const ShaderPixelInputInfo* GetPsInputInfo() const { return m_ps_input_info; }

	[[nodiscard]] const ShaderResources* GetBindInfo() const { return m_bind; }

	void                 AddConstantUint(uint32_t u);
	void                 AddConstantInt(int i);
	void                 AddConstantFloat(float f);
	void                 AddConstant(ShaderOperand op);
	[[nodiscard]] String GetConstantUint(uint32_t u) const;
	[[nodiscard]] String GetConstantInt(int i) const;
	[[nodiscard]] String GetConstantFloat(float f) const;
	[[nodiscard]] String GetConstant(ShaderOperand op) const;

	void GetMappedIndex(int offset, int* buffer, int* field) const
	{
		EXIT_NOT_IMPLEMENTED(offset >= m_extended_mapping.Size());
		*buffer = m_extended_mapping[offset][0];
		*field  = m_extended_mapping[offset][1];
	}

private:
	struct Variable
	{
		ShaderOperand op;
	};

	struct Constant
	{
		SpirvType      type     = SpirvType::Unknown;
		ShaderConstant constant = {0};
		String         type_str;
		String         value_str;
		String         id;
	};

	void AddConstant(SpirvType type, ShaderConstant constant);
	void AddVariable(ShaderOperand op);
	void AddVariable(ShaderOperandType type, int register_id, int size);

	void WriteHeader();
	void WriteAnnotations();
	void WriteTypes();
	void WriteConstants();
	void WriteGlobalVariables();
	void WriteMainProlog();
	void WriteLocalVariables();
	void WriteInstructions();
	void WriteMainEpilog();
	void WriteFunctions();

	void FindConstants();
	void FindVariables();

	String                        m_source;
	ShaderCode                    m_code;
	Vector<Constant>              m_constants;
	Vector<Variable>              m_variables;
	const ShaderVertexInputInfo*  m_vs_input_info = nullptr;
	const ShaderComputeInputInfo* m_cs_input_info = nullptr;
	const ShaderPixelInputInfo*   m_ps_input_info = nullptr;
	const ShaderResources*        m_bind          = nullptr;

	Core::Array2<int, 64, 2> m_extended_mapping {};
};

// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
struct RecompilerFunc
{
	inst_recompile_func_t           func      = nullptr;
	ShaderInstructionType           type      = ShaderInstructionType::Unknown;
	ShaderInstructionFormat::Format format    = ShaderInstructionFormat::Unknown;
	const char32_t*                 param[4]  = {nullptr, nullptr, nullptr, nullptr};
	SccCheck                        scc_check = SccCheck::None;
};

static bool operand_is_constant(ShaderOperand op)
{
	return (op.type == ShaderOperandType::LiteralConstant || op.type == ShaderOperandType::IntegerInlineConstant ||
	        op.type == ShaderOperandType::FloatInlineConstant);
}

static bool operand_is_variable(ShaderOperand op)
{
	return (op.type == ShaderOperandType::Vgpr || op.type == ShaderOperandType::VccLo || op.type == ShaderOperandType::VccHi ||
	        op.type == ShaderOperandType::Sgpr || op.type == ShaderOperandType::ExecLo || op.type == ShaderOperandType::ExecHi ||
	        op.type == ShaderOperandType::ExecZ || op.type == ShaderOperandType::Scc || op.type == ShaderOperandType::M0);
}

static SpirvValue operand_variable_to_str(ShaderOperand op)
{
	SpirvValue ret;

	EXIT_IF(op.size != 1);

	switch (op.type)
	{
		case ShaderOperandType::Vgpr:
			ret.value = String::FromPrintf("v%d", op.register_id);
			ret.type  = SpirvType::Float;
			break;
		case ShaderOperandType::Sgpr:
			ret.value = String::FromPrintf("s%d", op.register_id);
			ret.type  = SpirvType::Uint;
			break;
		case ShaderOperandType::VccLo:
			ret.value = U"vcc_lo";
			ret.type  = SpirvType::Uint;
			break;
		case ShaderOperandType::VccHi:
			ret.value = U"vcc_hi";
			ret.type  = SpirvType::Uint;
			break;
		case ShaderOperandType::ExecLo:
			ret.value = U"exec_lo";
			ret.type  = SpirvType::Uint;
			break;
		case ShaderOperandType::ExecHi:
			ret.value = U"exec_hi";
			ret.type  = SpirvType::Uint;
			break;
		case ShaderOperandType::ExecZ:
			ret.value = U"execz";
			ret.type  = SpirvType::Uint;
			break;
		case ShaderOperandType::Scc:
			ret.value = U"scc";
			ret.type  = SpirvType::Uint;
			break;
		case ShaderOperandType::M0:
			ret.value = U"m0";
			ret.type  = SpirvType::Uint;
			break;
		default: break;
	}

	return ret;
}

static SpirvValue operand_variable_to_str(ShaderOperand op, int shift)
{
	SpirvValue ret;

	EXIT_IF(op.size < 2 || op.size <= shift);

	switch (op.type)
	{
		case ShaderOperandType::Vgpr:
			ret.value = String::FromPrintf("v%d", op.register_id + shift);
			ret.type  = SpirvType::Float;
			break;
		case ShaderOperandType::Sgpr:
			ret.value = String::FromPrintf("s%d", op.register_id + shift);
			ret.type  = SpirvType::Uint;
			break;
		case ShaderOperandType::VccLo:
			if (shift == 0)
			{
				ret.value = U"vcc_lo";
				ret.type  = SpirvType::Uint;
			} else if (shift == 1)
			{
				ret.value = U"vcc_hi";
				ret.type  = SpirvType::Uint;
			}
			break;
		case ShaderOperandType::ExecLo:
			if (shift == 0)
			{
				ret.value = U"exec_lo";
				ret.type  = SpirvType::Uint;
			} else if (shift == 1)
			{
				ret.value = U"exec_hi";
				ret.type  = SpirvType::Uint;
			}
			break;
		default: break;
	}

	return ret;
}

static bool operand_is_exec(ShaderOperand op)
{
	switch (op.type)
	{
		case ShaderOperandType::ExecLo:
		case ShaderOperandType::ExecHi:
		case ShaderOperandType::ExecZ: return true;
		default: break;
	}
	return false;
}

static bool operand_load_int(Spirv* spirv, ShaderOperand op, const String& result_id, const String& index, String* load)
{
	EXIT_IF(load == nullptr);

	EXIT_NOT_IMPLEMENTED(op.negate);

	if (operand_is_constant(op))
	{
		String id = spirv->GetConstant(op);

		*load = String(U"%<result_id> = OpBitcast %int %<id>")
		            .ReplaceStr(U"<index>", index)
		            .ReplaceStr(U"<id>", id)
		            .ReplaceStr(U"<result_id>", result_id);
	} else if (operand_is_variable(op))
	{
		auto value = operand_variable_to_str(op);

		if (value.type == SpirvType::Float)
		{
			*load = (String(U"%t<result_id> = OpLoad %float %<id>\n") + String(U' ', 10) +
			         String(U"%<result_id> = OpBitcast %int %t<result_id>\n"))
			            .ReplaceStr(U"<index>", index)
			            .ReplaceStr(U"<id>", value.value)
			            .ReplaceStr(U"<result_id>", result_id);
		} else if (value.type == SpirvType::Uint)
		{
			*load = (String(U"%t<result_id> = OpLoad %uint %<id>\n") + String(U' ', 10) +
			         String(U"%<result_id> = OpBitcast %int %t<result_id>\n"))
			            .ReplaceStr(U"<index>", index)
			            .ReplaceStr(U"<id>", value.value)
			            .ReplaceStr(U"<result_id>", result_id);
		}
	} else
	{
		return false;
	}
	return true;
}

static bool operand_load_uint(Spirv* spirv, ShaderOperand op, const String& result_id, const String& index, String* load, int shift = -1)
{
	EXIT_IF(load == nullptr);

	EXIT_NOT_IMPLEMENTED(op.negate);

	if (operand_is_constant(op))
	{
		String id = spirv->GetConstant(op);

		*load = String(U"%<result_id> = OpBitcast %uint %<id>")
		            .ReplaceStr(U"<index>", index)
		            .ReplaceStr(U"<id>", id)
		            .ReplaceStr(U"<result_id>", result_id);
	} else if (operand_is_variable(op))
	{
		auto value = (shift >= 0 ? operand_variable_to_str(op, shift) : operand_variable_to_str(op));

		if (value.type == SpirvType::Float)
		{
			*load = (String(U"%t<result_id> = OpLoad %float %<id>\n") + String(U' ', 10) +
			         String(U"%<result_id> = OpBitcast %uint %t<result_id>\n"))
			            .ReplaceStr(U"<index>", index)
			            .ReplaceStr(U"<id>", value.value)
			            .ReplaceStr(U"<result_id>", result_id);
		} else if (value.type == SpirvType::Uint)
		{
			*load = (String(U"%<result_id> = OpLoad %uint %<id>"))
			            .ReplaceStr(U"<index>", index)
			            .ReplaceStr(U"<id>", value.value)
			            .ReplaceStr(U"<result_id>", result_id);
		} else
		{
			return false;
		}

	} else
	{
		return false;
	}
	return true;
}

static bool operand_load_float(Spirv* spirv, ShaderOperand op, const String& result_id, const String& index, String* load)
{
	EXIT_IF(load == nullptr);

	// EXIT_NOT_IMPLEMENTED(op.negate);

	String l;

	if (operand_is_constant(op))
	{
		String id = spirv->GetConstant(op);

		l = String(U"%<result_id> = OpBitcast %float %<id>").ReplaceStr(U"<id>", id);
	} else if (operand_is_variable(op))
	{
		auto value = operand_variable_to_str(op);

		if (value.type == SpirvType::Float)
		{
			l = String(U"%<result_id> = OpLoad %float %<id>\n").ReplaceStr(U"<id>", value.value);
		} else if (value.type == SpirvType::Uint)
		{
			l = (String(U"%t<result_id> = OpLoad %uint %<id>\n") + String(U' ', 10) +
			     String(U"%<result_id> = OpBitcast %float %t<result_id>\n"))
			        .ReplaceStr(U"<id>", value.value);
		} else
		{
			return false;
		}
	} else
	{
		return false;
	}

	if (op.negate)
	{
		l += String(U' ', 10) + String(U"%<result> = OpFNegate %float %<result_id>\n");
		*load = l.ReplaceStr(U"<index>", index).ReplaceStr(U"<result_id>", U"n" + result_id).ReplaceStr(U"<result>", result_id);
	} else
	{
		*load = l.ReplaceStr(U"<index>", index).ReplaceStr(U"<result_id>", result_id);
	}

	return true;
}

static String get_scc_check(SccCheck scc_check, int dst_num)
{
	EXIT_IF(dst_num < 1 || dst_num > 2);

	if (dst_num == 1)
	{
		switch (scc_check)
		{
			case SccCheck::NonZero: return SCC_NZ_1; break;
			case SccCheck::Overflow: return SCC_OVERFLOW_1; break;
			default: break;
		}
	} else if (dst_num == 2)
	{
		switch (scc_check)
		{
			case SccCheck::NonZero: return SCC_NZ_2; break;
			case SccCheck::Overflow: KYTY_NOT_IMPLEMENTED; break;
			default: break;
		}
	}
	return U"";
}

KYTY_RECOMPILER_FUNC(Recompile_BufferLoadDword_Vdata1VaddrSvSoffsIdxen)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[2]));

		auto dst_value   = operand_variable_to_str(inst.dst);
		auto src0_value  = operand_variable_to_str(inst.src[0]);
		auto src1_value0 = operand_variable_to_str(inst.src[1], 0);
		auto src1_value1 = operand_variable_to_str(inst.src[1], 1);
		// auto   src1_value3 = operand_variable_to_str(inst.src[1], 3);
		String offset = spirv->GetConstant(inst.src[2]);

		EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src0_value.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src1_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src1_value1.type != SpirvType::Uint);
		// EXIT_NOT_IMPLEMENTED(src1_value3.type != SpirvType::Uint);

		// TODO() check VSKIP
		// TODO() check EXEC

		static const char32_t* text = UR"(
        %t100_<index> = OpLoad %float %<src0>
        %t101_<index> = OpBitcast %int %t100_<index>
               OpStore %temp_int_1 %t101_<index>
        %t148_<index> = OpLoad %uint %<src1_value1>
        %t150_<index> = OpShiftRightLogical %uint %t148_<index> %int_16
        %t152_<index> = OpBitwiseAnd %uint %t150_<index> %uint_0x00003fff
        %t153_<index> = OpBitcast %int %t152_<index>
               OpStore %temp_int_3 %t153_<index>
        %t155_<index> = OpLoad %uint %<src1_value0>
        %t156_<index> = OpBitcast %int %t155_<index>
               OpStore %temp_int_4 %t156_<index>                
               OpStore %temp_int_2 %<offset>
		;%t206_<index> = OpLoad %uint %<src1_value3>
        ;%t208_<index> = OpShiftRightLogical %uint %t206_<index> %int_12
        ;%t210_<index> = OpBitwiseAnd %uint %t208_<index> %uint_127
        ;%t211_<index> = OpBitcast %int %t210_<index>
        ;       OpStore %temp_int_5 %t211_<index>
        %t110_<index> = OpFunctionCall %void %buffer_load_float1 %<p0> %temp_int_1 %temp_int_2 %temp_int_3 %temp_int_4 
)";
		*dst_source += String(text)
		                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<src0>", src0_value.value)
		                   .ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<src1_value0>", src1_value0.value)
		                   .ReplaceStr(U"<src1_value1>", src1_value1.value)
		                   //.ReplaceStr(U"<src1_value3>", src1_value3.value)
		                   .ReplaceStr(U"<p0>", dst_value.value);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_BufferLoadFormatX_Vdata1VaddrSvSoffsIdxen)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		// EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[0]));
		// EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
		EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[2]));

		auto   dst_value   = operand_variable_to_str(inst.dst);
		auto   src0_value  = operand_variable_to_str(inst.src[0]);
		auto   src1_value0 = operand_variable_to_str(inst.src[1], 0);
		auto   src1_value1 = operand_variable_to_str(inst.src[1], 1);
		auto   src1_value3 = operand_variable_to_str(inst.src[1], 3);
		String offset      = spirv->GetConstant(inst.src[2]);

		EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src0_value.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src1_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src1_value1.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src1_value3.type != SpirvType::Uint);

		// TODO() check VSKIP
		// TODO() check EXEC

		static const char32_t* text = UR"(
        %t100_<index> = OpLoad %float %<src0>
        %t101_<index> = OpBitcast %int %t100_<index>
               OpStore %temp_int_1 %t101_<index>
        %t148_<index> = OpLoad %uint %<src1_value1>
        %t150_<index> = OpShiftRightLogical %uint %t148_<index> %int_16
        %t152_<index> = OpBitwiseAnd %uint %t150_<index> %uint_0x00003fff
        %t153_<index> = OpBitcast %int %t152_<index>
               OpStore %temp_int_3 %t153_<index>
        %t155_<index> = OpLoad %uint %<src1_value0>
        %t156_<index> = OpBitcast %int %t155_<index>
               OpStore %temp_int_4 %t156_<index>                
               OpStore %temp_int_2 %<offset>
		%t206_<index> = OpLoad %uint %<src1_value3>
        %t208_<index> = OpShiftRightLogical %uint %t206_<index> %int_12
        %t210_<index> = OpBitwiseAnd %uint %t208_<index> %uint_127
        %t211_<index> = OpBitcast %int %t210_<index>
               OpStore %temp_int_5 %t211_<index>
        %t110_<index> = OpFunctionCall %void %tbuffer_load_format_x %<p0> %temp_int_1 %temp_int_2 %temp_int_3 %temp_int_4 %temp_int_5 
)";
		*dst_source += String(text)
		                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<src0>", src0_value.value)
		                   .ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<src1_value0>", src1_value0.value)
		                   .ReplaceStr(U"<src1_value1>", src1_value1.value)
		                   .ReplaceStr(U"<src1_value3>", src1_value3.value)
		                   .ReplaceStr(U"<p0>", dst_value.value);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_BufferStoreDword_Vdata1VaddrSvSoffsIdxen)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[2]));

		auto dst_value   = operand_variable_to_str(inst.dst);
		auto src0_value  = operand_variable_to_str(inst.src[0]);
		auto src1_value0 = operand_variable_to_str(inst.src[1], 0);
		auto src1_value1 = operand_variable_to_str(inst.src[1], 1);
		// auto   src1_value3 = operand_variable_to_str(inst.src[1], 3);
		String offset = spirv->GetConstant(inst.src[2]);

		EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src0_value.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src1_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src1_value1.type != SpirvType::Uint);
		// EXIT_NOT_IMPLEMENTED(src1_value3.type != SpirvType::Uint);

		// TODO() check VSKIP

		static const char32_t* text = UR"(
        %exec_lo_u_<index> = OpLoad %uint %exec_lo
        %exec_hi_u_<index> = OpLoad %uint %exec_hi ; unused
        %exec_lo_b_<index> = OpINotEqual %bool %exec_lo_u_<index> %uint_0
               OpSelectionMerge %t278_<index> None
               OpBranchConditional %exec_lo_b_<index> %t277_<index> %t278_<index>       
		%t277_<index> = OpLabel

        %t100_<index> = OpLoad %float %<src0>
        %t101_<index> = OpBitcast %int %t100_<index>
               OpStore %temp_int_1 %t101_<index>
        %t148_<index> = OpLoad %uint %<src1_value1>
        %t150_<index> = OpShiftRightLogical %uint %t148_<index> %int_16
        %t152_<index> = OpBitwiseAnd %uint %t150_<index> %uint_0x00003fff
        %t153_<index> = OpBitcast %int %t152_<index>
               OpStore %temp_int_3 %t153_<index>
        %t155_<index> = OpLoad %uint %<src1_value0>
        %t156_<index> = OpBitcast %int %t155_<index>
               OpStore %temp_int_4 %t156_<index>                
               OpStore %temp_int_2 %<offset>
		;%t206_<index> = OpLoad %uint %<src1_value3>
        ;%t208_<index> = OpShiftRightLogical %uint %t206_<index> %int_12
        ;%t210_<index> = OpBitwiseAnd %uint %t208_<index> %uint_127
        ;%t211_<index> = OpBitcast %int %t210_<index>
        ;       OpStore %temp_int_5 %t211_<index>
        %t110_<index> = OpFunctionCall %void %buffer_store_float1 %<p0> %temp_int_1 %temp_int_2 %temp_int_3 %temp_int_4 

               OpBranch %t278_<index>			   
        %t278_<index> = OpLabel
)";
		*dst_source += String(text)
		                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<src0>", src0_value.value)
		                   .ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<src1_value0>", src1_value0.value)
		                   .ReplaceStr(U"<src1_value1>", src1_value1.value)
		                   //.ReplaceStr(U"<src1_value3>", src1_value3.value)
		                   .ReplaceStr(U"<p0>", dst_value.value);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_BufferStoreFormatX_Vdata1VaddrSvSoffsIdxen)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		// EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[0]));
		// EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
		EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[2]));

		auto   dst_value   = operand_variable_to_str(inst.dst);
		auto   src0_value  = operand_variable_to_str(inst.src[0]);
		auto   src1_value0 = operand_variable_to_str(inst.src[1], 0);
		auto   src1_value1 = operand_variable_to_str(inst.src[1], 1);
		auto   src1_value3 = operand_variable_to_str(inst.src[1], 3);
		String offset      = spirv->GetConstant(inst.src[2]);

		EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src0_value.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src1_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src1_value1.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src1_value3.type != SpirvType::Uint);

		// TODO() check VSKIP

		static const char32_t* text = UR"(
        %exec_lo_u_<index> = OpLoad %uint %exec_lo
        %exec_hi_u_<index> = OpLoad %uint %exec_hi ; unused
        %exec_lo_b_<index> = OpINotEqual %bool %exec_lo_u_<index> %uint_0
               OpSelectionMerge %t278_<index> None
               OpBranchConditional %exec_lo_b_<index> %t277_<index> %t278_<index>       
		%t277_<index> = OpLabel

        %t100_<index> = OpLoad %float %<src0>
        %t101_<index> = OpBitcast %int %t100_<index>
               OpStore %temp_int_1 %t101_<index>
        %t148_<index> = OpLoad %uint %<src1_value1>
        %t150_<index> = OpShiftRightLogical %uint %t148_<index> %int_16
        %t152_<index> = OpBitwiseAnd %uint %t150_<index> %uint_0x00003fff
        %t153_<index> = OpBitcast %int %t152_<index>
               OpStore %temp_int_3 %t153_<index>
        %t155_<index> = OpLoad %uint %<src1_value0>
        %t156_<index> = OpBitcast %int %t155_<index>
               OpStore %temp_int_4 %t156_<index>                
               OpStore %temp_int_2 %<offset>
		%t206_<index> = OpLoad %uint %<src1_value3>
        %t208_<index> = OpShiftRightLogical %uint %t206_<index> %int_12
        %t210_<index> = OpBitwiseAnd %uint %t208_<index> %uint_127
        %t211_<index> = OpBitcast %int %t210_<index>
               OpStore %temp_int_5 %t211_<index>
        %t110_<index> = OpFunctionCall %void %tbuffer_store_format_x %<p0> %temp_int_1 %temp_int_2 %temp_int_3 %temp_int_4 %temp_int_5 

               OpBranch %t278_<index>			   
        %t278_<index> = OpLabel
)";
		*dst_source += String(text)
		                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<src0>", src0_value.value)
		                   .ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<src1_value0>", src1_value0.value)
		                   .ReplaceStr(U"<src1_value1>", src1_value1.value)
		                   .ReplaceStr(U"<src1_value3>", src1_value3.value)
		                   .ReplaceStr(U"<p0>", dst_value.value);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_DsAppend_VdstGds)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->gds_pointers.pointers_num > 0)
	{
		String index_str = String::FromPrintf("%u", index);

		EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

		auto dst_value = operand_variable_to_str(inst.dst);

		EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

		// TODO() check VSKIP
		// TODO() check EXEC

		static const char32_t* text = UR"(
        %t192_<index> = OpLoad %uint %m0
        %t194_<index> = OpShiftRightLogical %uint %t192_<index> %int_16
        %t196_<index> = OpAccessChain %_ptr_StorageBuffer_uint %gds %int_0 %t194_<index>
        %t198_<index> = OpAtomicIAdd %uint %t196_<index> %uint_1 %uint_0 %uint_1
        %t199_<index> = OpBitcast %float %t198_<index>
               OpStore %<dst> %t199_<index>
               OpMemoryBarrier %uint_1 %uint_72
)";
		*dst_source += String(text).ReplaceStr(U"<dst>", dst_value.value).ReplaceStr(U"<index>", index_str);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_DsConsume_VdstGds)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->gds_pointers.pointers_num > 0)
	{
		String index_str = String::FromPrintf("%u", index);

		EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

		auto dst_value = operand_variable_to_str(inst.dst);

		EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

		// TODO() check VSKIP
		// TODO() check EXEC

		static const char32_t* text = UR"(
        %t192_<index> = OpLoad %uint %m0
        %t194_<index> = OpShiftRightLogical %uint %t192_<index> %int_16
        %t196_<index> = OpAccessChain %_ptr_StorageBuffer_uint %gds %int_0 %t194_<index>
        %t198_<index> = OpAtomicISub %uint %t196_<index> %uint_1 %uint_0 %uint_1
        %t199_<index> = OpBitcast %float %t198_<index>
               OpStore %<dst> %t199_<index>
               OpMemoryBarrier %uint_1 %uint_72
)";
		*dst_source += String(text).ReplaceStr(U"<dst>", dst_value.value).ReplaceStr(U"<index>", index_str);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_Exp_Mrt0OffOffComprVmDone)
{
	EXIT_NOT_IMPLEMENTED(index == 0 || index + 1 >= code.GetInstructions().Size());

	const auto& prev_inst = code.GetInstructions().At(index - 1);
	const auto& inst      = code.GetInstructions().At(index);
	const auto& next_inst = code.GetInstructions().At(index + 1);

	if (!(prev_inst.type == ShaderInstructionType::SMovB64 && prev_inst.format == ShaderInstructionFormat::Sdst2Ssrc0 &&
	      prev_inst.dst.type == ShaderOperandType::ExecLo && prev_inst.src[0].type == ShaderOperandType::IntegerInlineConstant &&
	      prev_inst.src[0].constant.i == 0 && next_inst.type == ShaderInstructionType::SEndpgm))
	{
		return false;
	}

	const auto* info = spirv->GetPsInputInfo();

	EXIT_NOT_IMPLEMENTED(info == nullptr || !info->ps_pixel_kill_enable);
	EXIT_NOT_IMPLEMENTED(info->target_output_mode[0] != 4);

	EXIT_NOT_IMPLEMENTED(inst.src_num > 0);

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
        OpKill
)";

	*dst_source += String(text);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_Exp_Mrt0Vsrc0Vsrc1ComprVmDone)
{
	const auto& inst = code.GetInstructions().At(index);

	EXIT_NOT_IMPLEMENTED(spirv->GetPsInputInfo()->target_output_mode[0] != 4);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[0]));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[1]));

	auto src0_value = operand_variable_to_str(inst.src[0]);
	auto src1_value = operand_variable_to_str(inst.src[1]);

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
         %t1_<index> = OpLoad %float %<src0> 
         %t2_<index> = OpBitcast %uint %t1_<index>
         %t3_<index> = OpExtInst %v2float %GLSL_std_450 UnpackHalf2x16 %t2_<index>
         %t4_<index> = OpCompositeExtract %float %t3_<index> 0 
         %t5_<index> = OpCompositeExtract %float %t3_<index> 1 
         %t6_<index> = OpLoad %float %<src1> 
         %t7_<index> = OpBitcast %uint %t6_<index> 
         %t8_<index> = OpExtInst %v2float %GLSL_std_450 UnpackHalf2x16 %t7_<index> 
         %t9_<index> = OpCompositeExtract %float %t8_<index> 0 
         %t10_<index> = OpCompositeExtract %float %t8_<index> 1 
         %t11_<index> = OpCompositeConstruct %v4float %t4_<index> %t5_<index> %t9_<index> %t10_<index> 
               OpStore %outColor %t11_<index>
)";

	*dst_source += String(text)
	                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
	                   .ReplaceStr(U"<src0>", src0_value.value)
	                   .ReplaceStr(U"<src1>", src1_value.value);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_Exp_Mrt0Vsrc0Vsrc1Vsrc2Vsrc3VmDone)
{
	const auto& inst = code.GetInstructions().At(index);

	EXIT_NOT_IMPLEMENTED(spirv->GetPsInputInfo()->target_output_mode[0] != 9);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[0]));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[1]));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[2]));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[3]));

	auto src0_value = operand_variable_to_str(inst.src[0]);
	auto src1_value = operand_variable_to_str(inst.src[1]);
	auto src2_value = operand_variable_to_str(inst.src[2]);
	auto src3_value = operand_variable_to_str(inst.src[3]);

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
         %t0_<index> = OpLoad %float %<src0>
         %t1_<index> = OpLoad %float %<src1>
         %t2_<index> = OpLoad %float %<src2>
         %t3_<index> = OpLoad %float %<src3>
         %t11_<index> = OpCompositeConstruct %v4float %t0_<index> %t1_<index> %t2_<index> %t3_<index> 
               OpStore %outColor %t11_<index>
)";

	*dst_source += String(text)
	                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
	                   .ReplaceStr(U"<src0>", src0_value.value)
	                   .ReplaceStr(U"<src1>", src1_value.value)
	                   .ReplaceStr(U"<src2>", src2_value.value)
	                   .ReplaceStr(U"<src3>", src3_value.value);

	return true;
}

/* XXX: 0, 1, 2, 3*/
KYTY_RECOMPILER_FUNC(Recompile_Exp_Param_XXX_Vsrc0Vsrc1Vsrc2Vsrc3)
{
	const auto& inst = code.GetInstructions().At(index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[0]));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[1]));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[2]));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[3]));

	auto src0_value = operand_variable_to_str(inst.src[0]);
	auto src1_value = operand_variable_to_str(inst.src[1]);
	auto src2_value = operand_variable_to_str(inst.src[2]);
	auto src3_value = operand_variable_to_str(inst.src[3]);

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
         %t0_<index> = OpLoad %float %<src0>
         %t1_<index> = OpLoad %float %<src1>
         %t2_<index> = OpLoad %float %<src2>
         %t3_<index> = OpLoad %float %<src3>
         %t4_<index> = OpCompositeConstruct %v4float %t0_<index> %t1_<index> %t2_<index> %t3_<index>
               OpStore %<param> %t4_<index>
)";

	*dst_source += String(text)
	                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
	                   .ReplaceStr(U"<src0>", src0_value.value)
	                   .ReplaceStr(U"<src1>", src1_value.value)
	                   .ReplaceStr(U"<src2>", src2_value.value)
	                   .ReplaceStr(U"<src3>", src3_value.value)
	                   .ReplaceStr(U"<param>", param[0]);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_Exp_Pos0Vsrc0Vsrc1Vsrc2Vsrc3Done)
{
	const auto& inst = code.GetInstructions().At(index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[0]));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[1]));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[2]));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[3]));

	auto src0_value = operand_variable_to_str(inst.src[0]);
	auto src1_value = operand_variable_to_str(inst.src[1]);
	auto src2_value = operand_variable_to_str(inst.src[2]);
	auto src3_value = operand_variable_to_str(inst.src[3]);

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
         %t0_<index> = OpLoad %float %<src0>
         %t1_<index> = OpLoad %float %<src1>
         %t2_<index> = OpLoad %float %<src2>
         %t3_<index> = OpLoad %float %<src3>
         %t4_<index> = OpCompositeConstruct %v4float %t0_<index> %t1_<index> %t2_<index> %t3_<index>
         %t5_<index> = OpAccessChain %_ptr_Output_v4float %outPerVertex %int_per_vertex_0
               OpStore %t5_<index> %t4_<index>
)";

	*dst_source += String(text)
	                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
	                   .ReplaceStr(U"<src0>", src0_value.value)
	                   .ReplaceStr(U"<src1>", src1_value.value)
	                   .ReplaceStr(U"<src2>", src2_value.value)
	                   .ReplaceStr(U"<src3>", src3_value.value);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_ImageSample_Vdata3Vaddr3StSsDmask7)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->textures2D.textures_num > 0 && bind_info->samplers.samplers_num > 0)
	{
		auto dst_value0  = operand_variable_to_str(inst.dst, 0);
		auto dst_value1  = operand_variable_to_str(inst.dst, 1);
		auto dst_value2  = operand_variable_to_str(inst.dst, 2);
		auto src0_value0 = operand_variable_to_str(inst.src[0], 0);
		auto src0_value1 = operand_variable_to_str(inst.src[0], 1);
		auto src0_value2 = operand_variable_to_str(inst.src[0], 2);
		auto src1_value0 = operand_variable_to_str(inst.src[1], 0);
		auto src2_value0 = operand_variable_to_str(inst.src[2], 0);

		EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src0_value0.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src1_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src2_value0.type != SpirvType::Uint);

		// TODO() check VSKIP
		// TODO() check LOD_CLAMPED

		static const char32_t* text = UR"(
         %t24_<index> = OpLoad %uint %<src1_value0>
         %t26_<index> = OpAccessChain %_ptr_UniformConstant_SampledImage %textures2D %t24_<index>
         %t27_<index> = OpLoad %SampledImage %t26_<index>
         %t33_<index> = OpLoad %uint %<src2_value0>
         %t35_<index> = OpAccessChain %_ptr_UniformConstant_Sampler %samplers %t33_<index>
         %t36_<index> = OpLoad %Sampler %t35_<index>
         %t38_<index> = OpSampledImage %_SampledImage %t27_<index> %t36_<index>
         %t39_<index> = OpLoad %float %<src0_value0>
         %t40_<index> = OpLoad %float %<src0_value1>
         %t42_<index> = OpCompositeConstruct %v2float %t39_<index> %t40_<index>
         %t43_<index> = OpImageSampleImplicitLod %v4float %t38_<index> %t42_<index>
               OpStore %temp_v4float %t43_<index>
         %t46_<index> = OpAccessChain %_ptr_Function_float %temp_v4float %uint_0
         %t47_<index> = OpLoad %float %t46_<index>
               OpStore %<dst_value0> %t47_<index>
         %t50_<index> = OpAccessChain %_ptr_Function_float %temp_v4float %uint_1
         %t51_<index> = OpLoad %float %t50_<index>
               OpStore %<dst_value1> %t51_<index>
         %t54_<index> = OpAccessChain %_ptr_Function_float %temp_v4float %uint_2
         %t55_<index> = OpLoad %float %t54_<index>
               OpStore %<dst_value2> %t55_<index>
)";
		*dst_source += String(text)
		                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<src0_value0>", src0_value0.value)
		                   .ReplaceStr(U"<src0_value1>", src0_value1.value)
		                   .ReplaceStr(U"<src0_value2>", src0_value2.value)
		                   .ReplaceStr(U"<src1_value0>", src1_value0.value)
		                   .ReplaceStr(U"<src2_value0>", src2_value0.value)
		                   .ReplaceStr(U"<dst_value0>", dst_value0.value)
		                   .ReplaceStr(U"<dst_value1>", dst_value1.value)
		                   .ReplaceStr(U"<dst_value2>", dst_value2.value);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_ImageSample_Vdata4Vaddr3StSsDmaskF)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->textures2D.textures_num > 0 && bind_info->samplers.samplers_num > 0)
	{
		auto dst_value0  = operand_variable_to_str(inst.dst, 0);
		auto dst_value1  = operand_variable_to_str(inst.dst, 1);
		auto dst_value2  = operand_variable_to_str(inst.dst, 2);
		auto dst_value3  = operand_variable_to_str(inst.dst, 3);
		auto src0_value0 = operand_variable_to_str(inst.src[0], 0);
		auto src0_value1 = operand_variable_to_str(inst.src[0], 1);
		auto src0_value2 = operand_variable_to_str(inst.src[0], 2);
		auto src1_value0 = operand_variable_to_str(inst.src[1], 0);
		auto src2_value0 = operand_variable_to_str(inst.src[2], 0);

		EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src0_value0.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src1_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src2_value0.type != SpirvType::Uint);

		// TODO() check VSKIP
		// TODO() check LOD_CLAMPED

		static const char32_t* text = UR"(
         %t24_<index> = OpLoad %uint %<src1_value0>
         %t26_<index> = OpAccessChain %_ptr_UniformConstant_SampledImage %textures2D %t24_<index>
         %t27_<index> = OpLoad %SampledImage %t26_<index>
         %t33_<index> = OpLoad %uint %<src2_value0>
         %t35_<index> = OpAccessChain %_ptr_UniformConstant_Sampler %samplers %t33_<index>
         %t36_<index> = OpLoad %Sampler %t35_<index>
         %t38_<index> = OpSampledImage %_SampledImage %t27_<index> %t36_<index>
         %t39_<index> = OpLoad %float %<src0_value0>
         %t40_<index> = OpLoad %float %<src0_value1>
         %t42_<index> = OpCompositeConstruct %v2float %t39_<index> %t40_<index>
         %t43_<index> = OpImageSampleImplicitLod %v4float %t38_<index> %t42_<index>
               OpStore %temp_v4float %t43_<index>
         %t46_<index> = OpAccessChain %_ptr_Function_float %temp_v4float %uint_0
         %t47_<index> = OpLoad %float %t46_<index>
               OpStore %<dst_value0> %t47_<index>
         %t50_<index> = OpAccessChain %_ptr_Function_float %temp_v4float %uint_1
         %t51_<index> = OpLoad %float %t50_<index>
               OpStore %<dst_value1> %t51_<index>
         %t54_<index> = OpAccessChain %_ptr_Function_float %temp_v4float %uint_2
         %t55_<index> = OpLoad %float %t54_<index>
               OpStore %<dst_value2> %t55_<index>
         %t57_<index> = OpAccessChain %_ptr_Function_float %temp_v4float %uint_3
         %t58_<index> = OpLoad %float %t57_<index>
               OpStore %<dst_value3> %t58_<index>
)";
		*dst_source += String(text)
		                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<src0_value0>", src0_value0.value)
		                   .ReplaceStr(U"<src0_value1>", src0_value1.value)
		                   .ReplaceStr(U"<src0_value2>", src0_value2.value)
		                   .ReplaceStr(U"<src1_value0>", src1_value0.value)
		                   .ReplaceStr(U"<src2_value0>", src2_value0.value)
		                   .ReplaceStr(U"<dst_value0>", dst_value0.value)
		                   .ReplaceStr(U"<dst_value1>", dst_value1.value)
		                   .ReplaceStr(U"<dst_value2>", dst_value2.value)
		                   .ReplaceStr(U"<dst_value3>", dst_value3.value);

		return true;
	}

	return false;
}

/* XXX: Andn2, Or, Nor */
KYTY_RECOMPILER_FUNC(Recompile_S_XXX_B64_Sdst2Ssrc02Ssrc12)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value0 = operand_variable_to_str(inst.dst, 0);
	auto dst_value1 = operand_variable_to_str(inst.dst, 1);

	EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);

	String load0;
	String load1;
	String load2;
	String load3;

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0, 0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[0], U"t1_<index>", index_str, &load1, 1))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[1], U"t2_<index>", index_str, &load2, 0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[1], U"t3_<index>", index_str, &load3, 1))
	{
		return false;
	}

	static const char32_t* text = UR"(    
    <load0>
    <load1>
    <load2>
    <load3>
    <param0>
    <param1>
    <param2>
    <param3>
    OpStore %<dst0> %tb_<index>
    OpStore %<dst1> %td_<index>
    <execz>
    <scc>
)";

	*dst_source += String(text)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<load2>", load2)
	                   .ReplaceStr(U"<load3>", load3)
	                   .ReplaceStr(U"<param0>", param[0])
	                   .ReplaceStr(U"<param1>", param[1])
	                   .ReplaceStr(U"<param2>", (param[2] == nullptr ? U"" : param[2]))
	                   .ReplaceStr(U"<param3>", (param[3] == nullptr ? U"" : param[3]))
	                   .ReplaceStr(U"<execz>", (operand_is_exec(inst.dst) ? EXECZ : U""))
	                   .ReplaceStr(U"<scc>", get_scc_check(scc_check, 2))
	                   .ReplaceStr(U"<dst0>", dst_value0.value)
	                   .ReplaceStr(U"<dst1>", dst_value1.value)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: And, Lshl, Lshr, CSelect */
KYTY_RECOMPILER_FUNC(Recompile_S_XXX_B32_SVdstSVsrc0SVsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Uint);
	EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	static const char32_t* text = UR"(
              <load0>
              <load1>
              <param0>
              <param1>
              <param2>
              OpStore %<dst> %t_<index>
              <scc>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param0>", param[0])
	                   .ReplaceStr(U"<param1>", (param[1] == nullptr ? U"" : param[1]))
	                   .ReplaceStr(U"<param2>", (param[2] == nullptr ? U"" : param[2]))
	                   .ReplaceStr(U"<scc>", get_scc_check(scc_check, 1))
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Add, Mul */
KYTY_RECOMPILER_FUNC(Recompile_S_XXX_I32_SVdstSVsrc0SVsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Uint);
	EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

	if (!operand_load_int(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_int(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	static const char32_t* text = UR"(
              <load0>
              <load1>
              <param>
              %tu_<index> = OpBitcast %uint %t_<index>
              OpStore %<dst> %tu_<index>
              <scc>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param>", param[0])
	                   .ReplaceStr(U"<scc>", get_scc_check(scc_check, 1))
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_SAndSaveexecB64_Sdst2Ssrc02)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value0 = operand_variable_to_str(inst.dst, 0);
	auto dst_value1 = operand_variable_to_str(inst.dst, 1);

	EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);

	EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

	String load0;
	String load1;

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0, 0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[0], U"t1_<index>", index_str, &load1, 1))
	{
		return false;
	}

	static const char32_t* text = UR"(
        <load0>
        <load1>
        %t190_<index> = OpLoad %uint %exec_lo
               OpStore %<dst0> %t190_<index>
        %t191_<index> = OpLoad %uint %exec_hi
               OpStore %<dst1> %t191_<index>
        %t194_<index> = OpBitwiseAnd %uint %t0_<index> %t190_<index>
               OpStore %exec_lo %t194_<index>
        %t197_<index> = OpBitwiseAnd %uint %t1_<index> %t191_<index>
               OpStore %exec_hi %t197_<index>    
        <execz>
        <scc>
)";

	*dst_source += String(text)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<execz>", EXECZ)
	                   .ReplaceStr(U"<scc>", get_scc_check(scc_check, 2))
	                   .ReplaceStr(U"<dst0>", dst_value0.value)
	                   .ReplaceStr(U"<dst1>", dst_value1.value)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Eq */
KYTY_RECOMPILER_FUNC(Recompile_SCmp_XXX_U32_Ssrc0Ssrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	static const char32_t* text = UR"(
          <load0>
          <load1>
          %t2_<index> = <param> %bool %t0_<index> %t1_<index>
          %t3_<index> = OpSelect %uint %t2_<index> %uint_1 %uint_0
          OpStore %scc %t3_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param>", param[0])
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_SBufferLoadDword_SdstSvSoffset)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[1]));

		auto   dst_value   = operand_variable_to_str(inst.dst);
		auto   src0_value0 = operand_variable_to_str(inst.src[0], 0);
		String offset      = spirv->GetConstant(inst.src[1]);

		EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src0_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

		static const char32_t* text = UR"(
        %t100_<index> = OpLoad %uint %<src0_value0>
        %t101_<index> = OpBitcast %int %t100_<index>
               OpStore %temp_int_2 %t101_<index>
        %t102_<index> = OpBitcast %int %<offset>
               OpStore %temp_int_1 %t102_<index>
        %t110_<index> = OpFunctionCall %void %sbuffer_load_dword %<p0> %temp_int_1 %temp_int_2 
)";
		*dst_source += String(text)
		                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<src0_value0>", src0_value0.value)
		                   .ReplaceStr(U"<p0>", dst_value.value);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_SBufferLoadDwordx2_Sdst2SvSoffset)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[1]));

		auto   dst_value0  = operand_variable_to_str(inst.dst, 0);
		auto   dst_value1  = operand_variable_to_str(inst.dst, 1);
		auto   src0_value0 = operand_variable_to_str(inst.src[0], 0);
		String offset      = spirv->GetConstant(inst.src[1]);

		EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src0_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

		EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

		static const char32_t* text = UR"(
        %t100_<index> = OpLoad %uint %<src0_value0>
        %t101_<index> = OpBitcast %int %t100_<index>
               OpStore %temp_int_2 %t101_<index>
        %t102_<index> = OpBitcast %int %<offset>
               OpStore %temp_int_1 %t102_<index>
        %t110_<index> = OpFunctionCall %void %sbuffer_load_dword_2 %<p0> %<p1> %temp_int_1 %temp_int_2 
)";
		*dst_source += String(text)
		                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<src0_value0>", src0_value0.value)
		                   .ReplaceStr(U"<p0>", dst_value0.value)
		                   .ReplaceStr(U"<p1>", dst_value1.value);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_SBufferLoadDwordx4_Sdst4SvSoffset)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		// EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[1]));

		auto dst_value0  = operand_variable_to_str(inst.dst, 0);
		auto dst_value1  = operand_variable_to_str(inst.dst, 1);
		auto dst_value2  = operand_variable_to_str(inst.dst, 2);
		auto dst_value3  = operand_variable_to_str(inst.dst, 3);
		auto src0_value0 = operand_variable_to_str(inst.src[0], 0);
		// String offset      = spirv->GetConstant(inst.src[1]);

		EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src0_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

		EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

		String index_str = String::FromPrintf("%u", index);

		String load1;

		if (!operand_load_uint(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
		{
			return false;
		}

		static const char32_t* text = UR"(
        <load1>
        %t100_<index> = OpLoad %uint %<src0_value0>
        %t101_<index> = OpBitcast %int %t100_<index>
               OpStore %temp_int_2 %t101_<index>
        %t102_<index> = OpBitcast %int %t1_<index>
               OpStore %temp_int_1 %t102_<index>
        %t110_<index> = OpFunctionCall %void %sbuffer_load_dword_4 %<p0> %<p1> %<p2> %<p3> %temp_int_1 %temp_int_2 
)";
		*dst_source += String(text)
		                   //.ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<load1>", load1)
		                   .ReplaceStr(U"<src0_value0>", src0_value0.value)
		                   .ReplaceStr(U"<p0>", dst_value0.value)
		                   .ReplaceStr(U"<p1>", dst_value1.value)
		                   .ReplaceStr(U"<p2>", dst_value2.value)
		                   .ReplaceStr(U"<p3>", dst_value3.value)
		                   .ReplaceStr(U"<index>", index_str);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_SBufferLoadDwordx8_Sdst8SvSoffset)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[1]));

		SpirvValue dst_value[8];

		for (int i = 0; i < 8; i++)
		{
			dst_value[i] = operand_variable_to_str(inst.dst, i);
		}

		auto   src0_value0 = operand_variable_to_str(inst.src[0], 0);
		String offset      = spirv->GetConstant(inst.src[1]);

		EXIT_NOT_IMPLEMENTED(dst_value[0].type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src0_value0.type != SpirvType::Uint);

		EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

		String text = UR"(
        %t100_<index> = OpLoad %uint %<src0_value0>
        %t101_<index> = OpBitcast %int %t100_<index>
               OpStore %temp_int_2 %t101_<index>
        %t102_<index> = OpBitcast %int %<offset>
               OpStore %temp_int_1 %t102_<index>
        %t110_<index> = OpFunctionCall %void %sbuffer_load_dword_8 %<p0> %<p1> %<p2> %<p3> %<p4> %<p5> %<p6> %<p7> %temp_int_1 %temp_int_2 
)";

		for (int i = 0; i < 8; i++)
		{
			text = text.ReplaceStr(String::FromPrintf("<p%d>", i), dst_value[i].value);
		}

		*dst_source += text.ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<src0_value0>", src0_value0.value);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_SBufferLoadDwordx16_Sdst16SvSoffset)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[1]));

		SpirvValue dst_value[16];

		for (int i = 0; i < 16; i++)
		{
			dst_value[i] = operand_variable_to_str(inst.dst, i);
		}

		auto   src0_value0 = operand_variable_to_str(inst.src[0], 0);
		String offset      = spirv->GetConstant(inst.src[1]);

		EXIT_NOT_IMPLEMENTED(dst_value[0].type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src0_value0.type != SpirvType::Uint);

		EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

		String text = UR"(
        %t100_<index> = OpLoad %uint %<src0_value0>
        %t101_<index> = OpBitcast %int %t100_<index>
               OpStore %temp_int_2 %t101_<index>
        %t102_<index> = OpBitcast %int %<offset>
               OpStore %temp_int_1 %t102_<index>
        %t110_<index> = OpFunctionCall %void %sbuffer_load_dword_16 %<p0> %<p1> %<p2> %<p3> %<p4> %<p5> %<p6> %<p7> %<p8> %<p9> %<p10> %<p11> %<p12> %<p13> %<p14> %<p15> %temp_int_1 %temp_int_2 
)";

		for (int i = 0; i < 16; i++)
		{
			text = text.ReplaceStr(String::FromPrintf("<p%d>", i), dst_value[i].value);
		}

		*dst_source += text.ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<src0_value0>", src0_value0.value);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_SCbranchExecz_Label)
{
	const auto& inst = code.GetInstructions().At(index);

	EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[0]));

	String label = String::FromPrintf("label_%04" PRIx32 "_%04" PRIx32, inst.pc + 4 + inst.src[0].constant.i, inst.pc);

	static const char32_t* text = UR"(
        %execz_u_<index> = OpLoad %uint %execz
        %execz_b_<index> = OpINotEqual %bool %execz_u_<index> %uint_0
               OpSelectionMerge %<label> None
               OpBranchConditional %execz_b_<index> %<label> %t230_<index>
        %t230_<index> = OpLabel
)";

	*dst_source += String(text).ReplaceStr(U"<index>", String::FromPrintf("%u", index)).ReplaceStr(U"<label>", label);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_SCbranchScc0_Label)
{
	const auto& inst = code.GetInstructions().At(index);

	EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[0]));

	String label = String::FromPrintf("label_%04" PRIx32 "_%04" PRIx32, inst.pc + 4 + inst.src[0].constant.i, inst.pc);

	static const char32_t* text = UR"(
        %scc_u_<index> = OpLoad %uint %scc
        %scc_b_<index> = OpIEqual %bool %scc_u_<index> %uint_0
               OpSelectionMerge %<label> None
               OpBranchConditional %scc_b_<index> %<label> %t230_<index>
        %t230_<index> = OpLabel
)";

	*dst_source += String(text).ReplaceStr(U"<index>", String::FromPrintf("%u", index)).ReplaceStr(U"<label>", label);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_SEndpgm_Empty)
{
	// const auto* info = spirv->GetPsInputInfo();

	// EXIT_NOT_IMPLEMENTED(info == nullptr || !info->ps_pixel_kill_enable);

	static const char32_t* text = UR"(
       OpReturn
)";

	EXIT_NOT_IMPLEMENTED(index < 2);

	const auto& prev_prev_inst = code.GetInstructions().At(index - 2);
	const auto& prev_inst      = code.GetInstructions().At(index - 1);

	bool after_kill =
	    (prev_prev_inst.type == ShaderInstructionType::SMovB64 && prev_prev_inst.format == ShaderInstructionFormat::Sdst2Ssrc0 &&
	     prev_prev_inst.dst.type == ShaderOperandType::ExecLo && prev_prev_inst.src[0].type == ShaderOperandType::IntegerInlineConstant &&
	     prev_prev_inst.src[0].constant.i == 0 && prev_inst.type == ShaderInstructionType::Exp &&
	     prev_inst.format == ShaderInstructionFormat::Mrt0OffOffComprVmDone);

	if (!after_kill)
	{
		*dst_source += String(text);
	}

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_SLoadDwordx4_Sdst4SbaseSoffset)
{
	const auto& inst = code.GetInstructions().At(index);

	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->extended.used)
	{
		EXIT_NOT_IMPLEMENTED(inst.src[1].type != ShaderOperandType::LiteralConstant);
		EXIT_NOT_IMPLEMENTED(inst.src[0].register_id != bind_info->extended.start_register);

		// TODO() check pointer

		SpirvValue dst_value[4];

		for (int i = 0; i < 4; i++)
		{
			dst_value[i] = operand_variable_to_str(inst.dst, i);
		}
		auto src0_value0 = operand_variable_to_str(inst.src[0], 0);
		auto src0_value1 = operand_variable_to_str(inst.src[0], 1);
		int  offset      = static_cast<int>(inst.src[1].constant.u >> 2u);

		EXIT_NOT_IMPLEMENTED(dst_value[0].type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src0_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src0_value1.type != SpirvType::Uint);

		static const char32_t* text = UR"(
		         %vsharp_<index>_<reg> = OpAccessChain %_ptr_PushConstant_uint %vsharp %int_0 %int_<buffer> %int_<field>
		         %vsharp_<index>_value_<reg> = OpLoad %uint %vsharp_<index>_<reg>
		               OpStore %<reg> %vsharp_<index>_value_<reg>
				)";

		for (int i = 0; i < 4; i++)
		{
			int buffer = 0;
			int field  = 0;
			spirv->GetMappedIndex(offset + i, &buffer, &field);

			*dst_source += String(text)
			                   .ReplaceStr(U"<reg>", dst_value[i].value)
			                   .ReplaceStr(U"<buffer>", String::FromPrintf("%d", buffer))
			                   .ReplaceStr(U"<field>", String::FromPrintf("%d", field))
			                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index));
		}

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_SLoadDwordx8_Sdst8SbaseSoffset)
{
	const auto& inst = code.GetInstructions().At(index);

	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->extended.used)
	{
		EXIT_NOT_IMPLEMENTED(inst.src[1].type != ShaderOperandType::LiteralConstant);
		EXIT_NOT_IMPLEMENTED(inst.src[0].register_id != bind_info->extended.start_register);

		// TODO() check pointer

		SpirvValue dst_value[8];

		for (int i = 0; i < 8; i++)
		{
			dst_value[i] = operand_variable_to_str(inst.dst, i);
		}
		auto src0_value0 = operand_variable_to_str(inst.src[0], 0);
		auto src0_value1 = operand_variable_to_str(inst.src[0], 1);
		int  offset      = static_cast<int>(inst.src[1].constant.u >> 2u);

		EXIT_NOT_IMPLEMENTED(dst_value[0].type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src0_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src0_value1.type != SpirvType::Uint);

		static const char32_t* text = UR"(
		         %vsharp_<index>_<reg> = OpAccessChain %_ptr_PushConstant_uint %vsharp %int_0 %int_<buffer> %int_<field>
		         %vsharp_<index>_value_<reg> = OpLoad %uint %vsharp_<index>_<reg>
		               OpStore %<reg> %vsharp_<index>_value_<reg>
				)";

		for (int i = 0; i < 8; i++)
		{
			int buffer = 0;
			int field  = 0;
			spirv->GetMappedIndex(offset + i, &buffer, &field);

			*dst_source += String(text)
			                   .ReplaceStr(U"<reg>", dst_value[i].value)
			                   .ReplaceStr(U"<buffer>", String::FromPrintf("%d", buffer))
			                   .ReplaceStr(U"<field>", String::FromPrintf("%d", field))
			                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index));
		}

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_SMovB32_SVdstSVsrc0)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Uint);
	EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

	String load0;

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}

	static const char32_t* text = UR"(    
    <load0>
    OpStore %<dst> %t0_<index>
)";
	*dst_source += String(text).ReplaceStr(U"<dst>", dst_value.value).ReplaceStr(U"<load0>", load0).ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_SMovB64_Sdst2Ssrc02)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value0 = operand_variable_to_str(inst.dst, 0);
	auto dst_value1 = operand_variable_to_str(inst.dst, 1);

	EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);

	// EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

	String load0;
	String load1;

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0, 0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[0], U"t1_<index>", index_str, &load1, 1))
	{
		return false;
	}

	static const char32_t* text = UR"(    
    <load0>
    <load1>
    OpStore %<dst0> %t0_<index>
    OpStore %<dst1> %t1_<index>
    <execz>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst0>", dst_value0.value)
	                   .ReplaceStr(U"<dst1>", dst_value1.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<execz>", (operand_is_exec(inst.dst) ? EXECZ : U""))
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_SMovB64_Sdst2Ssrc0)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value0 = operand_variable_to_str(inst.dst, 0);
	auto dst_value1 = operand_variable_to_str(inst.dst, 1);

	EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);

	EXIT_NOT_IMPLEMENTED(!(inst.src[0].type == ShaderOperandType::IntegerInlineConstant && inst.src[0].constant.i >= 0));

	String load0;

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0, 0))
	{
		return false;
	}

	static const char32_t* text = UR"(    
    <load0>
    OpStore %<dst0> %t0_<index>
    OpStore %<dst1> %uint_0
    <execz>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst0>", dst_value0.value)
	                   .ReplaceStr(U"<dst1>", dst_value1.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<execz>", (operand_is_exec(inst.dst) ? EXECZ : U""))
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_SSwappcB64_Sdst2Ssrc02)
{
	const auto& inst       = code.GetInstructions().At(index);
	const auto* input_info = spirv->GetVsInputInfo();

	EXIT_IF(!input_info->fetch);

	if (input_info != nullptr && inst.dst.type == ShaderOperandType::Sgpr && inst.dst.register_id == 0 &&
	    inst.src[0].type == ShaderOperandType::Sgpr && inst.src[0].register_id == 0 && index == 1)
	{
		for (int i = 0; i < input_info->resources_num; i++)
		{
			const auto& r = input_info->resources_dst[i];

			String text;

			switch (r.registers_num)
			{
				case 1:
					text = UR"(
				         %t1_<index> = OpLoad %float %<attr> 
				                       OpStore %temp_float %t1_<index> 
				         %t2_<index> = OpFunctionCall %void %fetch_f1_f1_ %<p0> %temp_float
				)";
					break;
				case 2:
					text = UR"(
				         %t1_<index> = OpLoad %v2float %<attr> 
				                       OpStore %temp_v2float %t1_<index> 
				         %t2_<index> = OpFunctionCall %void %fetch_f1_f1_vf2_ %<p0> %<p1> %temp_v2float
				)";
					break;
				case 3:
					text = UR"(
				         %t1_<index> = OpLoad %v3float %<attr> 
				                       OpStore %temp_v3float %t1_<index> 
				         %t2_<index> = OpFunctionCall %void %fetch_f1_f1_f1_vf3_ %<p0> %<p1> %<p2> %temp_v3float
				)";
					break;
				case 4:
					text = UR"(
				         %t1_<index> = OpLoad %v4float %<attr> 
				                       OpStore %temp_v4float %t1_<index> 
				         %t2_<index> = OpFunctionCall %void %fetch_f1_f1_f1_f1_vf4_ %<p0> %<p1> %<p2> %<p3> %temp_v4float
				)";
					break;
				default: EXIT("invalid registers_num: %d\n", r.registers_num);
			}

			*dst_source += String(text)
			                   .ReplaceStr(U"<index>", String::FromPrintf("%d_%u", i, index))
			                   .ReplaceStr(U"<p0>", String::FromPrintf("v%d", r.register_start + 0))
			                   .ReplaceStr(U"<p1>", String::FromPrintf("v%d", r.register_start + 1))
			                   .ReplaceStr(U"<p2>", String::FromPrintf("v%d", r.register_start + 2))
			                   .ReplaceStr(U"<p3>", String::FromPrintf("v%d", r.register_start + 3))
			                   .ReplaceStr(U"<attr>", String::FromPrintf("attr%d", i));
		}
		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_SWqmB64_Sdst2Ssrc02)
{
	const auto& inst = code.GetInstructions().At(index);
	return (inst.dst.type == ShaderOperandType::ExecLo && inst.src[0].type == ShaderOperandType::ExecLo);
}

KYTY_RECOMPILER_FUNC(Recompile_SWaitcnt_Imm)
{
	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_TBufferLoadFormatXyzw_Vdata4VaddrSvSoffsIdxenFloat4)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[2]));

		auto   dst_value0  = operand_variable_to_str(inst.dst, 0);
		auto   dst_value1  = operand_variable_to_str(inst.dst, 1);
		auto   dst_value2  = operand_variable_to_str(inst.dst, 2);
		auto   dst_value3  = operand_variable_to_str(inst.dst, 3);
		auto   src0_value  = operand_variable_to_str(inst.src[0]);
		auto   src1_value0 = operand_variable_to_str(inst.src[1], 0);
		auto   src1_value1 = operand_variable_to_str(inst.src[1], 1);
		String offset      = spirv->GetConstant(inst.src[2]);

		EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src0_value.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src1_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src1_value1.type != SpirvType::Uint);

		// TODO() check VSKIP
		// TODO() check EXEC

		static const char32_t* text = UR"(
        %t100_<index> = OpLoad %float %<src0>
        %t101_<index> = OpBitcast %int %t100_<index>
               OpStore %temp_int_1 %t101_<index>
        %t148_<index> = OpLoad %uint %<src1_value1>
        %t150_<index> = OpShiftRightLogical %uint %t148_<index> %int_16
        %t152_<index> = OpBitwiseAnd %uint %t150_<index> %uint_0x00003fff
        %t153_<index> = OpBitcast %int %t152_<index>
               OpStore %temp_int_3 %t153_<index>
        %t155_<index> = OpLoad %uint %<src1_value0>
        %t156_<index> = OpBitcast %int %t155_<index>
               OpStore %temp_int_4 %t156_<index>                
               OpStore %temp_int_2 %<offset> 
               OpStore %temp_int_5 %int_119
        %t110_<index> = OpFunctionCall %void %tbuffer_load_format_xyzw %<p0> %<p1> %<p2> %<p3> %temp_int_1 %temp_int_2 %temp_int_3 %temp_int_4 %temp_int_5 
)";
		*dst_source += String(text)
		                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<src0>", src0_value.value)
		                   .ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<src1_value0>", src1_value0.value)
		                   .ReplaceStr(U"<src1_value1>", src1_value1.value)
		                   .ReplaceStr(U"<p0>", dst_value0.value)
		                   .ReplaceStr(U"<p1>", dst_value1.value)
		                   .ReplaceStr(U"<p2>", dst_value2.value)
		                   .ReplaceStr(U"<p3>", dst_value3.value);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_TBufferLoadFormatXyzw_Vdata4Vaddr2SvSoffsOffenIdxenFloat4)
{
	const auto& inst      = code.GetInstructions().At(index);
	const auto* bind_info = spirv->GetBindInfo();

	if (bind_info != nullptr && bind_info->storage_buffers.buffers_num > 0)
	{
		EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[2]));

		auto   dst_value0  = operand_variable_to_str(inst.dst, 0);
		auto   dst_value1  = operand_variable_to_str(inst.dst, 1);
		auto   dst_value2  = operand_variable_to_str(inst.dst, 2);
		auto   dst_value3  = operand_variable_to_str(inst.dst, 3);
		auto   src0_value0 = operand_variable_to_str(inst.src[0], 0);
		auto   src0_value1 = operand_variable_to_str(inst.src[0], 1);
		auto   src1_value0 = operand_variable_to_str(inst.src[1], 0);
		auto   src1_value1 = operand_variable_to_str(inst.src[1], 1);
		String offset      = spirv->GetConstant(inst.src[2]);

		EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src0_value0.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src0_value1.type != SpirvType::Float);
		EXIT_NOT_IMPLEMENTED(src1_value0.type != SpirvType::Uint);
		EXIT_NOT_IMPLEMENTED(src1_value1.type != SpirvType::Uint);

		// TODO() check VSKIP
		// TODO() check EXEC

		static const char32_t* text = UR"(
        %t100_<index> = OpLoad %float %<src0_value0>
        %t101_<index> = OpBitcast %int %t100_<index>
       %to100_<index> = OpLoad %float %<src0_value1>
       %to101_<index> = OpBitcast %int %to100_<index>
               OpStore %temp_int_1 %t101_<index>
        %t148_<index> = OpLoad %uint %<src1_value1>
        %t150_<index> = OpShiftRightLogical %uint %t148_<index> %int_16
        %t152_<index> = OpBitwiseAnd %uint %t150_<index> %uint_0x00003fff
        %t153_<index> = OpBitcast %int %t152_<index>
               OpStore %temp_int_3 %t153_<index>
        %t155_<index> = OpLoad %uint %<src1_value0>
        %t156_<index> = OpBitcast %int %t155_<index>
      %offset_<index> = OpIAdd %int %to101_<index> %<offset>
               OpStore %temp_int_4 %t156_<index>                
               OpStore %temp_int_2 %offset_<index> 
               OpStore %temp_int_5 %int_119
        %t110_<index> = OpFunctionCall %void %tbuffer_load_format_xyzw %<p0> %<p1> %<p2> %<p3> %temp_int_1 %temp_int_2 %temp_int_3 %temp_int_4 %temp_int_5 
)";
		*dst_source += String(text)
		                   .ReplaceStr(U"<index>", String::FromPrintf("%u", index))
		                   .ReplaceStr(U"<src0_value0>", src0_value0.value)
		                   .ReplaceStr(U"<src0_value1>", src0_value1.value)
		                   .ReplaceStr(U"<offset>", offset)
		                   .ReplaceStr(U"<src1_value0>", src1_value0.value)
		                   .ReplaceStr(U"<src1_value1>", src1_value1.value)
		                   .ReplaceStr(U"<p0>", dst_value0.value)
		                   .ReplaceStr(U"<p1>", dst_value1.value)
		                   .ReplaceStr(U"<p2>", dst_value2.value)
		                   .ReplaceStr(U"<p3>", dst_value3.value);

		return true;
	}

	return false;
}

/* XXX: Eq, Le, Neq */
KYTY_RECOMPILER_FUNC(Recompile_VCmp_XXX_F32_SmaskVsrc0Vsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value0 = operand_variable_to_str(inst.dst, 0);
	auto dst_value1 = operand_variable_to_str(inst.dst, 1);

	EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);

	EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

	if (!operand_load_float(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_float(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
          <load0>
          <load1>
          %t2_<index> = <param> %bool %t0_<index> %t1_<index>
          %t3_<index> = OpSelect %uint %t2_<index> %uint_1 %uint_0
          OpStore %<dst0> %t3_<index>
          OpStore %<dst1> %uint_0
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst0>", dst_value0.value)
	                   .ReplaceStr(U"<dst1>", dst_value1.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param>", param[0])
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Eq, Ne, Gt */
KYTY_RECOMPILER_FUNC(Recompile_VCmp_XXX_I32_SmaskVsrc0Vsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value0 = operand_variable_to_str(inst.dst, 0);
	auto dst_value1 = operand_variable_to_str(inst.dst, 1);

	EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);

	EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

	if (!operand_load_int(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_int(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
          <load0>
          <load1>
          %t2_<index> = <param> %bool %t0_<index> %t1_<index>
          %t3_<index> = OpSelect %uint %t2_<index> %uint_1 %uint_0
          OpStore %<dst0> %t3_<index>
          OpStore %<dst1> %uint_0
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst0>", dst_value0.value)
	                   .ReplaceStr(U"<dst1>", dst_value1.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param>", param[0])
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Le */
KYTY_RECOMPILER_FUNC(Recompile_VCmp_XXX_U32_SmaskVsrc0Vsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value0 = operand_variable_to_str(inst.dst, 0);
	auto dst_value1 = operand_variable_to_str(inst.dst, 1);

	EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);

	EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
          <load0>
          <load1>
          %t2_<index> = <param> %bool %t0_<index> %t1_<index>
          %t3_<index> = OpSelect %uint %t2_<index> %uint_1 %uint_0
          OpStore %<dst0> %t3_<index>
          OpStore %<dst1> %uint_0
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst0>", dst_value0.value)
	                   .ReplaceStr(U"<dst1>", dst_value1.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param>", param[0])
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Eq, Ne */
KYTY_RECOMPILER_FUNC(Recompile_VCmpx_XXX_I32_SmaskVsrc0Vsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value0 = operand_variable_to_str(inst.dst, 0);
	auto dst_value1 = operand_variable_to_str(inst.dst, 1);

	EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);

	EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

	if (!operand_load_int(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_int(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
          <load0>
          <load1>
          %t2_<index> = <param> %bool %t0_<index> %t1_<index>
          %t3_<index> = OpSelect %uint %t2_<index> %uint_1 %uint_0
          OpStore %<dst0> %t3_<index>
          OpStore %<dst1> %uint_0
          OpStore %exec_lo %t3_<index>
          OpStore %exec_hi %uint_0
          <execz>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst0>", dst_value0.value)
	                   .ReplaceStr(U"<dst1>", dst_value1.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param>", param[0])
	                   .ReplaceStr(U"<execz>", EXECZ)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Gt */
KYTY_RECOMPILER_FUNC(Recompile_VCmpx_XXX_U32_SmaskVsrc0Vsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value0 = operand_variable_to_str(inst.dst, 0);
	auto dst_value1 = operand_variable_to_str(inst.dst, 1);

	EXIT_NOT_IMPLEMENTED(dst_value0.type != SpirvType::Uint);

	EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst));

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
          <load0>
          <load1>
          %t2_<index> = <param> %bool %t0_<index> %t1_<index>
          %t3_<index> = OpSelect %uint %t2_<index> %uint_1 %uint_0
          OpStore %<dst0> %t3_<index>
          OpStore %<dst1> %uint_0
          OpStore %exec_lo %t3_<index>
          OpStore %exec_hi %uint_0
          <execz>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst0>", dst_value0.value)
	                   .ReplaceStr(U"<dst1>", dst_value1.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param>", param[0])
	                   .ReplaceStr(U"<execz>", EXECZ)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_VCndmaskB32_VdstVsrc0Vsrc1Smask2)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
	EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[2]));

	auto src_bool_value0 = operand_variable_to_str(inst.src[2], 0);
	auto src_bool_value1 = operand_variable_to_str(inst.src[2], 1);

	EXIT_NOT_IMPLEMENTED(src_bool_value0.type != SpirvType::Uint);

	if (!operand_load_float(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_float(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(    
    <load0>
    <load1>
    %t22_<index> = OpLoad %uint %<src0>
    %t23_<index> = OpLoad %uint %<src1> ; unused
    %t2_<index> = OpINotEqual %bool %t22_<index> %uint_0
    %t3_<index> = OpSelect %float %t2_<index> %t1_<index> %t0_<index>
          OpStore %<dst> %t3_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<src0>", src_bool_value0.value)
	                   .ReplaceStr(U"<src1>", src_bool_value1.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_VCvtPkrtzF16F32_SVdstSVsrc0SVsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
	EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

	auto dst_value = operand_variable_to_str(inst.dst);

	if (!operand_load_float(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_float(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC
	// TODO() check DX10_CLAMP

	static const char32_t* text = UR"(    
    <load0>
    <load1>
    %t2_<index> = OpCompositeConstruct %v2float %t0_<index> %t1_<index> 
    %t3_<index> = OpExtInst %uint %GLSL_std_450 PackHalf2x16 %t2_<index>
    %t4_<index> = OpBitcast %float %t3_<index>
          OpStore %<dst> %t4_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_VInterpP1F32_VdstVsrcAttrChan)
{
	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_VInterpP2F32_VdstVsrcAttrChan)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.src[0]));
	EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[1]));
	EXIT_NOT_IMPLEMENTED(!operand_is_constant(inst.src[2]));

	auto dst_value = operand_variable_to_str(inst.dst);

	String load0 = String::FromPrintf("%%t0_<index> = OpAccessChain %%_ptr_Input_float %%attr%u %%uint_%u", inst.src[1].constant.u,
	                                  inst.src[2].constant.u);

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(    
         <load0>
         %t1_<index> = OpLoad %float %t0_<index>
                       OpStore %<dst> %t1_<index>
)";
	*dst_source += String(text).ReplaceStr(U"<dst>", dst_value.value).ReplaceStr(U"<load0>", load0).ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_VMadakF32_VdstVsrc0Vsrc1Vsrc2)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;
	String load2;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
	EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	if (!operand_load_float(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_float(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}
	if (!operand_load_float(spirv, inst.src[2], U"t2_<index>", index_str, &load2))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC
	// TODO() check SP_ROUND
	// TODO() check DX10_CLAMP
	// TODO() check IEEE

	static const char32_t* text = UR"(
              <load0>
              <load1>
              <load2>
        %t245_<index> = OpExtInst %float %GLSL_std_450 Fma %t0_<index> %t1_<index> %t2_<index>
               OpStore %<dst> %t245_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<load2>", load2)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_VMadF32_VdstVsrc0Vsrc1Vsrc2)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;
	String load2;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
	EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	if (!operand_load_float(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_float(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}
	if (!operand_load_float(spirv, inst.src[2], U"t2_<index>", index_str, &load2))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC
	// TODO() check SP_ROUND
	// TODO() check DX10_CLAMP
	// TODO() check IEEE

	static const char32_t* text = UR"(
              <load0>
              <load1>
              <load2>
        %t245_<index> = OpExtInst %float %GLSL_std_450 Fma %t0_<index> %t1_<index> %t2_<index>
               OpStore %<dst> %t245_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<load2>", load2)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_VMbcntHiU32B32_SVdstSVsrc0SVsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	if (inst.src[0].type == ShaderOperandType::ExecHi)
	{
		String index_str = String::FromPrintf("%u", index);

		EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
		EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
		EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

		auto dst_value = operand_variable_to_str(inst.dst);

		EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

		String load0;

		if (!operand_load_float(spirv, inst.src[1], U"t0_<index>", index_str, &load0))
		{
			return false;
		}

		// TODO() check VSKIP
		// TODO() check EXEC

		static const char32_t* text = UR"(    
	    <load0>
	    OpStore %<dst> %t0_<index>
	)";
		*dst_source += String(text).ReplaceStr(U"<dst>", dst_value.value).ReplaceStr(U"<load0>", load0).ReplaceStr(U"<index>", index_str);

		return true;
	}

	return false;
}

KYTY_RECOMPILER_FUNC(Recompile_VMbcntLoU32B32_SVdstSVsrc0SVsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	if (inst.src[0].type == ShaderOperandType::ExecLo)
	{
		String index_str = String::FromPrintf("%u", index);

		EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

		auto dst_value = operand_variable_to_str(inst.dst);

		EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

		String load0;

		if (!operand_load_float(spirv, inst.src[1], U"t0_<index>", index_str, &load0))
		{
			return false;
		}

		// TODO() check VSKIP
		// TODO() check EXEC

		static const char32_t* text = UR"(    
	    <load0>
	    OpStore %<dst> %t0_<index>
	)";
		*dst_source += String(text).ReplaceStr(U"<dst>", dst_value.value).ReplaceStr(U"<load0>", load0).ReplaceStr(U"<index>", index_str);

		return true;
	}

	return false;
}

/* XXX: Not */
KYTY_RECOMPILER_FUNC(Recompile_V_XXX_B32_SVdstSVsrc0)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	String load0;

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(    
              <load0>
              <param0>
              %tf_<index> = OpBitcast %float %t_<index>
              OpStore %<dst> %tf_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<param0>", param[0])
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

KYTY_RECOMPILER_FUNC(Recompile_VMovB32_SVdstSVsrc0)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	String load0;

	if (!operand_load_float(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(    
    <load0>
    OpStore %<dst> %t0_<index>
)";
	*dst_source += String(text).ReplaceStr(U"<dst>", dst_value.value).ReplaceStr(U"<load0>", load0).ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Mac, Max, Min, Mul, Sub, Subrev */
KYTY_RECOMPILER_FUNC(Recompile_V_XXX_F32_SVdstSVsrc0SVsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;
	String load_dst;
	String param0 = param[0];

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	if (!operand_load_float(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_float(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}
	if (param0.ContainsStr(U"tdst_<index>") && !operand_load_float(spirv, inst.dst, U"tdst_<index>", index_str, &load_dst))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC
	// TODO() check SP_DENORM
	// TODO() check SP_ROUND
	// TODO() check DX10_CLAMP
	// TODO() check IEEE

	static const char32_t* text = UR"(
              <load0>
              <load1>
              <load_dst>
              <param>
              OpStore %<dst> %t_<index>
              <multiply>
              <clamp>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<multiply>", (inst.dst.multiplier != 1.0f
	                                                   ? String(MULTIPLY).ReplaceStr(U"<mul>", spirv->GetConstantFloat(inst.dst.multiplier))
	                                                   : U""))
	                   .ReplaceStr(U"<clamp>", (inst.dst.clamp ? CLAMP : U""))
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<load_dst>", load_dst)
	                   .ReplaceStr(U"<param>", param0)
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Rcp, Rsq, Sqrt */
KYTY_RECOMPILER_FUNC(Recompile_V_XXX_F32_SVdstSVsrc0)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
	EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	String load0;

	if (!operand_load_float(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC
	// TODO() check DX10_CLAMP
	// TODO() check IEEE

	static const char32_t* text = UR"(    
    <load0>
    <param>
    OpStore %<dst> %t_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<param>", param[0])
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: And, Lshr, Lshlrev, Lshrrev, MulU32U24 */
KYTY_RECOMPILER_FUNC(Recompile_V_XXX_B32_SVdstSVsrc0SVsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
	EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
              <load0>
              <load1>
              <param0>
              <param1>
              <param2>
              %tf_<index> = OpBitcast %float %t_<index>
              OpStore %<dst> %tf_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param0>", param[0])
	                   .ReplaceStr(U"<param1>", (param[1] == nullptr ? U"" : param[1]))
	                   .ReplaceStr(U"<param2>", (param[2] == nullptr ? U"" : param[2]))
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Ashrrev, MulLo */
KYTY_RECOMPILER_FUNC(Recompile_V_XXX_I32_SVdstSVsrc0SVsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
	EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	if (!operand_load_int(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_int(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
              <load0>
              <load1>
              <param0>
              <param1>
              %tf_<index> = OpBitcast %float %t_<index>
              OpStore %<dst> %tf_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param0>", param[0])
	                   .ReplaceStr(U"<param1>", (param[1] == nullptr ? U"" : param[1]))
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: U32 */
KYTY_RECOMPILER_FUNC(Recompile_VCvt_XXX_F32_SVdstSVsrc0)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
	EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	String load0;

	if (!operand_load_float(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC
	// TODO() check SP_DENORM_IN

	static const char32_t* text = UR"(    
    <load0>
    <param0>
    <param1>
    <param2>
    %t_<index> = OpBitcast %float %t2_<index>
    OpStore %<dst> %t_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<param0>", param[0])
	                   .ReplaceStr(U"<param1>", param[1])
	                   .ReplaceStr(U"<param2>", (param[2] == nullptr ? U"" : param[2]))
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: U32, UbyteX */
KYTY_RECOMPILER_FUNC(Recompile_VCvtF32_XXX_SVdstSVsrc0)
{
	const auto& inst = code.GetInstructions().At(index);

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
	EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	String load0;

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC
	// TODO() check SP_ROUND

	static const char32_t* text = UR"(    
    <load0>
    <param0>
    <param1>
    OpStore %<dst> %t_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<param0>", param[0])
	                   .ReplaceStr(U"<param1>", (param[1] == nullptr ? U"" : param[1]))
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Sad, Bfe, MadU32U24 */
KYTY_RECOMPILER_FUNC(Recompile_V_XXX_U32_VdstVsrc0Vsrc1Vsrc2)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;
	String load2;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(inst.dst.clamp);
	EXIT_NOT_IMPLEMENTED(inst.dst.multiplier != 1.0f);

	auto dst_value = operand_variable_to_str(inst.dst);

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[2], U"t2_<index>", index_str, &load2))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC
	// TODO() Sad: use only lower 16 bits of Vaccum

	static const char32_t* text = UR"(
               <load0>
               <load1>
               <load2>
               <param0>
               <param1>
               <param2>
               <param3>
         %tf_<index> = OpBitcast %float %t_<index>
               OpStore %<dst> %tf_<index>
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<load2>", load2)
	                   .ReplaceStr(U"<param0>", param[0])
	                   .ReplaceStr(U"<param1>", param[1])
	                   .ReplaceStr(U"<param2>", (param[2] == nullptr ? U"" : param[2]))
	                   .ReplaceStr(U"<param3>", (param[3] == nullptr ? U"" : param[3]))
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

/* XXX: Add, Sub, Subrev */
KYTY_RECOMPILER_FUNC(Recompile_V_XXX_U32_VdstSdst2Vsrc0Vsrc1)
{
	const auto& inst = code.GetInstructions().At(index);

	String load0;
	String load1;

	String index_str = String::FromPrintf("%u", index);

	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst));
	EXIT_NOT_IMPLEMENTED(!operand_is_variable(inst.dst2));

	auto dst_value   = operand_variable_to_str(inst.dst);
	auto dst2_value0 = operand_variable_to_str(inst.dst2, 0);
	auto dst2_value1 = operand_variable_to_str(inst.dst2, 1);

	EXIT_NOT_IMPLEMENTED(operand_is_exec(inst.dst2));

	EXIT_NOT_IMPLEMENTED(dst_value.type != SpirvType::Float);
	EXIT_NOT_IMPLEMENTED(dst2_value0.type != SpirvType::Uint);

	if (!operand_load_uint(spirv, inst.src[0], U"t0_<index>", index_str, &load0))
	{
		return false;
	}
	if (!operand_load_uint(spirv, inst.src[1], U"t1_<index>", index_str, &load1))
	{
		return false;
	}

	// TODO() check VSKIP
	// TODO() check EXEC

	static const char32_t* text = UR"(
              <load0>
              <load1>
        <param>
        %t208_<index> = OpCompositeExtract %uint %t_<index> 1
        %t209_<index> = OpCompositeExtract %uint %t_<index> 0
        %t210_<index> = OpBitcast %float %t209_<index>
               OpStore %<dst> %t210_<index>		   
        %exec_lo_u_<index> = OpLoad %uint %exec_lo
        %exec_hi_u_<index> = OpLoad %uint %exec_hi ; unused
        %exec_lo_b_<index> = OpINotEqual %bool %exec_lo_u_<index> %uint_0
        %t213_<index> = OpSelect %uint %exec_lo_b_<index> %t208_<index> %uint_0
               OpStore %<dst2_0> %t213_<index>
               OpStore %<dst2_1> %uint_0
)";
	*dst_source += String(text)
	                   .ReplaceStr(U"<dst>", dst_value.value)
	                   .ReplaceStr(U"<dst2_0>", dst2_value0.value)
	                   .ReplaceStr(U"<dst2_1>", dst2_value1.value)
	                   .ReplaceStr(U"<load0>", load0)
	                   .ReplaceStr(U"<load1>", load1)
	                   .ReplaceStr(U"<param>", param[0])
	                   .ReplaceStr(U"<index>", index_str);

	return true;
}

static RecompilerFunc g_recomp_func[] = {
    // clang-format off
    {Recompile_BufferLoadDword_Vdata1VaddrSvSoffsIdxen,    ShaderInstructionType::BufferLoadDword,     ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxen,        {U""}},
    {Recompile_BufferLoadFormatX_Vdata1VaddrSvSoffsIdxen,  ShaderInstructionType::BufferLoadFormatX,   ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxen,        {U""}},
    {Recompile_BufferStoreDword_Vdata1VaddrSvSoffsIdxen,   ShaderInstructionType::BufferStoreDword,    ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxen,        {U""}},
    {Recompile_BufferStoreFormatX_Vdata1VaddrSvSoffsIdxen, ShaderInstructionType::BufferStoreFormatX,  ShaderInstructionFormat::Vdata1VaddrSvSoffsIdxen,        {U""}},

    {Recompile_DsAppend_VdstGds,                           ShaderInstructionType::DsAppend,            ShaderInstructionFormat::VdstGds,                        {U""}},
    {Recompile_DsConsume_VdstGds,                          ShaderInstructionType::DsConsume,           ShaderInstructionFormat::VdstGds,                        {U""}},

	{Recompile_Exp_Mrt0OffOffComprVmDone,                  ShaderInstructionType::Exp,                 ShaderInstructionFormat::Mrt0OffOffComprVmDone,          {U""}},
    {Recompile_Exp_Mrt0Vsrc0Vsrc1ComprVmDone,              ShaderInstructionType::Exp,                 ShaderInstructionFormat::Mrt0Vsrc0Vsrc1ComprVmDone,      {U""}},
    {Recompile_Exp_Mrt0Vsrc0Vsrc1Vsrc2Vsrc3VmDone,         ShaderInstructionType::Exp,                 ShaderInstructionFormat::Mrt0Vsrc0Vsrc1Vsrc2Vsrc3VmDone, {U""}},
    {Recompile_Exp_Param_XXX_Vsrc0Vsrc1Vsrc2Vsrc3,         ShaderInstructionType::Exp,                 ShaderInstructionFormat::Param0Vsrc0Vsrc1Vsrc2Vsrc3,     {U"param0"}},
    {Recompile_Exp_Param_XXX_Vsrc0Vsrc1Vsrc2Vsrc3,         ShaderInstructionType::Exp,                 ShaderInstructionFormat::Param1Vsrc0Vsrc1Vsrc2Vsrc3,     {U"param1"}},
    {Recompile_Exp_Param_XXX_Vsrc0Vsrc1Vsrc2Vsrc3,         ShaderInstructionType::Exp,                 ShaderInstructionFormat::Param2Vsrc0Vsrc1Vsrc2Vsrc3,     {U"param2"}},
    {Recompile_Exp_Param_XXX_Vsrc0Vsrc1Vsrc2Vsrc3,         ShaderInstructionType::Exp,                 ShaderInstructionFormat::Param3Vsrc0Vsrc1Vsrc2Vsrc3,     {U"param3"}},
    {Recompile_Exp_Pos0Vsrc0Vsrc1Vsrc2Vsrc3Done,           ShaderInstructionType::Exp,                 ShaderInstructionFormat::Pos0Vsrc0Vsrc1Vsrc2Vsrc3Done,   {U""}},

    {Recompile_ImageSample_Vdata3Vaddr3StSsDmask7,         ShaderInstructionType::ImageSample,         ShaderInstructionFormat::Vdata3Vaddr3StSsDmask7,         {U""}},
    {Recompile_ImageSample_Vdata4Vaddr3StSsDmaskF,         ShaderInstructionType::ImageSample,         ShaderInstructionFormat::Vdata4Vaddr3StSsDmaskF,         {U""}},

    {Recompile_SBufferLoadDword_SdstSvSoffset,             ShaderInstructionType::SBufferLoadDword,    ShaderInstructionFormat::SdstSvSoffset,                  {U""}},
    {Recompile_SBufferLoadDwordx2_Sdst2SvSoffset,          ShaderInstructionType::SBufferLoadDwordx2,  ShaderInstructionFormat::Sdst2SvSoffset,                 {U""}},
    {Recompile_SBufferLoadDwordx4_Sdst4SvSoffset,          ShaderInstructionType::SBufferLoadDwordx4,  ShaderInstructionFormat::Sdst4SvSoffset,                 {U""}},
    {Recompile_SBufferLoadDwordx8_Sdst8SvSoffset,          ShaderInstructionType::SBufferLoadDwordx8,  ShaderInstructionFormat::Sdst8SvSoffset,                 {U""}},
	{Recompile_SBufferLoadDwordx16_Sdst16SvSoffset,        ShaderInstructionType::SBufferLoadDwordx16, ShaderInstructionFormat::Sdst16SvSoffset,                {U""}},

    {Recompile_SCbranchExecz_Label,                        ShaderInstructionType::SCbranchExecz,       ShaderInstructionFormat::Label,                          {U""}},
    {Recompile_SCbranchScc0_Label,                         ShaderInstructionType::SCbranchScc0,        ShaderInstructionFormat::Label,                          {U""}},

	{Recompile_SEndpgm_Empty,                              ShaderInstructionType::SEndpgm,             ShaderInstructionFormat::Empty,                          {U""}},

    {Recompile_SLoadDwordx4_Sdst4SbaseSoffset,             ShaderInstructionType::SLoadDwordx4,        ShaderInstructionFormat::Sdst4SbaseSoffset,              {U""}},
    {Recompile_SLoadDwordx8_Sdst8SbaseSoffset,             ShaderInstructionType::SLoadDwordx8,        ShaderInstructionFormat::Sdst8SbaseSoffset,              {U""}},

    {Recompile_S_XXX_B64_Sdst2Ssrc02Ssrc12, ShaderInstructionType::SAndn2B64, ShaderInstructionFormat::Sdst2Ssrc02Ssrc12, {U"%ta_<index> = OpNot %uint %t2_<index>",
                                                                                                                           U"%tb_<index> = OpBitwiseAnd %uint %t0_<index> %ta_<index>",
                                                                                                                           U"%tc_<index> = OpNot %uint %t3_<index>",
                                                                                                                           U"%td_<index> = OpBitwiseAnd %uint %t1_<index> %tc_<index>"}, SccCheck::NonZero},
    {Recompile_S_XXX_B64_Sdst2Ssrc02Ssrc12, ShaderInstructionType::SNorB64,   ShaderInstructionFormat::Sdst2Ssrc02Ssrc12, {U"%ta_<index> = OpBitwiseOr %uint %t0_<index> %t2_<index>",
                                                                                                                           U"%tb_<index> = OpNot %uint %ta_<index>",
                                                                                                                           U"%tc_<index> = OpBitwiseOr %uint %t1_<index> %t3_<index>",
                                                                                                                           U"%td_<index> = OpNot %uint %tc_<index>"}, SccCheck::NonZero},
    {Recompile_S_XXX_B64_Sdst2Ssrc02Ssrc12, ShaderInstructionType::SOrB64,    ShaderInstructionFormat::Sdst2Ssrc02Ssrc12, {U"%tb_<index> = OpBitwiseOr %uint %t0_<index> %t2_<index>",
                                                                                                                           U"%td_<index> = OpBitwiseOr %uint %t1_<index> %t3_<index>"}, SccCheck::NonZero},

    {Recompile_VCvtPkrtzF16F32_SVdstSVsrc0SVsrc1, ShaderInstructionType::VCvtPkrtzF16F32, ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U""}},
    {Recompile_VMbcntHiU32B32_SVdstSVsrc0SVsrc1,  ShaderInstructionType::VMbcntHiU32B32,  ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U""}},
    {Recompile_VMbcntLoU32B32_SVdstSVsrc0SVsrc1,  ShaderInstructionType::VMbcntLoU32B32,  ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U""}},
    {Recompile_S_XXX_B32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::SAndB32,         ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpBitwiseAnd %uint %t0_<index> %t1_<index>"}, SccCheck::NonZero},
    {Recompile_S_XXX_B32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::SCselectB32,     ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t22_<index> = OpLoad %uint %scc", U"%t2_<index> = OpINotEqual %bool %t22_<index> %uint_0", U"%t_<index> = OpSelect %uint %t2_<index> %t0_<index> %t1_<index>"}, SccCheck::None},
    {Recompile_S_XXX_B32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::SLshlB32,        ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%ts_<index> = OpBitwiseAnd %uint %t1_<index> %uint_31", U"%t_<index> = OpShiftLeftLogical %uint %t0_<index> %ts_<index>"}, SccCheck::NonZero},
    {Recompile_S_XXX_B32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::SLshrB32,        ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%ts_<index> = OpBitwiseAnd %uint %t1_<index> %uint_31", U"%t_<index> = OpShiftRightLogical %uint %t0_<index> %ts_<index>"}, SccCheck::NonZero},
    {Recompile_S_XXX_I32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::SAddI32,         ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpIAdd %int %t0_<index> %t1_<index>"}, SccCheck::Overflow},
    {Recompile_S_XXX_I32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::SMulI32,         ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpIMul %int %t0_<index> %t1_<index>"}, SccCheck::None},
    {Recompile_V_XXX_B32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VAndB32,         ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpBitwiseAnd %uint %t0_<index> %t1_<index>"}},
    {Recompile_V_XXX_B32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VLshlrevB32,     ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%ts_<index> = OpBitwiseAnd %uint %t0_<index> %uint_31", U"%t_<index> = OpShiftLeftLogical %uint %t1_<index> %ts_<index>"}},
    {Recompile_V_XXX_B32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VLshrB32,        ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%ts_<index> = OpBitwiseAnd %uint %t1_<index> %uint_31", U"%t_<index> = OpShiftRightLogical %uint %t0_<index> %ts_<index>"}},
    {Recompile_V_XXX_B32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VLshrrevB32,     ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%ts_<index> = OpBitwiseAnd %uint %t0_<index> %uint_31", U"%t_<index> = OpShiftRightLogical %uint %t1_<index> %ts_<index>"}},
    {Recompile_V_XXX_B32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VMulU32U24,      ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%tu0_<index> = OpBitwiseAnd %uint %t0_<index> %uint_0x00ffffff", U"%tu1_<index> = OpBitwiseAnd %uint %t1_<index> %uint_0x00ffffff", U"%t_<index> = OpFunctionCall %uint %mul_lo_uint %tu0_<index> %tu1_<index>"}},
    {Recompile_V_XXX_F32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VMacF32,         ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpExtInst %float %GLSL_std_450 Fma %t0_<index> %t1_<index> %tdst_<index>"}},
    {Recompile_V_XXX_F32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VMaxF32,         ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpExtInst %float %GLSL_std_450 FMax %t0_<index> %t1_<index>"}},
    {Recompile_V_XXX_F32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VMinF32,         ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpExtInst %float %GLSL_std_450 FMin %t0_<index> %t1_<index>"}},
    {Recompile_V_XXX_F32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VMulF32,         ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpFMul %float %t0_<index> %t1_<index>"}},
    {Recompile_V_XXX_F32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VSubF32,         ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpFSub %float %t0_<index> %t1_<index>"}},
    {Recompile_V_XXX_F32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VSubrevF32,      ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpFSub %float %t1_<index> %t0_<index>"}},
	{Recompile_V_XXX_I32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VAshrrevI32,     ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%ts_<index> = OpBitwiseAnd %int %t0_<index> %int_31", U"%t_<index> = OpShiftRightArithmetic %int %t1_<index> %ts_<index>"}},
	{Recompile_V_XXX_I32_SVdstSVsrc0SVsrc1,       ShaderInstructionType::VMulLoI32,       ShaderInstructionFormat::SVdstSVsrc0SVsrc1,  {U"%t_<index> = OpFunctionCall %int %mul_lo_int %t0_<index> %t1_<index>"}},


    {Recompile_SMovB32_SVdstSVsrc0,            ShaderInstructionType::SMovB32,             ShaderInstructionFormat::SVdstSVsrc0, {U""}},
    {Recompile_VMovB32_SVdstSVsrc0,            ShaderInstructionType::VMovB32,             ShaderInstructionFormat::SVdstSVsrc0, {U""}},
    {Recompile_V_XXX_B32_SVdstSVsrc0,          ShaderInstructionType::VNotB32,             ShaderInstructionFormat::SVdstSVsrc0, {U"%t_<index> = OpNot %uint %t0_<index>"}},
    {Recompile_V_XXX_F32_SVdstSVsrc0,          ShaderInstructionType::VRcpF32,             ShaderInstructionFormat::SVdstSVsrc0, {U"%t_<index> = OpFDiv %float %float_1_000000 %t0_<index>"}},
    {Recompile_V_XXX_F32_SVdstSVsrc0,          ShaderInstructionType::VRsqF32,             ShaderInstructionFormat::SVdstSVsrc0, {U"%t_<index> = OpExtInst %float %GLSL_std_450 InverseSqrt %t0_<index>"}},
    {Recompile_V_XXX_F32_SVdstSVsrc0,          ShaderInstructionType::VSqrtF32,            ShaderInstructionFormat::SVdstSVsrc0, {U"%t_<index> = OpExtInst %float %GLSL_std_450 Sqrt %t0_<index>"}},
    {Recompile_VCvt_XXX_F32_SVdstSVsrc0,       ShaderInstructionType::VCvtU32F32,          ShaderInstructionFormat::SVdstSVsrc0, {U"%t1_<index> = OpExtInst %float %GLSL_std_450 Trunc %t0_<index>", U"%t2_<index> = OpConvertFToU %uint %t1_<index>"}},
    {Recompile_VCvtF32_XXX_SVdstSVsrc0,        ShaderInstructionType::VCvtF32U32,          ShaderInstructionFormat::SVdstSVsrc0, {U"%t_<index> = OpConvertUToF %float %t0_<index>"}},
    {Recompile_VCvtF32_XXX_SVdstSVsrc0,        ShaderInstructionType::VCvtF32Ubyte0,       ShaderInstructionFormat::SVdstSVsrc0, {U"%tb_<index> = OpBitFieldUExtract %uint %t0_<index> %uint_0 %uint_8", U"%t_<index> = OpConvertUToF %float %tb_<index>"}},
    {Recompile_VCvtF32_XXX_SVdstSVsrc0,        ShaderInstructionType::VCvtF32Ubyte1,       ShaderInstructionFormat::SVdstSVsrc0, {U"%tb_<index> = OpBitFieldUExtract %uint %t0_<index> %uint_8 %uint_8", U"%t_<index> = OpConvertUToF %float %tb_<index>"}},
    {Recompile_VCvtF32_XXX_SVdstSVsrc0,        ShaderInstructionType::VCvtF32Ubyte2,       ShaderInstructionFormat::SVdstSVsrc0, {U"%tb_<index> = OpBitFieldUExtract %uint %t0_<index> %uint_16 %uint_8", U"%t_<index> = OpConvertUToF %float %tb_<index>"}},
    {Recompile_VCvtF32_XXX_SVdstSVsrc0,        ShaderInstructionType::VCvtF32Ubyte3,       ShaderInstructionFormat::SVdstSVsrc0, {U"%tb_<index> = OpBitFieldUExtract %uint %t0_<index> %uint_24 %uint_8", U"%t_<index> = OpConvertUToF %float %tb_<index>"}},

    {Recompile_SAndSaveexecB64_Sdst2Ssrc02,    ShaderInstructionType::SAndSaveexecB64,     ShaderInstructionFormat::Sdst2Ssrc02, {U""}, SccCheck::NonZero},
    {Recompile_SMovB64_Sdst2Ssrc02,            ShaderInstructionType::SMovB64,             ShaderInstructionFormat::Sdst2Ssrc02, {U""}},
    {Recompile_SSwappcB64_Sdst2Ssrc02,         ShaderInstructionType::SSwappcB64,          ShaderInstructionFormat::Sdst2Ssrc02, {U""}},
    {Recompile_SWqmB64_Sdst2Ssrc02,            ShaderInstructionType::SWqmB64,             ShaderInstructionFormat::Sdst2Ssrc02, {U""}},

	{Recompile_SMovB64_Sdst2Ssrc0,            ShaderInstructionType::SMovB64,             ShaderInstructionFormat::Sdst2Ssrc0, {U""}},

    {Recompile_SWaitcnt_Imm,                   ShaderInstructionType::SWaitcnt,            ShaderInstructionFormat::Imm,         {U""}},

    {Recompile_TBufferLoadFormatXyzw_Vdata4VaddrSvSoffsIdxenFloat4,       ShaderInstructionType::TBufferLoadFormatXyzw, ShaderInstructionFormat::Vdata4VaddrSvSoffsIdxenFloat4,  {U""}},
	{Recompile_TBufferLoadFormatXyzw_Vdata4Vaddr2SvSoffsOffenIdxenFloat4, ShaderInstructionType::TBufferLoadFormatXyzw, ShaderInstructionFormat::Vdata4Vaddr2SvSoffsOffenIdxenFloat4,  {U""}},

    {Recompile_V_XXX_U32_VdstSdst2Vsrc0Vsrc1,  ShaderInstructionType::VAddI32,    ShaderInstructionFormat::VdstSdst2Vsrc0Vsrc1,  {U"%t_<index> = OpIAddCarry %ResTypeU %t0_<index> %t1_<index>"}},
    {Recompile_V_XXX_U32_VdstSdst2Vsrc0Vsrc1,  ShaderInstructionType::VSubI32,    ShaderInstructionFormat::VdstSdst2Vsrc0Vsrc1,  {U"%t_<index> = OpISubBorrow %ResTypeU %t0_<index> %t1_<index>"}},
    {Recompile_V_XXX_U32_VdstSdst2Vsrc0Vsrc1,  ShaderInstructionType::VSubrevI32, ShaderInstructionFormat::VdstSdst2Vsrc0Vsrc1,  {U"%t_<index> = OpISubBorrow %ResTypeU %t1_<index> %t0_<index>"}},

    {Recompile_VCmp_XXX_F32_SmaskVsrc0Vsrc1,  ShaderInstructionType::VCmpEqF32,    ShaderInstructionFormat::SmaskVsrc0Vsrc1,      {U"OpFOrdEqual"}},
    {Recompile_VCmp_XXX_F32_SmaskVsrc0Vsrc1,  ShaderInstructionType::VCmpLeF32,    ShaderInstructionFormat::SmaskVsrc0Vsrc1,      {U"OpFOrdLessThanEqual"}},
    {Recompile_VCmp_XXX_F32_SmaskVsrc0Vsrc1,  ShaderInstructionType::VCmpNeqF32,   ShaderInstructionFormat::SmaskVsrc0Vsrc1,      {U"OpFUnordNotEqual"}},
    {Recompile_VCmp_XXX_I32_SmaskVsrc0Vsrc1,  ShaderInstructionType::VCmpEqU32,    ShaderInstructionFormat::SmaskVsrc0Vsrc1,      {U"OpIEqual"}},
    {Recompile_VCmp_XXX_I32_SmaskVsrc0Vsrc1,  ShaderInstructionType::VCmpGtI32,    ShaderInstructionFormat::SmaskVsrc0Vsrc1,      {U"OpSGreaterThan"}},
    {Recompile_VCmp_XXX_I32_SmaskVsrc0Vsrc1,  ShaderInstructionType::VCmpNeU32,    ShaderInstructionFormat::SmaskVsrc0Vsrc1,      {U"OpINotEqual"}},
    {Recompile_VCmp_XXX_U32_SmaskVsrc0Vsrc1,  ShaderInstructionType::VCmpLeU32,    ShaderInstructionFormat::SmaskVsrc0Vsrc1,      {U"OpULessThanEqual"}},
    {Recompile_VCmpx_XXX_I32_SmaskVsrc0Vsrc1, ShaderInstructionType::VCmpxEqU32,   ShaderInstructionFormat::SmaskVsrc0Vsrc1,      {U"OpIEqual"}},
	{Recompile_VCmpx_XXX_I32_SmaskVsrc0Vsrc1, ShaderInstructionType::VCmpxNeU32,   ShaderInstructionFormat::SmaskVsrc0Vsrc1,      {U"OpINotEqual"}},
    {Recompile_VCmpx_XXX_U32_SmaskVsrc0Vsrc1, ShaderInstructionType::VCmpxGtU32,   ShaderInstructionFormat::SmaskVsrc0Vsrc1,      {U"OpUGreaterThan"}},

    {Recompile_SCmp_XXX_U32_Ssrc0Ssrc1,  ShaderInstructionType::SCmpEqU32,    ShaderInstructionFormat::Ssrc0Ssrc1,      {U"OpIEqual"}},

    {Recompile_VCndmaskB32_VdstVsrc0Vsrc1Smask2,   ShaderInstructionType::VCndmaskB32,  ShaderInstructionFormat::VdstVsrc0Vsrc1Smask2, {U""}},

    {Recompile_VInterpP1F32_VdstVsrcAttrChan,      ShaderInstructionType::VInterpP1F32, ShaderInstructionFormat::VdstVsrcAttrChan,     {U""}},
    {Recompile_VInterpP2F32_VdstVsrcAttrChan,      ShaderInstructionType::VInterpP2F32, ShaderInstructionFormat::VdstVsrcAttrChan,     {U""}},

    {Recompile_VMadakF32_VdstVsrc0Vsrc1Vsrc2,  ShaderInstructionType::VMadakF32,    ShaderInstructionFormat::VdstVsrc0Vsrc1Vsrc2,  {U""}},
    {Recompile_VMadF32_VdstVsrc0Vsrc1Vsrc2,    ShaderInstructionType::VMadF32,      ShaderInstructionFormat::VdstVsrc0Vsrc1Vsrc2,  {U""}},
    {Recompile_V_XXX_U32_VdstVsrc0Vsrc1Vsrc2,  ShaderInstructionType::VSadU32,      ShaderInstructionFormat::VdstVsrc0Vsrc1Vsrc2,  {U"%td_<index> = OpFunctionCall %uint %abs_diff %t0_<index> %t1_<index>",
                                                                                                                                    U"%t_<index> = OpIAdd %uint %td_<index> %t2_<index>"}},
    {Recompile_V_XXX_U32_VdstVsrc0Vsrc1Vsrc2,  ShaderInstructionType::VBfeU32,      ShaderInstructionFormat::VdstVsrc0Vsrc1Vsrc2,  {U"%to_<index> = OpBitwiseAnd %uint %t1_<index> %uint_31",
                                                                                                                                    U"%ts_<index> = OpBitwiseAnd %uint %t2_<index> %uint_31",
                                                                                                                                    U"%t_<index> = OpBitFieldUExtract %uint %t0_<index> %to_<index> %ts_<index>"}},
    {Recompile_V_XXX_U32_VdstVsrc0Vsrc1Vsrc2,  ShaderInstructionType::VMadU32U24,   ShaderInstructionFormat::VdstVsrc0Vsrc1Vsrc2,  {U"%tu0_<index> = OpBitwiseAnd %uint %t0_<index> %uint_0x00ffffff",
                                                                                                                                    U"%tu1_<index> = OpBitwiseAnd %uint %t1_<index> %uint_0x00ffffff",
                                                                                                                                    U"%tm_<index> = OpFunctionCall %uint %mul_lo_uint %tu0_<index> %tu1_<index>",
                                                                                                                                    U"%t_<index> = OpIAdd %uint %tm_<index> %t2_<index>"}},

    // clang-format on
};

void Spirv::AddConstantUint(uint32_t u)
{
	ShaderConstant c {};
	c.u = u;
	AddConstant(SpirvType::Uint, c);
}

void Spirv::AddConstantInt(int i)
{
	ShaderConstant c {};
	c.i = i;
	AddConstant(SpirvType::Int, c);
}

void Spirv::AddConstantFloat(float f)
{
	ShaderConstant c {};
	c.f = f;
	AddConstant(SpirvType::Float, c);
}

void Spirv::AddConstant(ShaderOperand op)
{
	SpirvType type = SpirvType::Unknown;

	if (op.type == ShaderOperandType::LiteralConstant)
	{
		type = SpirvType::Uint;
	}
	if (op.type == ShaderOperandType::IntegerInlineConstant)
	{
		type = SpirvType::Int;
	}
	if (op.type == ShaderOperandType::FloatInlineConstant)
	{
		type = SpirvType::Float;
	}

	EXIT_NOT_IMPLEMENTED(type == SpirvType::Unknown);

	AddConstant(type, op.constant);
}

void Spirv::AddConstant(SpirvType type, ShaderConstant constant)
{
	for (const auto& c: m_constants)
	{
		if (c.type == type && c.constant.u == constant.u)
		{
			return;
		}
	}

	Constant c {};
	c.type     = type;
	c.constant = constant;
	c.type_str = Core::EnumName(type).ToLower();

	if (type == SpirvType::Uint)
	{
		c.value_str = constant.u < 256 ? String::FromPrintf("%u", constant.u) : String::FromPrintf("0x%08" PRIx32, constant.u);
	}
	if (type == SpirvType::Int)
	{
		c.value_str = String::FromPrintf("%d", constant.i);
	}
	if (type == SpirvType::Float)
	{
		c.value_str = String::FromPrintf("%f", constant.f);
	}

	c.id = String::FromPrintf("%s_%s", c.type_str.C_Str(), c.value_str.ReplaceChar(U'.', U'_').ReplaceChar(U'-', U'm').C_Str());

	m_constants.Add(c);
}

void Spirv::AddVariable(ShaderOperandType type, int register_id, int size)
{
	ShaderOperand op;
	op.type        = type;
	op.register_id = register_id;
	op.size        = size;
	AddVariable(op);
}

void Spirv::AddVariable(ShaderOperand op)
{
	if (operand_is_variable(op))
	{
		EXIT_IF(op.size == 0);

		for (int i = 0; i < op.size; i++)
		{
			Variable v;
			v.op.type        = op.type;
			v.op.register_id = op.register_id + i;
			v.op.size        = 1;

			if (op.type == ShaderOperandType::VccLo && op.size == 2 && i == 1)
			{
				v.op.type        = ShaderOperandType::VccHi;
				v.op.register_id = 0;
			}

			if (op.type == ShaderOperandType::ExecLo && op.size == 2 && i == 1)
			{
				v.op.type        = ShaderOperandType::ExecHi;
				v.op.register_id = 0;
			}

			if (!m_variables.Contains(v, [](auto v1, auto v2) { return v1.op == v2.op; }))
			{
				m_variables.Add(v);
			}
		}
	}
}

String Spirv::GetConstantUint(uint32_t u) const
{
	for (const auto& c: m_constants)
	{
		if (c.type == SpirvType::Uint && c.constant.u == u)
		{
			return c.id;
		}
	}

	return U"unknown_uint_constant";
}

String Spirv::GetConstantInt(int i) const
{
	for (const auto& c: m_constants)
	{
		if (c.type == SpirvType::Int && c.constant.i == i)
		{
			return c.id;
		}
	}

	return U"unknown_int_constant";
}

String Spirv::GetConstantFloat(float f) const
{
	for (const auto& c: m_constants)
	{
		if (c.type == SpirvType::Float && c.constant.f == f)
		{
			return c.id;
		}
	}

	return U"unknown_float_constant";
}

String Spirv::GetConstant(ShaderOperand op) const
{
	SpirvType type = SpirvType::Unknown;

	if (op.type == ShaderOperandType::LiteralConstant)
	{
		type = SpirvType::Uint;
	}
	if (op.type == ShaderOperandType::IntegerInlineConstant)
	{
		type = SpirvType::Int;
	}
	if (op.type == ShaderOperandType::FloatInlineConstant)
	{
		type = SpirvType::Float;
	}

	for (const auto& c: m_constants)
	{
		if (c.type == type && c.constant.u == op.constant.u)
		{
			return c.id;
		}
	}

	return U"unknown_operand_constant";
}

void Spirv::GenerateSource()
{
	m_source.Clear();

	switch (m_code.GetType())
	{
		case ShaderType::Pixel: m_bind = (m_ps_input_info != nullptr ? &m_ps_input_info->bind : nullptr); break;
		case ShaderType::Vertex: m_bind = (m_vs_input_info != nullptr ? &m_vs_input_info->bind : nullptr); break;
		case ShaderType::Compute: m_bind = (m_cs_input_info != nullptr ? &m_cs_input_info->bind : nullptr); break;
		default: m_bind = nullptr; break;
	}

	WriteHeader();
	WriteAnnotations();
	WriteTypes();
	WriteConstants();
	WriteGlobalVariables();
	WriteMainProlog();
	WriteLocalVariables();
	WriteInstructions();
	WriteMainEpilog();
	WriteFunctions();
}

void Spirv::WriteHeader()
{
	static const char32_t* header = UR"(
                ; Header
                OpCapability Shader 
%GLSL_std_450 = OpExtInstImport "GLSL.std.450" 
                OpMemoryModel Logical GLSL450 
                OpEntryPoint <Type> %main "main" <Variables>
                <ExecutionMode>
)";

	String header_str;
	String execution_mode;

	Core::StringList vars;

	if (m_bind != nullptr)
	{
		if (m_bind->storage_buffers.buffers_num > 0)
		{
			vars.Add(U"%buf");
		}
		if (m_bind->textures2D.textures_num > 0)
		{
			vars.Add(U"%textures2D");
		}
		if (m_bind->samplers.samplers_num > 0)
		{
			vars.Add(U"%samplers");
		}
		if (m_bind->gds_pointers.pointers_num > 0)
		{
			vars.Add(U"%gds");
		}
		if (m_bind->push_constant_size > 0)
		{
			vars.Add(U"%vsharp");
		}
	}

	switch (m_code.GetType())
	{
		case ShaderType::Pixel:
			vars.Add(U"%outColor");
			if (m_ps_input_info != nullptr)
			{
				for (uint32_t i = 0; i < m_ps_input_info->input_num; i++)
				{
					vars.Add(String::FromPrintf("%%attr%d", i));
				}
				if (m_ps_input_info->ps_pos_xy)
				{
					vars.Add(U"%gl_FragCoord");
				}
			}
			header_str     = String(header).ReplaceStr(U"<Type>", U"Fragment");
			execution_mode = U"OpExecutionMode %main OriginUpperLeft\n";
			// TODO() do we need PixelCenterInteger mode?
			break;
		case ShaderType::Vertex:
			if (m_vs_input_info != nullptr)
			{
				for (int i = 0; i < m_vs_input_info->resources_num; i++)
				{
					vars.Add(String::FromPrintf("%%attr%d", i));
				}
				for (int i = 0; i < m_vs_input_info->export_count; i++)
				{
					vars.Add(String::FromPrintf("%%param%d", i));
				}
			}
			vars.Add(U"%gl_VertexIndex");
			vars.Add(U"%outPerVertex");
			// vars.Add(U"%param0");
			header_str = String(header).ReplaceStr(U"<Type>", U"Vertex");
			break;
		case ShaderType::Compute:
			if (m_cs_input_info != nullptr)
			{
				execution_mode = String::FromPrintf("OpExecutionMode %%main LocalSize %u %u %u", m_cs_input_info->threads_num[0],
				                                    m_cs_input_info->threads_num[1], m_cs_input_info->threads_num[2]);
			}
			vars.Add(U"%gl_LocalInvocationID");
			vars.Add(U"%gl_WorkGroupID");
			header_str = String(header).ReplaceStr(U"<Type>", U"GLCompute");
			break;
		default: EXIT("unknown type: %s\n", Core::EnumName(m_code.GetType()).C_Str()); break;
	}

	m_source += header_str.ReplaceStr(U"<Variables>", vars.Concat(U' ')).ReplaceStr(U"<ExecutionMode>", execution_mode);
}

void Spirv::WriteAnnotations()
{
	static const char32_t* pixel_annotations   = UR"(
               ; Annotations
               OpDecorate %outColor Location 0
               <Variables>
)";
	static const char32_t* vertex_annotations  = UR"(
               ; Annotations
               OpDecorate %gl_VertexIndex BuiltIn VertexIndex
               OpMemberDecorate %gl_PerVertex 0 BuiltIn Position
               OpMemberDecorate %gl_PerVertex 1 BuiltIn PointSize 
               OpMemberDecorate %gl_PerVertex 2 BuiltIn ClipDistance
               OpMemberDecorate %gl_PerVertex 3 BuiltIn CullDistance
               OpDecorate %gl_PerVertex Block
               ; OpDecorate %param0 Location 0
               <Variables>
)";
	static const char32_t* compute_annotations = UR"(
               ; Annotations
               OpDecorate %gl_LocalInvocationID BuiltIn LocalInvocationId
               OpDecorate %gl_WorkGroupID BuiltIn WorkgroupId
               OpDecorate %gl_WorkGroupSize BuiltIn WorkgroupSize
               <Variables>
)";

	Core::StringList vars;

	switch (m_code.GetType())
	{
		case ShaderType::Pixel:
			if (m_ps_input_info != nullptr)
			{
				for (uint32_t i = 0; i < m_ps_input_info->input_num; i++)
				{
					vars.Add(String::FromPrintf("OpDecorate %%attr%d Location %d", i, i));
				}
				if (m_ps_input_info->ps_pos_xy)
				{
					vars.Add(U"OpDecorate %gl_FragCoord BuiltIn FragCoord");
				}
			}
			m_source += String(pixel_annotations).ReplaceStr(U"<Variables>", vars.Concat(U"\n" + String(U' ', 15)));
			break;
		case ShaderType::Vertex:
			if (m_vs_input_info != nullptr)
			{
				for (int i = 0; i < m_vs_input_info->resources_num; i++)
				{
					vars.Add(String::FromPrintf("OpDecorate %%attr%d Location %d", i, i));
				}
				for (int i = 0; i < m_vs_input_info->export_count; i++)
				{
					vars.Add(String::FromPrintf("OpDecorate %%param%d Location %d", i, i));
				}
			}
			m_source += String(vertex_annotations).ReplaceStr(U"<Variables>", vars.Concat(U"\n" + String(U' ', 15)));
			break;
		case ShaderType::Compute:
			m_source += String(compute_annotations).ReplaceStr(U"<Variables>", vars.Concat(U"\n" + String(U' ', 15)));
			break;
		default: EXIT("unknown type: %s\n", Core::EnumName(m_code.GetType()).C_Str()); break;
	}

	static const char32_t* storage_buffers_annotations = UR"(
       OpDecorate %buffers_runtimearr_float ArrayStride 4
       OpMemberDecorate %BufferObject 0 Offset 0
       OpDecorate %BufferObject Block
       OpDecorate %buf DescriptorSet <DescriptorSet>
       OpDecorate %buf Binding <BindingIndex>
)";

	static const char32_t* textures_annotations = UR"(
       OpDecorate %textures2D DescriptorSet <DescriptorSet>
       OpDecorate %textures2D Binding <BindingIndex>
)";

	static const char32_t* samplers_annotations = UR"(
       OpDecorate %samplers DescriptorSet <DescriptorSet>
       OpDecorate %samplers Binding <BindingIndex>
)";
	static const char32_t* gds_annotations      = UR"(
               OpDecorate %gds_runtimearr_uint ArrayStride 4
               OpMemberDecorate %GDS 0 Coherent
               OpMemberDecorate %GDS 0 Offset 0
               OpDecorate %GDS Block
               OpDecorate %gds DescriptorSet <DescriptorSet>
               OpDecorate %gds Binding <BindingIndex>
)";

	static const char32_t* vsharp_annotations = UR"(
       OpDecorate %vsharp_arr_uint_uint_4 ArrayStride 4
       OpDecorate %vsharp_arr__arr_uint_uint_4_uint_<buffers_num> ArrayStride 16
	   OpMemberDecorate %BufferResource 0 Offset <Offset>
       OpDecorate %BufferResource Block
)";

	if (m_bind != nullptr)
	{
		if (m_bind->storage_buffers.buffers_num > 0)
		{
			m_source += String(storage_buffers_annotations)
			                .ReplaceStr(U"<DescriptorSet>", String::FromPrintf("%u", m_bind->descriptor_set_slot))
			                .ReplaceStr(U"<BindingIndex>", String::FromPrintf("%d", m_bind->storage_buffers.binding_index));
		}
		if (m_bind->textures2D.textures_num > 0)
		{
			m_source += String(textures_annotations)
			                .ReplaceStr(U"<DescriptorSet>", String::FromPrintf("%u", m_bind->descriptor_set_slot))
			                .ReplaceStr(U"<BindingIndex>", String::FromPrintf("%d", m_bind->textures2D.binding_index));
		}
		if (m_bind->samplers.samplers_num > 0)
		{
			m_source += String(samplers_annotations)
			                .ReplaceStr(U"<DescriptorSet>", String::FromPrintf("%u", m_bind->descriptor_set_slot))
			                .ReplaceStr(U"<BindingIndex>", String::FromPrintf("%d", m_bind->samplers.binding_index));
		}
		if (m_bind->gds_pointers.pointers_num > 0)
		{
			m_source += String(gds_annotations)
			                .ReplaceStr(U"<DescriptorSet>", String::FromPrintf("%u", m_bind->descriptor_set_slot))
			                .ReplaceStr(U"<BindingIndex>", String::FromPrintf("%d", m_bind->gds_pointers.binding_index));
		}
		if (m_bind->push_constant_size > 0)
		{
			m_source += String(vsharp_annotations)
			                .ReplaceStr(U"<buffers_num>", String::FromPrintf("%d", m_bind->push_constant_size / 16))
			                .ReplaceStr(U"<Offset>", String::FromPrintf("%u", m_bind->push_constant_offset));
		}
	}
}

void Spirv::WriteTypes()
{
	static const char32_t* types = UR"(
                               ; Types
                         %void = OpTypeVoid 
                        %float = OpTypeFloat 32
                          %int = OpTypeInt 32 1 
                         %uint = OpTypeInt 32 0
                         %bool = OpTypeBool 
                      %v2float = OpTypeVector %float 2
                      %v3float = OpTypeVector %float 3 
                      %v4float = OpTypeVector %float 4 
                       %v3uint = OpTypeVector %uint 3
               %_ptr_Input_int = OpTypePointer Input %int
              %_ptr_Input_uint = OpTypePointer Input %uint
             %_ptr_Input_float = OpTypePointer Input %float
           %_ptr_Input_v2float = OpTypePointer Input %v2float
           %_ptr_Input_v3float = OpTypePointer Input %v3float
           %_ptr_Input_v4float = OpTypePointer Input %v4float
            %_ptr_Input_v3uint = OpTypePointer Input %v3uint
          %_ptr_Output_v4float = OpTypePointer Output %v4float
          %_ptr_Function_float = OpTypePointer Function %float
           %_ptr_Function_bool = OpTypePointer Function %bool
            %_ptr_Function_int = OpTypePointer Function %int
           %_ptr_Function_uint = OpTypePointer Function %uint
        %_ptr_Function_v2float = OpTypePointer Function %v2float
        %_ptr_Function_v3float = OpTypePointer Function %v3float
        %_ptr_Function_v4float = OpTypePointer Function %v4float
           %_ptr_Uniform_float = OpTypePointer Uniform %float
     %_ptr_StorageBuffer_float = OpTypePointer StorageBuffer %float
      %_ptr_StorageBuffer_uint = OpTypePointer StorageBuffer %uint
                     %ResTypeI = OpTypeStruct %int %int
                     %ResTypeU = OpTypeStruct %uint %uint
                %function_void = OpTypeFunction %void
              %function_fetch1 = OpTypeFunction %void %_ptr_Function_float %_ptr_Function_float 
              %function_fetch2 = OpTypeFunction %void %_ptr_Function_float %_ptr_Function_float %_ptr_Function_v2float
              %function_fetch3 = OpTypeFunction %void %_ptr_Function_float %_ptr_Function_float %_ptr_Function_float %_ptr_Function_v3float
              %function_fetch4 = OpTypeFunction %void %_ptr_Function_float %_ptr_Function_float %_ptr_Function_float %_ptr_Function_float %_ptr_Function_v4float
                 %function_u_u = OpTypeFunction %uint %uint %uint
                 %function_i_i = OpTypeFunction %int %int %int
   %function_tbuffer_load_format_xyzw = OpTypeFunction %void %_ptr_Function_float %_ptr_Function_float %_ptr_Function_float %_ptr_Function_float %_ptr_Function_int %_ptr_Function_int %_ptr_Function_int %_ptr_Function_int %_ptr_Function_int
   %function_buffer_load_store_float1 = OpTypeFunction %void %_ptr_Function_float %_ptr_Function_int %_ptr_Function_int %_ptr_Function_int %_ptr_Function_int
         %function_buffer_load_float4 = OpTypeFunction %void %_ptr_Function_float %_ptr_Function_float %_ptr_Function_float %_ptr_Function_float %_ptr_Function_int %_ptr_Function_int %_ptr_Function_int %_ptr_Function_int
%function_tbuffer_load_store_format_x = OpTypeFunction %void %_ptr_Function_float %_ptr_Function_int %_ptr_Function_int %_ptr_Function_int %_ptr_Function_int %_ptr_Function_int
         %function_sbuffer_load_dword = OpTypeFunction %void %_ptr_Function_uint %_ptr_Function_int %_ptr_Function_int
       %function_sbuffer_load_dword_2 = OpTypeFunction %void %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_int %_ptr_Function_int
       %function_sbuffer_load_dword_4 = OpTypeFunction %void %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_int %_ptr_Function_int
       %function_sbuffer_load_dword_8 = OpTypeFunction %void %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_int %_ptr_Function_int
      %function_sbuffer_load_dword_16 = OpTypeFunction %void %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_uint %_ptr_Function_int %_ptr_Function_int
)";

	static const char32_t* pixel_types = UR"(	
)";

	static const char32_t* vertex_types = UR"(	
            %array_length = OpConstant %uint 1
        %int_per_vertex_0 = OpConstant %int 0
       %_arr_float_uint_1 = OpTypeArray %float %array_length
            %gl_PerVertex = OpTypeStruct %v4float %float %_arr_float_uint_1 %_arr_float_uint_1
%_ptr_Output_gl_PerVertex = OpTypePointer Output %gl_PerVertex
)";

	static const char32_t* compute_types = UR"(	
)";

	m_source += types;

	switch (m_code.GetType())
	{
		case ShaderType::Vertex: m_source += vertex_types; break;
		case ShaderType::Pixel: m_source += pixel_types; break;
		case ShaderType::Compute: m_source += compute_types; break;
		default: EXIT("unknown type: %s\n", Core::EnumName(m_code.GetType()).C_Str()); break;
	}

	static const char32_t* storage_buffers_types = UR"(	
                        %buffers_runtimearr_float = OpTypeRuntimeArray %float
                             %BufferObject = OpTypeStruct %buffers_runtimearr_float
           %buffers_num_uint_<buffers_num> = OpConstant %uint <buffers_num>
     %_arr_BufferObject_uint_<buffers_num> = OpTypeArray %BufferObject %buffers_num_uint_<buffers_num>
%_ptr_StorageBuffer__arr_BufferObject_uint_<buffers_num> = OpTypePointer StorageBuffer %_arr_BufferObject_uint_<buffers_num>
)";

	static const char32_t* textures_types = UR"(
                             %SampledImage = OpTypeImage %float 2D 0 0 0 1 Unknown
              %textures2D_uint_<buffers_num> = OpConstant %uint <buffers_num>
     %_arr_SampledImage_uint_<buffers_num> = OpTypeArray %SampledImage %textures2D_uint_<buffers_num>
%_ptr_UniformConstant__arr_SampledImage_uint_<buffers_num> = OpTypePointer UniformConstant %_arr_SampledImage_uint_<buffers_num>
        %_ptr_UniformConstant_SampledImage = OpTypePointer UniformConstant %SampledImage
                                       %_SampledImage = OpTypeSampledImage %SampledImage	
)";

	static const char32_t* samplers_types = UR"(	
                                  %Sampler = OpTypeSampler
              %samplers_uint_<buffers_num> = OpConstant %uint <buffers_num>
          %_arr_Sampler_uint_<buffers_num> = OpTypeArray %Sampler %samplers_uint_<buffers_num>
%_ptr_UniformConstant__arr_Sampler_uint_<buffers_num> = OpTypePointer UniformConstant %_arr_Sampler_uint_<buffers_num>
             %_ptr_UniformConstant_Sampler = OpTypePointer UniformConstant %Sampler
)";

	static const char32_t* gds_types = UR"(	
            %gds_runtimearr_uint = OpTypeRuntimeArray %uint
                    %GDS = OpTypeStruct %gds_runtimearr_uint
            %_ptr_StorageBuffer_GDS = OpTypePointer StorageBuffer %GDS
)";

	static const char32_t* vsharp_types = UR"(	
           %vsharp_buffers_num_uint_<buffers_num> = OpConstant %uint <buffers_num>
                        %vsharp_num_uint_4 = OpConstant %uint 4
                   %vsharp_arr_uint_uint_4 = OpTypeArray %uint %vsharp_num_uint_4
%vsharp_arr__arr_uint_uint_4_uint_<buffers_num> = OpTypeArray %vsharp_arr_uint_uint_4 %vsharp_buffers_num_uint_<buffers_num>
                           %BufferResource = OpTypeStruct %vsharp_arr__arr_uint_uint_4_uint_<buffers_num>           
         %_ptr_PushConstant_BufferResource = OpTypePointer PushConstant %BufferResource
                   %_ptr_PushConstant_uint = OpTypePointer PushConstant %uint
)";

	if (m_bind != nullptr)
	{
		if (m_bind->storage_buffers.buffers_num > 0)
		{
			m_source +=
			    String(storage_buffers_types).ReplaceStr(U"<buffers_num>", String::FromPrintf("%d", m_bind->storage_buffers.buffers_num));
		}
		if (m_bind->textures2D.textures_num > 0)
		{
			m_source += String(textures_types).ReplaceStr(U"<buffers_num>", String::FromPrintf("%d", m_bind->textures2D.textures_num));
		}
		if (m_bind->samplers.samplers_num > 0)
		{
			m_source += String(samplers_types).ReplaceStr(U"<buffers_num>", String::FromPrintf("%d", m_bind->samplers.samplers_num));
		}
		if (m_bind->gds_pointers.pointers_num > 0)
		{
			m_source += String(gds_types);
		}
		if (m_bind->push_constant_size > 0)
		{
			m_source += String(vsharp_types).ReplaceStr(U"<buffers_num>", String::FromPrintf("%d", m_bind->push_constant_size / 16));
		}
	}
}

void Spirv::WriteConstants()
{
	FindConstants();

	static const char32_t* comment = UR"(	
    ; Constants
    %true = OpConstantTrue %bool
    %false = OpConstantFalse %bool
)";

	m_source += comment;

	for (const auto& c: m_constants)
	{
		m_source += String::FromPrintf("%%%s = OpConstant %%%s %s\n", c.id.C_Str(), c.type_str.C_Str(), c.value_str.C_Str());
	}
}

void Spirv::WriteGlobalVariables()
{
	static const char32_t* pixel_variables   = UR"(
              ;Variables
   %outColor = OpVariable %_ptr_Output_v4float Output
               <Variables> 
)";
	static const char32_t* vertex_variables  = UR"(
              ;Variables
    %gl_VertexIndex = OpVariable %_ptr_Input_int Input 
      %outPerVertex = OpVariable %_ptr_Output_gl_PerVertex Output
            ; %param0 = OpVariable %_ptr_Output_v4float Output            
               <Variables>
)";
	static const char32_t* compute_variables = UR"(
              ;Variables
%gl_LocalInvocationID = OpVariable %_ptr_Input_v3uint Input 
      %gl_WorkGroupID = OpVariable %_ptr_Input_v3uint Input           
               <Variables>
)";

	Core::StringList vars;

	if (m_bind != nullptr)
	{
		if (m_bind->storage_buffers.buffers_num > 0)
		{
			vars.Add(String::FromPrintf("%%buf = OpVariable %%_ptr_StorageBuffer__arr_BufferObject_uint_%d StorageBuffer",
			                            m_bind->storage_buffers.buffers_num));
		}
		if (m_bind->textures2D.textures_num > 0)
		{
			vars.Add(String::FromPrintf("%%textures2D = OpVariable %%_ptr_UniformConstant__arr_SampledImage_uint_%d UniformConstant",
			                            m_bind->textures2D.textures_num));
		}
		if (m_bind->samplers.samplers_num > 0)
		{
			vars.Add(String::FromPrintf("%%samplers = OpVariable %%_ptr_UniformConstant__arr_Sampler_uint_%d UniformConstant",
			                            m_bind->samplers.samplers_num));
		}
		if (m_bind->gds_pointers.pointers_num > 0)
		{
			vars.Add(U"%gds = OpVariable %_ptr_StorageBuffer_GDS StorageBuffer");
		}
		if (m_bind->push_constant_size > 0)
		{
			vars.Add(U"%vsharp = OpVariable %_ptr_PushConstant_BufferResource PushConstant");
		}
	}

	switch (m_code.GetType())
	{
		case ShaderType::Pixel:
			if (m_ps_input_info != nullptr)
			{
				for (uint32_t i = 0; i < m_ps_input_info->input_num; i++)
				{
					vars.Add(String::FromPrintf("%%attr%d = OpVariable %%_ptr_Input_v4float Input", i));
				}
				if (m_ps_input_info->ps_pos_xy)
				{
					vars.Add(U"%gl_FragCoord = OpVariable %_ptr_Input_v4float Input");
				}
			}
			m_source += String(pixel_variables).ReplaceStr(U"<Variables>", vars.Concat(U"\n" + String(U' ', 15)));
			break;
		case ShaderType::Vertex:
			if (m_vs_input_info != nullptr)
			{
				for (int i = 0; i < m_vs_input_info->resources_num; i++)
				{
					switch (m_vs_input_info->resources_dst[i].registers_num)
					{
						case 1: vars.Add(String::FromPrintf("%%attr%d = OpVariable %%_ptr_Input_float Input", i)); break;
						case 2: vars.Add(String::FromPrintf("%%attr%d = OpVariable %%_ptr_Input_v2float Input", i)); break;
						case 3: vars.Add(String::FromPrintf("%%attr%d = OpVariable %%_ptr_Input_v3float Input", i)); break;
						case 4: vars.Add(String::FromPrintf("%%attr%d = OpVariable %%_ptr_Input_v4float Input", i)); break;
						default: EXIT("invalid registers_num: %d\n", m_vs_input_info->resources_dst[i].registers_num);
					}
				}
				for (int i = 0; i < m_vs_input_info->export_count; i++)
				{
					vars.Add(String::FromPrintf("%%param%d = OpVariable %%_ptr_Output_v4float Output", i));
				}
			}
			m_source += String(vertex_variables).ReplaceStr(U"<Variables>", vars.Concat(U"\n" + String(U' ', 15)));
			break;
		case ShaderType::Compute:
			if (m_cs_input_info != nullptr)
			{
				vars.Add(String::FromPrintf("%%gl_WorkGroupSize = OpConstantComposite %%v3uint %%uint_%u %%uint_%u %%uint_%u",
				                            m_cs_input_info->threads_num[0], m_cs_input_info->threads_num[1],
				                            m_cs_input_info->threads_num[2]));
			}
			m_source += String(compute_variables).ReplaceStr(U"<Variables>", vars.Concat(U"\n" + String(U' ', 15)));
			break;
		default: EXIT("unknown type: %s\n", Core::EnumName(m_code.GetType()).C_Str()); break;
	}
}

void Spirv::WriteMainProlog()
{
	static const char32_t* text = UR"(
                   ; Function main
                   ; Prolog
       %main       = OpFunction %void None %function_void
       %main_label = OpLabel 
)";

	m_source += text;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Spirv::WriteLocalVariables()
{
	FindVariables();

	static const char32_t* comment = UR"(	
    ; Registers
)";

	m_source += comment;

	for (const auto& c: m_variables)
	{
		auto value = operand_variable_to_str(c.op);
		m_source += String::FromPrintf("%%%s = OpVariable %%_ptr_Function_%s Function\n", value.value.C_Str(),
		                               Core::EnumName(value.type).ToLower().C_Str());
	}

	static const char32_t* common_vars = UR"(
             %temp_float = OpVariable %_ptr_Function_float Function
           %temp_v2float = OpVariable %_ptr_Function_v2float Function
           %temp_v3float = OpVariable %_ptr_Function_v3float Function
	       %temp_v4float = OpVariable %_ptr_Function_v4float Function
           %temp_int_0 = OpVariable %_ptr_Function_int Function
           %temp_int_1 = OpVariable %_ptr_Function_int Function
           %temp_int_2 = OpVariable %_ptr_Function_int Function
           %temp_int_3 = OpVariable %_ptr_Function_int Function
           %temp_int_4 = OpVariable %_ptr_Function_int Function
           %temp_int_5 = OpVariable %_ptr_Function_int Function
           %temp_uint_0 = OpVariable %_ptr_Function_uint Function
           %temp_uint_1 = OpVariable %_ptr_Function_uint Function
           %temp_uint_2 = OpVariable %_ptr_Function_uint Function
           %temp_uint_3 = OpVariable %_ptr_Function_uint Function
           %temp_uint_4 = OpVariable %_ptr_Function_uint Function
           %temp_uint_5 = OpVariable %_ptr_Function_uint Function
)";

	m_source += common_vars;

	if (m_code.GetType() == ShaderType::Vertex)
	{
		static const char32_t* text = UR"(
       %vertex_index_int = OpLoad %int %gl_VertexIndex
           %vertex_index = OpBitcast %float %vertex_index_int
                           OpStore %v0 %vertex_index           
)";
		m_source += text;
	}

	if (m_code.GetType() == ShaderType::Pixel)
	{
		if (m_ps_input_info->ps_pos_xy)
		{
			static const char32_t* text = UR"(  
         %FragCoord_px = OpAccessChain %_ptr_Input_float %gl_FragCoord %uint_0
         %FragCoord_x = OpLoad %float %FragCoord_px
               OpStore %v2 %FragCoord_x
         %FragCoord_py = OpAccessChain %_ptr_Input_float %gl_FragCoord %uint_1
         %FragCoord_y = OpLoad %float %FragCoord_py
               OpStore %v3 %FragCoord_y     
)";
			m_source += text;
		}
	}

	if (m_code.GetType() == ShaderType::Compute)
	{
		static const char32_t* text = UR"(
		%LocalInvocationID_114 = OpAccessChain %_ptr_Input_uint %gl_LocalInvocationID %uint_0
        %LocalInvocationID_115 = OpLoad %uint %LocalInvocationID_114
        %LocalInvocationID_116 = OpBitcast %float %LocalInvocationID_115
               OpStore %v0 %LocalInvocationID_116		   
        %WorkGroupID_120 = OpAccessChain %_ptr_Input_uint %gl_WorkGroupID %uint_0
        %WorkGroupID_121 = OpLoad %uint %WorkGroupID_120
               OpStore %<WorkGroupReg> %WorkGroupID_121       
)";
		if (m_cs_input_info != nullptr)
		{
			m_source += String(text).ReplaceStr(U"<WorkGroupReg>", String::FromPrintf("s%u", m_cs_input_info->workgroup_register));
		}
	}

	if (m_bind != nullptr)
	{
		static const char32_t* text = UR"(
         %vsharp_<reg> = OpAccessChain %_ptr_PushConstant_uint %vsharp %int_0 %int_<buffer> %int_<field>
         %vsharp_value_<reg> = OpLoad %uint %vsharp_<reg>
               OpStore %<reg> %vsharp_value_<reg>
		)";

		int buffer_index = 0;

		for (auto& m: m_extended_mapping)
		{
			m[0] = m[1] = 0;
		}

		for (int i = 0; i < m_bind->storage_buffers.buffers_num; i++)
		{
			int  start_reg = m_bind->storage_buffers.start_register[i];
			bool extended  = m_bind->storage_buffers.extended[i];

			EXIT_IF(buffer_index + i >= static_cast<int>(m_bind->push_constant_size) / 16);

			String buffer = String::FromPrintf("%d", buffer_index + i);
			for (int f = 0; f < 4; f++)
			{
				if (extended)
				{
					EXIT_IF(start_reg < 16);
					EXIT_NOT_IMPLEMENTED(start_reg - 16 + f >= m_extended_mapping.Size());
					m_extended_mapping[start_reg - 16 + f][0] = buffer_index + i;
					m_extended_mapping[start_reg - 16 + f][1] = f;
				} else
				{
					String reg   = String::FromPrintf("s%d", start_reg + f);
					String field = String::FromPrintf("%d", f);
					m_source += String(text).ReplaceStr(U"<reg>", reg).ReplaceStr(U"<buffer>", buffer).ReplaceStr(U"<field>", field);
				}
			}
		}

		buffer_index += m_bind->storage_buffers.buffers_num;

		for (int i = 0; i < m_bind->textures2D.textures_num; i++)
		{
			int  start_reg = m_bind->textures2D.start_register[i];
			bool extended  = m_bind->textures2D.extended[i];

			for (int ti = 0; ti < 2; ti++)
			{
				EXIT_IF(buffer_index + i * 2 + ti >= static_cast<int>(m_bind->push_constant_size) / 16);

				String buffer = String::FromPrintf("%d", buffer_index + i * 2 + ti);
				for (int f = 0; f < 4; f++)
				{
					if (extended)
					{
						EXIT_IF(start_reg < 16);
						EXIT_NOT_IMPLEMENTED(start_reg - 16 + 4 * ti + f >= m_extended_mapping.Size());
						m_extended_mapping[start_reg - 16 + 4 * ti + f][0] = buffer_index + i * 2 + ti;
						m_extended_mapping[start_reg - 16 + 4 * ti + f][1] = f;
					} else
					{
						String reg   = String::FromPrintf("s%d", start_reg + 4 * ti + f);
						String field = String::FromPrintf("%d", f);
						m_source += String(text).ReplaceStr(U"<reg>", reg).ReplaceStr(U"<buffer>", buffer).ReplaceStr(U"<field>", field);
					}
				}
			}
		}

		buffer_index += m_bind->textures2D.textures_num * 2;

		for (int i = 0; i < m_bind->samplers.samplers_num; i++)
		{
			int  start_reg = m_bind->samplers.start_register[i];
			bool extended  = m_bind->samplers.extended[i];

			EXIT_IF(buffer_index + i >= static_cast<int>(m_bind->push_constant_size) / 16);

			String buffer = String::FromPrintf("%d", buffer_index + i);
			for (int f = 0; f < 4; f++)
			{
				if (extended)
				{
					EXIT_IF(start_reg < 16);
					EXIT_NOT_IMPLEMENTED(start_reg - 16 + f >= m_extended_mapping.Size());
					m_extended_mapping[start_reg - 16 + f][0] = buffer_index + i;
					m_extended_mapping[start_reg - 16 + f][1] = f;
				} else
				{
					String reg   = String::FromPrintf("s%d", start_reg + f);
					String field = String::FromPrintf("%d", f);
					m_source += String(text).ReplaceStr(U"<reg>", reg).ReplaceStr(U"<buffer>", buffer).ReplaceStr(U"<field>", field);
				}
			}
		}

		buffer_index += m_bind->samplers.samplers_num;

		for (int i = 0; i < m_bind->gds_pointers.pointers_num; i++)
		{
			int  start_reg = m_bind->gds_pointers.start_register[i];
			bool extended  = m_bind->gds_pointers.extended[i];

			EXIT_IF(buffer_index + i / 4 >= static_cast<int>(m_bind->push_constant_size) / 16);

			if (extended)
			{
				EXIT_IF(start_reg < 16);
				EXIT_NOT_IMPLEMENTED(start_reg - 16 >= m_extended_mapping.Size());
				m_extended_mapping[start_reg - 16][0] = buffer_index + i / 4;
				m_extended_mapping[start_reg - 16][1] = i % 4;
			} else
			{
				String buffer = String::FromPrintf("%d", buffer_index + i / 4);
				String reg    = String::FromPrintf("s%d", start_reg);
				String field  = String::FromPrintf("%d", i % 4);
				m_source += String(text).ReplaceStr(U"<reg>", reg).ReplaceStr(U"<buffer>", buffer).ReplaceStr(U"<field>", field);
			}
		}

		/* buffer_index += (m_bind->gds_pointers.pointers_num > 0 ? (m_bind->gds_pointers.pointers_num - 1) / 4 + 1 : 0); */

		if (m_bind->extended.used)
		{
			// TODO() load pointer

			printf("Extended mapping: ");
			for (auto& m: m_extended_mapping)
			{
				printf("{%d, %d} ", m[0], m[1]);
			}
			printf("\n");
		}
	}

	static const char32_t* common_init = UR"(
               OpStore %exec_lo %uint_1
               OpStore %exec_hi %uint_0
               OpStore %execz %uint_0
               OpStore %scc %uint_0
	)";

	m_source += common_init;
	m_source += U"\n";
}

void Spirv::WriteInstructions()
{
	int         index        = -1;
	const auto& instructions = m_code.GetInstructions();
	for (const auto& inst: instructions)
	{
		index++;

		const auto& labels = m_code.GetLabels();
		for (uint32_t i = labels.Size(); i > 0; i--)
		{
			auto label = labels.At(i - 1);
			if (index > 0 && label.dst == inst.pc)
			{
				static const char32_t* text = UR"(
                   <branch>
                   %<label> = OpLabel
		)";

				auto prev_type   = instructions.At(index - 1).type;
				bool skip_branch = (prev_type == ShaderInstructionType::SEndpgm);

				m_source += String(text)
				                .ReplaceStr(U"<branch>", (skip_branch ? U"" : U"OpBranch %<label>"))
				                .ReplaceStr(U"<label>", String::FromPrintf("label_%04" PRIx32 "_%04" PRIx32, label.dst, label.src));
			}
		}

		//		if ((index == 0 && inst.type == ShaderInstructionType::SMovB32 && inst.format == ShaderInstructionFormat::SVdstSVsrc0 &&
		//		     inst.dst.type == ShaderOperandType::VccHi) ||
		//		    (inst.type == ShaderInstructionType::SEndpgm && index == instructions.Size() - 1))
		//		{
		//			continue;
		//		}

		String src = ShaderCode::DbgInstructionToStr(inst);

		String dst;

		bool ok = false;
		for (auto& func: g_recomp_func)
		{
			if (func.type == inst.type && func.format == inst.format)
			{
				ok = func.func(index, m_code, &dst, this, func.param, func.scc_check);
				break;
			}
		}

		if (!ok)
		{
			printf("%s\n", m_source.C_Str());
			EXIT("Can't recompile: %s\n", src.C_Str());
		}

		m_source += String::FromPrintf("; %s\n", src.C_Str());
		m_source += String::FromPrintf("%s\n", dst.C_Str());
	}
}

void Spirv::WriteMainEpilog()
{
	static const char32_t* text = UR"(
                   ; Epilog
                   OpFunctionEnd
)";

	m_source += text;
}

void Spirv::WriteFunctions()
{
	if (m_code.HasAnyOf({ShaderInstructionType::VSadU32}))
	{
		m_source += FUNC_ABS_DIFF;
	}

	if (m_code.HasAnyOf({ShaderInstructionType::VMulLoI32, ShaderInstructionType::VMadU32U24, ShaderInstructionType::VMulU32U24}))
	{
		m_source += FUNC_MUL_EXTENDED;
	}

	if (m_code.HasAnyOf({ShaderInstructionType::SSwappcB64}))
	{
		m_source += FUNC_FETCH_1;
		m_source += FUNC_FETCH_2;
		m_source += FUNC_FETCH_3;
		m_source += FUNC_FETCH_4;
	}

	if (m_code.HasAnyOf({ShaderInstructionType::BufferLoadDword, ShaderInstructionType::BufferLoadFormatX,
	                     ShaderInstructionType::BufferLoadFormatXy, ShaderInstructionType::BufferLoadFormatXyz,
	                     ShaderInstructionType::BufferLoadFormatXyzw, ShaderInstructionType::TBufferLoadFormatXyzw}))
	{
		m_source += BUFFER_LOAD_FLOAT1;
		m_source += BUFFER_LOAD_FLOAT4;
		m_source += TBUFFER_LOAD_FORMAT_X;
		m_source += TBUFFER_LOAD_FORMAT_XYZW;
	}

	if (m_code.HasAnyOf({ShaderInstructionType::BufferStoreDword, ShaderInstructionType::BufferStoreFormatX}))
	{
		m_source += BUFFER_STORE_FLOAT1;
		m_source += TBUFFER_STORE_FORMAT_X;
	}

	if (m_code.HasAnyOf({ShaderInstructionType::SBufferLoadDword, ShaderInstructionType::SBufferLoadDwordx2,
	                     ShaderInstructionType::SBufferLoadDwordx4, ShaderInstructionType::SBufferLoadDwordx8,
	                     ShaderInstructionType::SBufferLoadDwordx16}))
	{
		m_source += SBUFFER_LOAD_DWORD;
		m_source += SBUFFER_LOAD_DWORD_2;
		m_source += SBUFFER_LOAD_DWORD_4;
		m_source += SBUFFER_LOAD_DWORD_8;
		m_source += SBUFFER_LOAD_DWORD_16;
	}
}

void Spirv::FindConstants()
{
	m_constants.Clear();
	AddConstantFloat(0.0f);
	AddConstantFloat(0.5f);
	AddConstantFloat(1.0f);
	AddConstantFloat(2.0f);
	AddConstantFloat(4.0f);
	for (int i = 0; i <= 16; i++)
	{
		AddConstantInt(i);
		AddConstantUint(i);
	}
	for (const auto& inst: m_code.GetInstructions())
	{
		for (int i = 0; i < inst.src_num; i++)
		{
			if (operand_is_constant(inst.src[i]))
			{
				AddConstant(inst.src[i]);
			}
		}
	}
	if (m_vs_input_info != nullptr || m_ps_input_info != nullptr || m_cs_input_info != nullptr)
	{
		AddConstantInt(12);
		AddConstantInt(16);
		AddConstantInt(31);
		AddConstantInt(36);
		AddConstantInt(119);
		AddConstantUint(24);
		AddConstantUint(31);
		AddConstantUint(72);
		AddConstantUint(127);
		AddConstantUint(0x3fff);
		AddConstantUint(0xffffff);
	}
	if (m_cs_input_info != nullptr)
	{
		AddConstantUint(m_cs_input_info->threads_num[0]);
		AddConstantUint(m_cs_input_info->threads_num[1]);
		AddConstantUint(m_cs_input_info->threads_num[2]);
	}
}

void Spirv::FindVariables()
{
	m_variables.Clear();

	AddVariable(ShaderOperandType::Vgpr, 0, 1);
	AddVariable(ShaderOperandType::ExecLo, 0, 2);
	AddVariable(ShaderOperandType::ExecZ, 0, 1);
	AddVariable(ShaderOperandType::Scc, 0, 1);

	for (const auto& inst: m_code.GetInstructions())
	{
		AddVariable(inst.dst);
		AddVariable(inst.dst2);
		for (int i = 0; i < inst.src_num; i++)
		{
			AddVariable(inst.src[i]);
		}
	}

	if (m_vs_input_info != nullptr)
	{
		for (int i = 0; i < m_vs_input_info->resources_num; i++)
		{
			AddVariable(ShaderOperandType::Vgpr, m_vs_input_info->resources_dst[i].register_start,
			            m_vs_input_info->resources_dst[i].registers_num);
		}
	}

	if (m_ps_input_info != nullptr)
	{
		if (m_ps_input_info->ps_pos_xy)
		{
			AddVariable(ShaderOperandType::Vgpr, 2, 1);
			AddVariable(ShaderOperandType::Vgpr, 3, 1);
		}
	}

	if (m_cs_input_info != nullptr)
	{
		AddVariable(ShaderOperandType::Sgpr, m_cs_input_info->workgroup_register, 1);
	}

	if (m_bind != nullptr)
	{
		for (int i = 0; i < m_bind->storage_buffers.buffers_num; i++)
		{
			int storage_start = m_bind->storage_buffers.start_register[i];
			AddVariable(ShaderOperandType::Sgpr, storage_start, 4);
		}
		for (int i = 0; i < m_bind->textures2D.textures_num; i++)
		{
			int storage_start = m_bind->textures2D.start_register[i];
			AddVariable(ShaderOperandType::Sgpr, storage_start, 8);
		}
		for (int i = 0; i < m_bind->samplers.samplers_num; i++)
		{
			int storage_start = m_bind->samplers.start_register[i];
			AddVariable(ShaderOperandType::Sgpr, storage_start, 8);
		}
	}
}

String SpirvGenerateSource(const ShaderCode& code, const ShaderVertexInputInfo* vs_input_info, const ShaderPixelInputInfo* ps_input_info,
                           const ShaderComputeInputInfo* cs_input_info)
{
	Spirv spirv;
	spirv.SetCode(code);
	spirv.SetVsInputInfo(vs_input_info);
	spirv.SetPsInputInfo(ps_input_info);
	spirv.SetCsInputInfo(cs_input_info);
	spirv.GenerateSource();

	return spirv.GetSource();
}

String SpirvGetEmbeddedVs(uint32_t id)
{
	EXIT_NOT_IMPLEMENTED(id != 0);

	return EMBEDDED_SHADER_VS_0;
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
