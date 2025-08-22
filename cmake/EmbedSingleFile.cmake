# cmake/EmbedSingleFile.cmake
#
# embed_single_file(<input_file> <output_header> <symbol_name> <out_target_var>)
# - <input_file>    : path to the source file (relative to source dir or absolute)
# - <output_header> : path to the generated .h (relative or absolute)
# - <symbol_name>   : e.g. gPrelude (array will be `const unsigned char gPrelude[]`)
# - <out_target_var>: (OUT) returns the custom target name to depend on
#
# The header will also export: `extern const size_t <symbol>_len;`

function(embed_file INPUT_FILE OUTPUT_HEADER SYMBOL_NAME OUT_TARGET_VAR)
  # Resolve paths
  if(NOT IS_ABSOLUTE "${INPUT_FILE}")
    set(_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}")
  else()
    set(_INPUT "${INPUT_FILE}")
  endif()

  if(NOT IS_ABSOLUTE "${OUTPUT_HEADER}")
    set(_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_HEADER}")
  else()
    set(_OUTPUT "${OUTPUT_HEADER}")
  endif()

  get_filename_component(_OUTDIR "${_OUTPUT}" DIRECTORY)
  file(MAKE_DIRECTORY "${_OUTDIR}")

  # Where the generator script lives (next file below)
  get_filename_component(_THIS_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
  set(_SCRIPT "${_THIS_DIR}/cmake/EmbedSingleFileScript.cmake")
  if(NOT EXISTS "${_SCRIPT}")
    message(FATAL_ERROR "EmbedSingleFile: generator script not found at: ${_SCRIPT}")
  endif()

  # Make a stable target name from the symbol
  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _SYM "${SYMBOL_NAME}")
  set(_TARGET "embed_${_SYM}")

  add_custom_command(
    OUTPUT "${_OUTPUT}"
    COMMAND ${CMAKE_COMMAND}
            -DINPUT=${_INPUT}
            -DOUTPUT=${_OUTPUT}
            -DSYMBOL=${_SYM}
            -P ${_SCRIPT}
    DEPENDS "${_INPUT}" "${_SCRIPT}"
    COMMENT "Embedding ${_INPUT} -> ${_OUTPUT} (${_SYM})"
    VERBATIM
  )

  add_custom_target("${_TARGET}" ALL DEPENDS "${_OUTPUT}")

  # Return the target name to caller
  set(${OUT_TARGET_VAR} "${_TARGET}" PARENT_SCOPE)
endfunction()