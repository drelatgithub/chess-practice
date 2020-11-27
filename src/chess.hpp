#ifndef CHESS_CHESS_HPP
#define CHESS_CHESS_HPP

#include <algorithm> // max
#include <iostream>
#include <string>
#include <tuple>

#include "utility.hpp"

namespace chess {

// Piece definition

enum class Occupation {
    empty,

    white_king,
    white_queen,
    white_rook,
    white_bishop,
    white_knight,
    white_pawn,
    white_pawn_ep, // en passant

    black_king,
    black_queen,
    black_rook,
    black_bishop,
    black_knight,
    black_pawn,
    black_pawn_ep, // en passant

    last_
};
constexpr auto num_occupation_state() { return underlying(Occupation::last_); }
constexpr bool is_white_piece(Occupation o) {
    return underlying(o) >= underlying(Occupation::white_king)
        && underlying(o) <= underlying(Occupation::white_pawn_ep);
}
constexpr bool is_black_piece(Occupation o) {
    return underlying(o) >= underlying(Occupation::black_king)
        && underlying(o) <= underlying(Occupation::black_pawn_ep);
}
constexpr bool is_white_pawn(Occupation o) {
    return o == Occupation::white_pawn || o == Occupation::white_pawn_ep;
}
constexpr bool is_black_pawn(Occupation o) {
    return o == Occupation::black_pawn || o == Occupation::black_pawn_ep;
}

constexpr const char* occupation_text[] {
    " ",
    "♔", "♕", "♖", "♗", "♘", "♙", "♙",
    "♚", "♛", "♜", "♝", "♞", "♟", "♟"
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

    bool       white_castle_queen = true;
    bool       white_castle_king = true;
    bool       black_castle_queen = true;
    bool       black_castle_king = true;


    static constexpr bool is_location_valid(int x, int y) {
        return 0 <= x && x <= width && 0 <= y && y <= height;
    }
    static constexpr auto coord_to_index(int x, int y) {
        return width * y + x;
    }
    static constexpr auto index_to_coord(int index) {
        return std::tuple { index % width, index / width };
    }

    // Get element based on x and y index (0-based)
    constexpr auto& operator()(int x, int y) {
        return board[coord_to_index(x, y)];
    }
    constexpr const auto& operator()(int x, int y) const {
        return board[coord_to_index(x, y)];
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

    // check whether a position is attacked
    //
    // Note:
    //   - not counting en passant
    bool position_attacked(int x, int y, bool by_black) const {
        using enum Occupation;

        if(by_black) {
            if(
                (is_location_valid(x-1, y+1) && is_black_pawn((*this)(x-1, y+1))) ||
                (is_location_valid(x+1, y+1) && is_black_pawn((*this)(x+1, y+1)))
            ) return true;
        }
        else {
            if(
                (is_location_valid(x-1, y-1) && is_white_pawn((*this)(x-1, y-1))) ||
                (is_location_valid(x+1, y-1) && is_white_pawn((*this)(x+1, y-1)))
            ) return true;
        }

        const auto enemy_bishop = by_black ? black_bishop : white_bishop;
        const auto enemy_rook = by_black ? black_rook : white_rook;
        const auto enemy_queen = by_black ? black_queen : white_queen;
        const auto has_enemy_diag = [&, this](int nx, int ny) {
            if(is_location_valid(nx, ny)) {
                const auto o = (*this)(nx, ny);
                return o == enemy_bishop || o == enemy_queen;
            }
            return false;
        };
        const auto has_enemy_cross = [&, this](int nx, int ny) {
            if(is_location_valid(nx, ny)) {
                const auto o = (*this)(nx, ny);
                return o == enemy_rook || o == enemy_queen;
            }
            return false;
        };
        for(int step = 1; step < std::max(width, height); ++step) {
            if(
                has_enemy_diag(x+step, y+step) ||
                has_enemy_diag(x-step, y+step) ||
                has_enemy_diag(x-step, y-step) ||
                has_enemy_diag(x+step, y-step) ||
                has_enemy_cross(x+step, y) ||
                has_enemy_cross(x, y+step) ||
                has_enemy_cross(x-step, y) ||
                has_enemy_cross(x, y-step)
            ) return true;
        }

        const auto has_enemy_knight = [&, this](int nx, int ny) {
            return is_location_valid(nx, ny) && (*this)(nx, ny) == (by_black ? black_knight : white_knight);
        };
        if(
            has_enemy_knight(x+2, y+1) ||
            has_enemy_knight(x+1, y+2) ||
            has_enemy_knight(x-1, y+2) ||
            has_enemy_knight(x-2, y+1) ||
            has_enemy_knight(x-2, y-1) ||
            has_enemy_knight(x-1, y-2) ||
            has_enemy_knight(x+1, y-2) ||
            has_enemy_knight(x+2, y-1)
        ) return true;

        const auto has_enemy_king = [&, this](int nx, int ny) {
            return is_location_valid(nx, ny) && (*this)(nx, ny) == (by_black ? black_king : white_king);
        };
        if(
            has_enemy_king(x+1, y) ||
            has_enemy_king(x+1, y+1) ||
            has_enemy_king(x, y+1) ||
            has_enemy_king(x-1, y+1) ||
            has_enemy_king(x-1, y) ||
            has_enemy_king(x-1, y-1) ||
            has_enemy_king(x, y-1) ||
            has_enemy_king(x+1, y-1)
        ) return true;

        // not attacked
        return false;
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

struct GameState {
    BoardState board_state;
    bool       black_turn = false;

    // generated state
    bool       check = false;
};

// Rules and state changes

struct Operation {
    enum class Category {
        move, castle, promote
    };
    Category category;
    int x0, y0;
    int x1, y1;

    // Special number for category
    //
    // promote: index of promoted piece
    int code = 0;
};

struct OperationValidationResult {
    bool okay = false;
    std::string error_message;
};
// This function assumes that
//   - The game is not in the checkmate state.
inline auto validate_operation(const GameState& game_state, Operation op) {
    using enum Occupation;

    auto occu_before = game_state.board_state(op.x0, op.y0);

    if(occu_before == empty) {
        return OperationValidationResult { false, "Not a piece." };
    }
    if(is_white_piece(occu_before) && game_state.black_turn) {
        return OperationValidationResult { false, "Black turn." };
    }
    if(is_black_piece(occu_before) && !game_state.black_turn) {
        return OperationValidationResult { false, "White turn." };
    }
    // check target valid
    if(!BoardState::is_location_valid(op.x1, op.y1)) {
        return OperationValidationResult { false, "Dst out of range." };
    }

    auto occu_after = game_state.board_state(op.x1, op.y1);

    // auxiliary functions
    const auto check_occupied = [&] {
        return !(game_state.black_turn && is_black_piece(occu_after))
            && !(!game_state.black_turn && is_white_piece(occu_after));
    };

    const auto check_king_move = [&] {
        return (abs(op.x0 - op.x1) <= 1 && abs(op.y0 - op.y1) <= 1)
            && check_occupied()
            && !game_state.board_state.position_attacked(op.x1, op.y1, !game_state.black_turn);
    };
    const auto check_king_castle = [&] {
        return 
            (
                // white castle
                (
                    !game_state.black_turn && (
                        // white queen
                        (
                            game_state.board_state.white_castle_queen
                            && !game_state.check
                            && op.x0 == 4 && op.y0 == 0
                            && op.x1 == 2 && op.y1 == 0
                            && game_state.board_state(1, 0) == empty
                            && game_state.board_state(2, 0) == empty
                            && game_state.board_state(3, 0) == empty
                            && !game_state.board_state.position_attacked(2, 0, true)
                            && !game_state.board_state.position_attacked(3, 0, true)
                        )
                        // white king
                        || (
                            game_state.board_state.white_castle_king
                            && !game_state.check
                            && op.x0 == 4 && op.y0 == 0
                            && op.x1 == 6 && op.y1 == 0
                            && game_state.board_state(5, 0) == empty
                            && game_state.board_state(6, 0) == empty
                            && !game_state.board_state.position_attacked(5, 0, true)
                            && !game_state.board_state.position_attacked(6, 0, true)
                        )
                    )
                )
                // or black castle
                || (
                    game_state.black_turn && (
                        // black queen
                        (
                            game_state.board_state.black_castle_queen
                            && !game_state.check
                            && op.x0 == 4 && op.y0 == 7
                            && op.x1 == 2 && op.y1 == 7
                            && game_state.board_state(1, 7) == empty
                            && game_state.board_state(2, 7) == empty
                            && game_state.board_state(3, 7) == empty
                            && !game_state.board_state.position_attacked(2, 7, false)
                            && !game_state.board_state.position_attacked(3, 7, false)
                        )
                        // black king
                        || (
                            game_state.board_state.black_castle_king
                            && !game_state.check
                            && op.x0 == 4 && op.y0 == 7
                            && op.x1 == 6 && op.y1 == 7
                            && game_state.board_state(5, 0) == empty
                            && game_state.board_state(6, 0) == empty
                            && !game_state.board_state.position_attacked(5, 7, false)
                            && !game_state.board_state.position_attacked(6, 7, false)
                        )
                    )
                )
            );
    };


    switch(occu_before) {
        // king
        case white_king: [[fallthrough]];
        case black_king:

            if(op.category == Operation::Category::move) {
                if(!check_king_move()) {
                    return OperationValidationResult { false, "Invalid king move." };
                }
            }
            else if(op.category == Operation::Category::castle) {
                if(!check_king_castle()) {
                    return OperationValidationResult { false, "Invalid king castle." };
                }
            }
            else {
                return OperationValidationResult { false, "Invalid king operation." };
            }
            break;
    }

    return OperationValidationResult { true };
}


} // namespace chess

#endif
