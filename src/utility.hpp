#ifndef CHESS_UTILITY_HPP
#define CHESS_UTILITY_HPP

#include <random>
#include <type_traits>

namespace chess {

// auxiliary functions
//-------------------------------------

// A random seed based on clock cycles
unsigned long long rdtsc();


// debug
//-------------------------------------

#ifdef NDEBUG
constexpr bool debug = false;
#else
constexpr bool debug = true;
#endif


// concepts
//-------------------------------------

template< typename T >
concept EnumType = std::is_enum_v< T >;

template< EnumType T >
constexpr auto underlying(T a) { return static_cast< std::underlying_type_t< T > >(a); }


// random
//-------------------------------------

inline std::mt19937 rand_gen(rdtsc());

} // namespace chess

#endif
