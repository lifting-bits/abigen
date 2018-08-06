
#pragma once

// clang-format off
#if defined(__unix__)
  #if defined(SRCPARSER_EXPORTSYMBOLS)
    #define SRCPARSER_PUBLICSYMBOL __attribute__((visibility("default")))
  #else
    #define SRCPARSER_PUBLICSYMBOL
  #endif

#else
  #if defined(SRCPARSER_EXPORTSYMBOLS)
    #define SRCPARSER_PUBLICSYMBOL __declspec(dllexport)
  #else
    #define SRCPARSER_PUBLICSYMBOL __declspec(dllimport)
  #endif
#endif
// clang-format on
