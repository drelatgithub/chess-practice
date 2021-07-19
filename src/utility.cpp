#include "utility.hpp"

#include "environment.hpp"

#ifdef COMPILER_MSVC
    #include <intrin.h>
    #pragma intrinsic(__rdtsc)
#endif

namespace chess {

unsigned long long rdtsc() {

#ifdef COMPILER_MSVC
    return __rdtsc();

#else
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;

#endif
}

} // namespace chess
