#C stuff
cmake_minimum_required(VERSION 3.16)
project(kui LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Embed files from kui/runtime into the library (configure-time) ---------
find_program(XXD xxd)  # optional (uses fallback if missing)

file(GLOB_RECURSE KUI_RUNTIME_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/kui/runtime/*.*")

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/kui_embed")
file(MAKE_DIRECTORY "${GEN_DIR}")

function(make_sym_from_rel REL OUTVAR)
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" _sym "${REL}")
  set(${OUTVAR} "${_sym}" PARENT_SCOPE)
endfunction()

set(_INCLUDES "")
set(_TABLE "")

foreach(F IN LISTS KUI_RUNTIME_FILES)
  file(RELATIVE_PATH REL "${CMAKE_CURRENT_SOURCE_DIR}/kui/runtime" "${F}")
  make_sym_from_rel("${REL}" SYM)
  set(OUT_INC "${GEN_DIR}/${SYM}.inc")

  if(XXD)
    execute_process(COMMAND "${XXD}" -i "${F}"
                    OUTPUT_FILE "${OUT_INC}"
                    RESULT_VARIABLE _rc)
    if(NOT _rc EQUAL 0)
      message(FATAL_ERROR "xxd failed for ${F} (${_rc})")
    endif()
  else()
    file(READ "${F}" HEXDATA HEX)
    string(LENGTH "${HEXDATA}" N)
    math(EXPR BYTES "${N} / 2")
    set(CONTENT "/* ${REL} */\nunsigned char ${SYM}[] = {")
    set(i 0)
    while(i LESS N)
      string(SUBSTRING "${HEXDATA}" ${i} 2 B)
      string(APPEND CONTENT "0x${B},")
      math(EXPR i "${i}+2")
    endwhile()
    string(APPEND CONTENT "};\n")
    file(WRITE "${OUT_INC}" "${CONTENT}")
  endif()

  list(APPEND _INCLUDES "#include \"${OUT_INC}\"\n")

  get_filename_component(EXT "${F}" EXT)
  string(TOLOWER "${EXT}" EXT)
  if(EXT MATCHES "\\.(html|css|js)$")
    set(KIND "KUI_RESOURCE_TEXT")
  else()
    set(KIND "KUI_RESOURCE_BINARY")
  endif()

  list(APPEND _TABLE "  { \"${REL}\", (const unsigned char*)&${SYM}[0], sizeof(${SYM}), ${KIND} },\n")
endforeach()

set(KUI_RESOURCES_C "${GEN_DIR}/kui_resources.c")
file(WRITE  "${KUI_RESOURCES_C}" "#include <stddef.h>\n#include \"kui/resource.h\"\n")
foreach(L IN LISTS _INCLUDES) 
  file(APPEND "${KUI_RESOURCES_C}" "${L}") 
endforeach()
  file(APPEND "${KUI_RESOURCES_C}" "const KuiResource gKuiResources[] = {\n")
foreach(R IN LISTS _TABLE)     
  file(APPEND "${KUI_RESOURCES_C}" "${R}") 
endforeach()
file(APPEND "${KUI_RESOURCES_C}" "};\nconst size_t gKuiResourceCount = sizeof(gKuiResources)/sizeof(gKuiResources[0]);\n")



# Fetch webview
include(FetchContent)

FetchContent_Declare(
    webview
    GIT_REPOSITORY https://github.com/webview/webview
    GIT_TAG 0.12.0)
FetchContent_MakeAvailable(webview)


# Put built stuff in neat folders: build/bin and build/lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  # .exe, .dll
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  # .so, .dylib (and .dll on Win if shared)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
  string(TOUPPER "${cfg}" CFGU)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFGU} ${CMAKE_BINARY_DIR}/bin/${cfg})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFGU} ${CMAKE_BINARY_DIR}/bin/${cfg})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFGU} ${CMAKE_BINARY_DIR}/lib/${cfg})
endforeach()

# cJSON
add_library(cjson STATIC kui/third_party/cjson/cJSON.c)
target_include_directories(cjson PUBLIC third_party/cjson)

# KUI
file(GLOB KUI_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/kui/src/*.c")

add_library(kui SHARED ${KUI_SRC})
target_include_directories(kui PUBLIC kui/include/)

target_link_libraries(kui PRIVATE webview::core_static)
if(WIN32)
  target_compile_definitions(kui PRIVATE WEBVIEW_EDGE=1)   # use WebView2
  target_link_libraries(kui PRIVATE user32 ole32 shell32 advapi32)
endif()

target_sources(kui PRIVATE "${KUI_RESOURCES_C}")
target_include_directories(kui PRIVATE "${GEN_DIR}")
target_include_directories(kui PRIVATE kui/third_party)

target_link_libraries(kui PRIVATE cjson)

target_compile_definitions(kui PRIVATE KUI_BUILD=1)

# Demo
file(GLOB DEMO_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/demo/src/*.c")

add_executable(kui_demo WIN32 ${DEMO_SRC})
target_link_libraries(kui_demo PRIVATE kui)
target_include_directories(kui_demo PRIVATE kui/include/)