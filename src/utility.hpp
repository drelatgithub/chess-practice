#ifndef CHESS_UTILITY_HPP
#define CHESS_UTILITY_HPP

#include <type_traits>

namespace chess {

template< typename T >
concept EnumType = std::is_enum_v< T >;

template< EnumType T >
constexpr auto underlying(T a) { return static_cast< std::underlying_type_t< T > >(a); }

} // namespace chess

#endif
