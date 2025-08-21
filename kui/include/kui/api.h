#pragma once
#ifdef _WIN32
  #ifdef KUI_BUILD
    #define KUI_API __declspec(dllexport)
  #else
    #define KUI_API __declspec(dllimport)
  #endif
#else
  #define KUI_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif