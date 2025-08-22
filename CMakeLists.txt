# ---- Settings ----
set(APP_NAME "kui-desktop demo")

# ---- Project ----
cmake_minimum_required(VERSION 3.16)
project(kui LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Build output layout ----
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
if(CMAKE_CONFIGURATION_TYPES) # multi-config generators
  foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${cfg}" CFGU)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFGU} ${CMAKE_BINARY_DIR}/bin/${cfg})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFGU} ${CMAKE_BINARY_DIR}/bin/${cfg})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFGU} ${CMAKE_BINARY_DIR}/lib/${cfg})
  endforeach()
endif()

# ---- Generators (build-time) ----
include(cmake/EmbedSingleFile.cmake)
include(cmake/EmbedResources.cmake)

# 1) Single file -> header for the internal prelude
#    Generates: ${CMAKE_BINARY_DIR}/gen/internal/prelude.h
#    Declares:  extern const unsigned char gPrelude[]; extern const size_t gPrelude_len;
embed_file(
  "kui/js/prelude.js"
  "gen/internal/prelude.h"
  gPrelude
  PRELUDE_GEN_TARGET
)

# 2) App runtime -> resources.c compiled into the library
embed_resources("runtime" "runtime" KUI_RESOURCES_C KUI_GEN_DIR)

# ---- WebView dependency ----
include(FetchContent)
FetchContent_Declare(
  webview
  GIT_REPOSITORY https://github.com/webview/webview
  GIT_TAG 0.12.0
)
FetchContent_MakeAvailable(webview)

# ---- cJSON (vendored) ----
add_library(cjson STATIC kui/third_party/cjson/cJSON.c)
target_include_directories(cjson PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/kui/third_party/cjson
)

# ---- KUI (static library) ----
file(GLOB KUI_SRC CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/kui/src/*.c"
)

add_library(kui STATIC
  ${KUI_SRC}
  "${KUI_RESOURCES_C}"          # compile embedded runtime into the lib
)

# the lib must see generated headers/incs and wait for the generators
add_dependencies(kui ${PRELUDE_GEN_TARGET} runtime_resources)
target_include_directories(kui
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen  # for prelude.h
          ${KUI_GEN_DIR}                   # for resources includes
          ${CMAKE_CURRENT_SOURCE_DIR}/kui/third_party
)

target_link_libraries(kui PRIVATE webview::core_static cjson)
if(WIN32)
  target_compile_definitions(kui PRIVATE WEBVIEW_EDGE=1)
  target_link_libraries(kui PRIVATE user32 ole32 shell32 advapi32)
endif()

# If you ever ship a DLL build again, you can add your KUI_API toggles here.
# For a pure static lib, no declspecs are needed.

# ---- App (demo executable) ----
file(GLOB DEMO_SRC CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

if(WIN32)
  add_executable(app WIN32 ${DEMO_SRC})
  target_link_libraries(app PRIVATE user32 gdi32)
elseif(APPLE)
  add_executable(app MACOSX_BUNDLE ${DEMO_SRC})
elseif(UNIX)
  add_executable(app ${DEMO_SRC})
  target_link_libraries(app PRIVATE m pthread dl)
endif()

set_target_properties(app PROPERTIES
    OUTPUT_NAME ${APP_NAME}
)

target_link_libraries(app PRIVATE kui)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)