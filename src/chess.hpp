#ifndef CHESS_CHESS_HPP
#define CHESS_CHESS_HPP

#include <iostream>

#include "utility.hpp"

namespace chess {

// Piece definition

enum class Occupation {
    empty,

    white_king,
    white_king_castle,
    white_queen,
    white_rook,
    white_bishop,
    white_knight,
    white_pawn,
    white_pawn_ep, // en passant

    black_king,
    black_king_castle,
    black_queen,
    black_rook,
    black_bishop,
    black_knight,
    black_pawn,
    black_pawn_ep, // en passant

    last_
};
constexpr auto num_occupation_state() { return underlying(Occupation::last_); }

constexpr const char* occupation_text[] {
    " ",
    "♔", "♔", "♕", "♖", "♗", "♘", "♙", "♙",
    "♚", "♚", "♛", "♜", "♝", "♞", "♟", "♟"
};
constexpr auto text(Occupation o) { return occupation_text[underlying(o)]; }

// Board state definition
struct BoardState {
    inline static constexpr int width = 8;
    inline static constexpr int height = 8;
    inline static constexpr int size = width * height;

    // The board is stored in the following order:
    //   a1, ..., h1,  a2, ..., h7,  a8, ..., h8
    Occupation board[size] {};

    // Get element based on x and y index (0-based)
    constexpr auto& operator()(int x, int y) {
        return board[width * y + x];
    }
    constexpr const auto& operator()(int x, int y) const {
        return board[width * y + x];
    }

    // pretty print to ostream
    void pretty_print_to(std::ostream& os) const {
        os << "╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\n";
        for(int y = height - 1; y >= 0; --y) {
            os << "║ ";
            for(int x = 0; x < width; ++x) {
                os << (x > 0 ? " │ " : "") << text((*this)(x, y));
            }
            os << " ║ " << y + 1 << '\n';
            if(y > 0) {
                os << "╟───┼───┼───┼───┼───┼───┼───┼───╢\n";
            }
        }
        os << "╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n";
        os << "  a   b   c   d   e   f   g   h\n";
    }
};


constexpr BoardState board_standard_opening() {
    using enum Occupation;

    BoardState state;

    state(0, 0) = state(7, 0) = white_rook;
    state(1, 0) = state(6, 0) = white_knight;
    state(2, 0) = state(5, 0) = white_bishop;
    state(3, 0) = white_queen;
    state(4, 0) = white_king;
    for(int i = 0; i < BoardState::width; ++i) state(i, 1) = white_pawn;

    state(0, 7) = state(7, 7) = black_rook;
    state(1, 7) = state(6, 7) = black_knight;
    state(2, 7) = state(5, 7) = black_bishop;
    state(3, 7) = black_queen;
    state(4, 7) = black_king;
    for(int i = 0; i < BoardState::width; ++i) state(i, 6) = black_pawn;

    return state;
}

} // namespace chess

#endif
