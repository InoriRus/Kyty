add_library(vulkan-1 SHARED IMPORTED GLOBAL)

if (MINGW AND NOT CLANG)
	set_target_properties(vulkan-1 PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan-1.lib")
elseif(MINGW AND CLANG)
	set_target_properties(vulkan-1 PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan-1.lib")
elseif(MSVC AND CLANG)
	if(CMAKE_BUILD_TYPE MATCHES Debug)
		set_target_properties(vulkan-1 PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan-1.lib")
	else()
		set_target_properties(vulkan-1 PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/vulkan-1.lib")
	endif()	
elseif(LINUX)
	find_library(Vulkan_Lib vulkan)
    set_target_properties(vulkan-1 PROPERTIES IMPORTED_LOCATION ${Vulkan_Lib})
endif()

set(SPIRV_SOURCES

  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/bitutils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/bit_vector.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/hex_float.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/make_unique.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/parse_number.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/small_vector.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/string_utils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/timer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/assembly_grammar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/binary.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/cfa.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/common_debug_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/diagnostic.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/disassemble.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/enum_set.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/enum_string_mapping.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ext_inst.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/extensions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/instruction.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/latest_version_glsl_std_450_header.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/latest_version_opencl_std_header.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/latest_version_spirv_header.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/macro.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/name_mapper.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opcode.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/operand.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/parsed_operand.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/print.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_constant.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_definition.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_endian.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_fuzzer_options.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_optimizer_options.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_reducer_options.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_target_env.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_validator_options.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/table.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/text.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/text_handler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate.h
							  
  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/bit_vector.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/parse_number.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/util/string_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/assembly_grammar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/binary.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/diagnostic.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/disassemble.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/enum_string_mapping.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/ext_inst.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/extensions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/libspirv.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/name_mapper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opcode.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/operand.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/parsed_operand.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/print.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/software_version.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_endian.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_fuzzer_options.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_optimizer_options.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_reducer_options.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_target_env.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/spirv_validator_options.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/text.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/text_handler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_adjacency.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_annotation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_arithmetics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_atomics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_barriers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_bitwise.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_builtins.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_capability.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_cfg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_composites.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_constants.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_conversion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_debug.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_decorations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_derivatives.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_extensions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_execution_limitations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_function.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_id.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_image.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_interfaces.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_instruction.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_literals.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_logicals.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_memory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_memory_semantics.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_misc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_mode_setting.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_non_uniform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_primitives.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_scopes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_small_type_uses.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validate_type.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/decoration.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/basic_block.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/construct.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/function.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/instruction.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/val/validation_state.cpp
)
  
set(SPIRV_TOOLS_OPT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/aggressive_dead_code_elim_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/amd_ext_to_khr.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/basic_block.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/block_merge_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/block_merge_util.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/build_module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/ccp_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/cfg_cleanup_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/cfg.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/code_sink.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/combine_access_chains.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/compact_ids_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/composite.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/const_folding_rules.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/constants.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/control_dependence.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/convert_to_half_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/copy_prop_arrays.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/dead_branch_elim_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/dead_insert_elim_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/dead_variable_elimination.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/decoration_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/debug_info_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/def_use_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/desc_sroa.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/dominator_analysis.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/dominator_tree.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/eliminate_dead_constant_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/eliminate_dead_functions_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/eliminate_dead_functions_util.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/eliminate_dead_members_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/empty_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/feature_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/fix_storage_class.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/flatten_decoration_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/fold.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/folding_rules.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/fold_spec_constant_op_and_composite_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/freeze_spec_constant_value_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/function.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/graphics_robust_access_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/if_conversion.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inline_exhaustive_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inline_opaque_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inline_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inst_bindless_check_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inst_buff_addr_check_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inst_debug_printf_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/instruction.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/instruction_list.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/instrument_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/interp_fixup_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/ir_builder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/ir_context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/ir_loader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/licm_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/local_access_chain_convert_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/local_redundancy_elimination.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/local_single_block_elim_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/local_single_store_elim_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/log.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_dependence.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_descriptor.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_fission.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_fusion.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_fusion_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_peeling.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_unroller.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_utils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_unswitch_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/mem_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/merge_return_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/null_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/passes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/pass_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/private_to_local_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/propagator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/reduce_load_size.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/redundancy_elimination.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/reflect.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/register_pressure.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/relax_float_ops_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/remove_duplicates_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/remove_unused_interface_variables_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/replace_invalid_opc.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/scalar_analysis.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/scalar_analysis_nodes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/scalar_replacement_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/set_spec_constant_default_value_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/simplification_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/ssa_rewrite_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/strength_reduction_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/strip_debug_info_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/strip_reflect_info_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/struct_cfg_analysis.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/tree_iterator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/type_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/unify_const_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/upgrade_memory_model.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/value_number_table.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/vector_dce.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/workaround1209.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/wrap_opkill.h

  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/aggressive_dead_code_elim_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/amd_ext_to_khr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/basic_block.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/block_merge_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/block_merge_util.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/build_module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/ccp_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/cfg_cleanup_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/cfg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/code_sink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/combine_access_chains.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/compact_ids_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/composite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/const_folding_rules.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/constants.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/control_dependence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/convert_to_half_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/copy_prop_arrays.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/dead_branch_elim_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/dead_insert_elim_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/dead_variable_elimination.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/decoration_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/debug_info_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/def_use_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/desc_sroa.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/dominator_analysis.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/dominator_tree.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/eliminate_dead_constant_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/eliminate_dead_functions_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/eliminate_dead_functions_util.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/eliminate_dead_members_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/feature_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/fix_storage_class.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/flatten_decoration_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/fold.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/folding_rules.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/fold_spec_constant_op_and_composite_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/freeze_spec_constant_value_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/function.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/graphics_robust_access_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/if_conversion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inline_exhaustive_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inline_opaque_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inline_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inst_bindless_check_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inst_buff_addr_check_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/inst_debug_printf_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/instruction.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/instruction_list.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/instrument_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/interp_fixup_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/ir_context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/ir_loader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/licm_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/local_access_chain_convert_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/local_redundancy_elimination.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/local_single_block_elim_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/local_single_store_elim_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_dependence.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_dependence_helpers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_descriptor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_fission.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_fusion.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_fusion_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_peeling.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_unroller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/loop_unswitch_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/mem_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/merge_return_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/optimizer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/pass_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/private_to_local_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/propagator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/reduce_load_size.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/redundancy_elimination.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/register_pressure.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/relax_float_ops_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/remove_duplicates_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/remove_unused_interface_variables_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/replace_invalid_opc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/scalar_analysis.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/scalar_analysis_simplification.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/scalar_replacement_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/set_spec_constant_default_value_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/simplification_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/ssa_rewrite_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/strength_reduction_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/strip_debug_info_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/strip_reflect_info_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/struct_cfg_analysis.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/type_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/types.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/unify_const_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/upgrade_memory_model.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/value_number_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/vector_dce.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/workaround1209.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/opt/wrap_opkill.cpp
)

if (LINUX)
    set_source_files_properties(${SPIRV_SOURCES} PROPERTIES COMPILE_FLAGS "-Os -DSPIRV_LINUX")
    set_source_files_properties(${SPIRV_TOOLS_OPT_SOURCES} PROPERTIES COMPILE_FLAGS "-Os -DSPIRV_LINUX")
elseif (MSVC)
    set_source_files_properties(${SPIRV_SOURCES} PROPERTIES COMPILE_FLAGS "/Os -D_CRT_SECURE_NO_WARNINGS -DSPIRV_WINDOWS")
    set_source_files_properties(${SPIRV_TOOLS_OPT_SOURCES} PROPERTIES COMPILE_FLAGS "/Os -D_CRT_SECURE_NO_WARNINGS -DSPIRV_WINDOWS")
else()
    set_source_files_properties(${SPIRV_SOURCES} PROPERTIES COMPILE_FLAGS "-Os -DSPIRV_WINDOWS")
    set_source_files_properties(${SPIRV_TOOLS_OPT_SOURCES} PROPERTIES COMPILE_FLAGS "-Os -DSPIRV_WINDOWS")
endif()


add_library(spirv-tools_obj OBJECT ${SPIRV_SOURCES})
add_library(spirv-tools STATIC $<TARGET_OBJECTS:spirv-tools_obj>)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/gen)

target_include_directories(spirv-tools PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

get_property(dirs TARGET spirv-tools PROPERTY INCLUDE_DIRECTORIES)
target_include_directories(spirv-tools_obj PRIVATE ${dirs})

add_library(spirv-tools-opt_obj OBJECT ${SPIRV_TOOLS_OPT_SOURCES})
add_library(spirv-tools-opt STATIC $<TARGET_OBJECTS:spirv-tools-opt_obj>)

target_include_directories(spirv-tools-opt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

get_property(dirs-opt TARGET spirv-tools-opt PROPERTY INCLUDE_DIRECTORIES)
target_include_directories(spirv-tools-opt_obj PRIVATE ${dirs-opt})


