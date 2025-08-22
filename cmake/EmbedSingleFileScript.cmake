# cmake/EmbedSingleFileScript.cmake
# args: -DINPUT=... -DOUTPUT=... -DSYMBOL=...

if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED SYMBOL)
  message(FATAL_ERROR "EmbedSingleFileScript: need -DINPUT, -DOUTPUT, -DSYMBOL")
endif()

# Read file as hex nybbles (portable, no xxd dependency needed)
file(READ "${INPUT}" _HEX HEX)
string(LENGTH "${_HEX}" _N)
math(EXPR _BYTES "${_N} / 2")

# Header guard from output file name
get_filename_component(_BASE "${OUTPUT}" NAME_WE)
string(REGEX REPLACE "[^A-Za-z0-9]" "_" _GUARD "${_BASE}_H_")

# Write header
file(WRITE "${OUTPUT}" "/* Auto-generated from ${INPUT}. Do not edit. */\n")
file(APPEND "${OUTPUT}" "#ifndef ${_GUARD}\n#define ${_GUARD}\n\n")
file(APPEND "${OUTPUT}" "#include <stddef.h>\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
file(APPEND "${OUTPUT}" "extern const unsigned char ${SYMBOL}[];\n")
file(APPEND "${OUTPUT}" "extern const size_t ${SYMBOL}_len;\n\n")
file(APPEND "${OUTPUT}" "#ifdef __cplusplus\n}\n#endif\n\n")

# Also emit a private static definition section guarded by a macro so you can
# include the header in exactly one C file to define the data:
file(APPEND "${OUTPUT}" "#ifdef ${SYMBOL}_DEFINE\n")
file(APPEND "${OUTPUT}" "const unsigned char ${SYMBOL}[] = {\n")

# Stream bytes with a reasonable wrap
set(_i 0)
set(_col 0)
while(_i LESS _N)
  string(SUBSTRING "${_HEX}" ${_i} 2 _B)
  if(_col EQUAL 0)
    file(APPEND "${OUTPUT}" "  ")
  endif()
  file(APPEND "${OUTPUT}" "0x${_B},")
  math(EXPR _i   "${_i}+2")
  math(EXPR _col "(${_col}+1) % 16")
  if(_col EQUAL 0)
    file(APPEND "${OUTPUT}" "\n")
  endif()
endwhile()
if(NOT _col EQUAL 0)
  file(APPEND "${OUTPUT}" "\n")
endif()

file(APPEND "${OUTPUT}" "};\n")
file(APPEND "${OUTPUT}" "const size_t ${SYMBOL}_len = ${_BYTES};\n")
file(APPEND "${OUTPUT}" "#endif /* ${SYMBOL}_DEFINE */\n\n")
file(APPEND "${OUTPUT}" "#endif /* ${_GUARD} */\n")
