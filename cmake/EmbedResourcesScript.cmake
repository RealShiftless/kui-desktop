# cmake/EmbedResourcesScript.cmake
# Invoked with -DINPUT_DIR=... -DGEN_DIR=... -DOUT_C=... -DTABLE_NAME=... -DCOUNT_NAME=... -DHEADER_PATH=...

if(NOT DEFINED INPUT_DIR OR NOT DEFINED GEN_DIR OR NOT DEFINED OUT_C)
  message(FATAL_ERROR "EmbedResourcesScript: missing required -DINPUT_DIR, -DGEN_DIR, or -DOUT_C")
endif()

if(NOT DEFINED TABLE_NAME)
  set(TABLE_NAME "gKuiResources")
endif()
if(NOT DEFINED COUNT_NAME)
  set(COUNT_NAME "gKuiResourceCount")
endif()
if(NOT DEFINED HEADER_PATH)
  set(HEADER_PATH "kui/resource.h")
endif()

# Ensure output dir exists
file(MAKE_DIRECTORY "${GEN_DIR}")

# Find xxd and detect -n support once
if(NOT DEFINED __ERS_XXD_PATH)
  find_program(__ERS_XXD_PATH xxd)
endif()

if(NOT DEFINED __ERS_XXD_SUPPORTS_N)
  set(__ERS_XXD_SUPPORTS_N OFF)
  if(__ERS_XXD_PATH)
    execute_process(COMMAND "${__ERS_XXD_PATH}" -h
                    OUTPUT_VARIABLE _XXD_HELP OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
    if(_XXD_HELP MATCHES "\n\\s*-n\\s")
      set(__ERS_XXD_SUPPORTS_N ON)
    endif()
  endif()
endif()

# Re-glob inputs (safe even though DEPENDS is handled by the caller)
file(GLOB_RECURSE _FILES "${INPUT_DIR}/*.*")

# Symbol mangler (from relative path to valid C identifier)
function(_ers_make_sym_from_rel REL OUTVAR)
  string(TOLOWER "${REL}" _rel)
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" _sym "${_rel}")
  if(_sym MATCHES "^[0-9]")
    set(_sym "r_${_sym}")
  endif()
  set(${OUTVAR} "${_sym}" PARENT_SCOPE)
endfunction()

set(_INCLUDES "")
set(_TABLE    "")

foreach(_ABS IN LISTS _FILES)
  # RELATIVE path (e.g., "js/prelude.js")
  file(RELATIVE_PATH _REL "${INPUT_DIR}" "${_ABS}")
  _ers_make_sym_from_rel("${_REL}" _SYM)

  set(_OUT_INC_NAME "${_SYM}.inc")
  set(_OUT_INC      "${GEN_DIR}/${_OUT_INC_NAME}")

  if(__ERS_XXD_PATH AND __ERS_XXD_SUPPORTS_N)
    execute_process(
      COMMAND "${__ERS_XXD_PATH}" -i -n "${_SYM}" "${_ABS}"
      OUTPUT_FILE "${_OUT_INC}"
      RESULT_VARIABLE _rc
    )
    if(NOT _rc EQUAL 0)
      message(FATAL_ERROR "xxd failed for ${_ABS} (${_rc})")
    endif()
  else()
    # Portable fallback: emit bytes as a C array
    file(READ "${_ABS}" _HEX HEX)
    string(LENGTH "${_HEX}" _N)
    set(_CONTENT "/* ${_REL} */\nunsigned char ${_SYM}[] = {")
    set(_i 0)
    while(_i LESS _N)
      string(SUBSTRING "${_HEX}" ${_i} 2 _B)
      string(APPEND _CONTENT "0x${_B},")
      math(EXPR _i "${_i}+2")
    endwhile()
    string(APPEND _CONTENT "};\n")
    file(WRITE "${_OUT_INC}" "${_CONTENT}")
  endif()

  # Build include line (relative to GEN_DIR, since we'll add it to include dirs)
  list(APPEND _INCLUDES "#include \"${_OUT_INC_NAME}\"\n")

  # Decide resource kind by extension
  get_filename_component(_EXT "${_ABS}" EXT)
  string(TOLOWER "${_EXT}" _EXT)
  if(_EXT MATCHES "\\.(html|css|js)$")
    set(_KIND "KUI_RESOURCE_TEXT")
  else()
    set(_KIND "KUI_RESOURCE_BINARY")
  endif()

  # Add row
  list(APPEND _TABLE
    "  { \"${_REL}\", (const unsigned char*)&${_SYM}[0], sizeof(${_SYM}), ${_KIND} },\n")
endforeach()

# Write the final resources.c
file(WRITE  "${OUT_C}" "#include <stddef.h>\n#include \"${HEADER_PATH}\"\n")
foreach(_L IN LISTS _INCLUDES)
  file(APPEND "${OUT_C}" "${_L}")
endforeach()
file(APPEND "${OUT_C}" "const KuiResource ${TABLE_NAME}[] = {\n")
foreach(_R IN LISTS _TABLE)
  file(APPEND "${OUT_C}" "${_R}")
endforeach()
file(APPEND "${OUT_C}" "};\nconst size_t ${COUNT_NAME} = sizeof(${TABLE_NAME})/sizeof(${TABLE_NAME}[0]);\n")

# Touch a stamp so the custom target has a second output
file(WRITE "${GEN_DIR}/.stamp" "ok\n")