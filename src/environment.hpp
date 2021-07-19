#ifndef CHESS_ENVIRONMENT_HPP
#define CHESS_ENVIRONMENT_HPP

// Target platforms
//-----------------------------------------------------------------------------
#if defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
#endif
#if defined(__unix__) || defined(PLATFORM_MACOS)
    #define PLATFORM_UNIX_LIKE
#endif
#if defined(_WIN32)
    #define PLATFORM_WINDOWS
#endif

// Compiler
//-----------------------------------------------------------------------------
#if defined(__clang__)
    #define COMPILER_CLANG
#elif defined(__GNUC__)
    #define COMPILER_GCC
#elif defined(_MSC_VER)
    #define COMPILER_MSVC
#endif

#endif
