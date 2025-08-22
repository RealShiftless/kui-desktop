# cmake/EmbedResources.cmake
# Generates a C source that embeds all files under <input_dir> into arrays,
# plus a lookup table (gKuiResources / gKuiResourceCount).
#
# Usage:
#   embed_resources(<input_dir> <output_subdir> <out_c_var> <out_incdir_var>)
#
# - <input_dir>      : path relative to ${CMAKE_CURRENT_SOURCE_DIR}
# - <output_subdir>  : subdir under ${CMAKE_CURRENT_BINARY_DIR}/gen where files go
# - <out_c_var>      : (OUT) var name to receive generated .c file path
# - <out_incdir_var> : (OUT) var name to receive generated include dir

function(embed_resources INPUT_DIR OUTPUT_SUBDIR OUT_C_VAR OUT_INCDIR_VAR)
  if(NOT IS_ABSOLUTE "${INPUT_DIR}")
    set(_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_DIR}")
  else()
    set(_INPUT_DIR "${INPUT_DIR}")
  endif()

  set(_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen/${OUTPUT_SUBDIR}")
  file(MAKE_DIRECTORY "${_GEN_DIR}")

  # ABSOLUTE deps so Ninja can find them
  file(GLOB_RECURSE _FILES_ABS "${_INPUT_DIR}/*.*")

  set(_OUT_C  "${_GEN_DIR}/resources.c")
  set(_STAMP  "${_GEN_DIR}/.stamp")
  # NOTE: use the actual filename you created (Resource**s**Script.cmake)
  set(_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedResourcesScript.cmake")

  # only pass optional args if set
  set(_table_args)
  if(DEFINED ER_TABLE AND NOT ER_TABLE STREQUAL "")
    list(APPEND _table_args -DTABLE_NAME=${ER_TABLE})
  endif()
  if(DEFINED ER_COUNT AND NOT ER_COUNT STREQUAL "")
    list(APPEND _table_args -DCOUNT_NAME=${ER_COUNT})
  endif()
  if(DEFINED ER_HEADER AND NOT ER_HEADER STREQUAL "")
    list(APPEND _table_args -DHEADER_PATH=${ER_HEADER})
  endif()

  add_custom_command(
    OUTPUT  ${_OUT_C} ${_STAMP}
    COMMAND ${CMAKE_COMMAND}
            -DINPUT_DIR=${_INPUT_DIR}
            -DGEN_DIR=${_GEN_DIR}
            -DOUT_C=${_OUT_C}
            ${_table_args}
            -P ${_SCRIPT}
    DEPENDS ${_FILES_ABS} ${_SCRIPT}
    COMMENT "Embedding resources: ${INPUT_DIR} -> ${_OUT_C}"
    VERBATIM
  )

  add_custom_target(${OUTPUT_SUBDIR}_resources ALL
    DEPENDS ${_OUT_C} ${_STAMP}
  )

  set(${OUT_C_VAR}      "${_OUT_C}"   PARENT_SCOPE)
  set(${OUT_INCDIR_VAR} "${_GEN_DIR}" PARENT_SCOPE)
endfunction()

