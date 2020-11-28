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
    white_pawn_init, // can be captured by en passant

    black_king,
    black_queen,
    black_rook,
    black_bishop,
    black_knight,
    black_pawn,
    black_pawn_init, // can be captured by en passant

    last_
};
constexpr auto num_occupation_state() { return underlying(Occupation::last_); }
constexpr bool is_white_piece(Occupation o) {
    return underlying(o) >= underlying(Occupation::white_king)
        && underlying(o) <= underlying(Occupation::white_pawn_init);
}
constexpr bool is_black_piece(Occupation o) {
    return underlying(o) >= underlying(Occupation::black_king)
        && underlying(o) <= underlying(Occupation::black_pawn_init);
}
constexpr bool is_white_pawn(Occupation o) {
    return o == Occupation::white_pawn || o == Occupation::white_pawn_init;
}
constexpr bool is_black_pawn(Occupation o) {
    return o == Occupation::black_pawn || o == Occupation::black_pawn_init;
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
    inline static constexpr int max_side_size = std::max(width, height);
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
        const auto is_diag_enemy = [&](Occupation o) { return o == enemy_bishop || o == enemy_queen; };
        const auto is_cross_enemy = [&](Occupation o) { return o == enemy_rook || o == enemy_queen; };

        const auto has_enemy_dir = [&, this](int x_dir, int y_dir, auto&& pred_o) -> bool {
            for(int step = 1; step < max_side_size; ++step) {
                const int nx = x + step * x_dir;
                const int ny = y + step * y_dir;
                if(is_location_valid(nx, ny)) {
                    const auto o = (*this)(nx, ny);
                    if(o != Occupation::empty) {
                        return pred_o(o);
                    }
                } else {
                    // out of bound, terminate diag search
                    return false;
                }
            }
            return false;
        };
        if(
            has_enemy_dir(1, 1, is_diag_enemy)
            || has_enemy_dir(-1, 1, is_diag_enemy)
            || has_enemy_dir(-1, -1, is_diag_enemy)
            || has_enemy_dir(1, -1, is_diag_enemy)

            || has_enemy_dir(1, 0, is_cross_enemy)
            || has_enemy_dir(0, 1, is_cross_enemy)
            || has_enemy_dir(-1, 0, is_cross_enemy)
            || has_enemy_dir(0, -1, is_cross_enemy)
        ) {
            return true;
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


struct GameState {
    enum class Status {
        active, white_win, black_win, draw
    };

    BoardState board_state;
    bool       black_turn = false;

    // generated state
    int        white_king_x = 0;
    int        white_king_y = 0;
    int        black_king_x = 0;
    int        black_king_y = 0;
    bool       check = false;
    Status     status = Status::active;
};

constexpr GameState game_standard_opening() {
    using enum Occupation;

    GameState game_state;
    auto& state = game_state.board_state;

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

    game_state.black_turn = false;
    game_state.white_king_x = 4;
    game_state.white_king_y = 0;
    game_state.black_king_x = 4;
    game_state.black_king_y = 7;

    return game_state;
}



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
//
// Note:
//   - This function does not check whether the move would leave the king in a
//     checked state.
inline auto validate_operation(const GameState& game_state, Operation op) {
    using enum Occupation;

    auto occu_before = game_state.board_state(op.x0, op.y0);

    if(occu_before == empty) {
        return OperationValidationResult { false, "Not a piece." };
    }
    if(op.x0 == op.x1 && op.y0 == op.y1) {
        return OperationValidationResult { false, "Not a valid move." };
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

    const bool target_occupied_by_friend =
        (game_state.black_turn && is_black_piece(occu_after)) ||
        (!game_state.black_turn && is_white_piece(occu_after));
    const bool target_occupied_by_enemy =
        (game_state.black_turn && is_white_piece(occu_after)) ||
        (!game_state.black_turn && is_black_piece(occu_after));

    // auxiliary functions
    const auto check_king_move = [&] {
        return (abs(op.x0 - op.x1) <= 1 && abs(op.y0 - op.y1) <= 1)
            && !target_occupied_by_friend
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

    const auto check_diag_move = [&] {
        if(target_occupied_by_friend || abs(op.x1 - op.x0) != abs(op.y1 - op.y0)) return false;

        // check empty along path
        const int num_step = abs(op.x1 - op.x0);
        if(op.x1 > op.x0) {
            if(op.y1 > op.y0) {
                for(int step = 1; step < num_step; ++step) {
                    if(game_state.board_state(op.x0 + step, op.y0 + step) != empty) return false;
                }
            }
            else {
                for(int step = 1; step < num_step; ++step) {
                    if(game_state.board_state(op.x0 + step, op.y0 - step) != empty) return false;
                }
            }
        }
        else {
            if(op.y1 > op.y0) {
                for(int step = 1; step < num_step; ++step) {
                    if(game_state.board_state(op.x0 - step, op.y0 + step) != empty) return false;
                }
            }
            else {
                for(int step = 1; step < num_step; ++step) {
                    if(game_state.board_state(op.x0 - step, op.y0 - step) != empty) return false;
                }
            }
        }

        return true;
    };
    const auto check_cross_move = [&] {
        if(target_occupied_by_friend || (op.x1 != op.x0 && op.y1 != op.y0)) return false;

        // check empty along path
        if(op.x1 == op.x0) {
            const int num_step = abs(op.y1 - op.y0);
            if(op.y1 > op.y0) {
                for(int step = 1; step < num_step; ++step) {
                    if(game_state.board_state(op.x0, op.y0 + step) != empty) return false;
                }
            }
            else {
                for(int step = 1; step < num_step; ++step) {
                    if(game_state.board_state(op.x0, op.y0 - step) != empty) return false;
                }
            }
        }
        else {
            const int num_step = abs(op.x1 - op.x0);
            if(op.x1 > op.x0) {
                for(int step = 1; step < num_step; ++step) {
                    if(game_state.board_state(op.x0 + step, op.y0) != empty) return false;
                }
            }
            else {
                for(int step = 1; step < num_step; ++step) {
                    if(game_state.board_state(op.x0 - step, op.y0) != empty) return false;
                }
            }
        }

        return true;
    };

    const auto check_knight_move = [&] {
        return !target_occupied_by_friend
            &&
                (
                    (abs(op.y1 - op.y0) == 2 && abs(op.x1 - op.x0) == 1) ||
                    (abs(op.y1 - op.y0) == 1 && abs(op.x1 - op.x0) == 1)
                );
    };

    const auto check_pawn_move = [&] {
        return
            // move forward
            (
                op.y1 - op.y0 == (game_state.black_turn ? -1 : 1)
                && op.x1 == op.x0
                && !target_occupied_by_friend
                && !target_occupied_by_enemy
            )
            // or capture
            || (
                op.y1 - op.y0 == (game_state.black_turn ? -1 : 1)
                && abs(op.x1 - op.x0) == 1
                && target_occupied_by_enemy
            )
            // or en passant capture
            || (
                op.y1 - op.y0 == (game_state.black_turn ? -1 : 1)
                && abs(op.x1 - op.x0) == 1
                && game_state.board_state(op.x1, op.y0) == (game_state.black_turn ? white_pawn_init : black_pawn_init)
                && !target_occupied_by_friend
                && !target_occupied_by_enemy
            );
    };
    const auto check_pawn_promote = [&] {
        return
            (
                !game_state.black_turn && (
                    op.code == underlying(white_queen) ||
                    op.code == underlying(white_rook) ||
                    op.code == underlying(white_bishop) ||
                    op.code == underlying(white_knight)
                )
            )
            || (
                game_state.black_turn && (
                    op.code == underlying(black_queen) ||
                    op.code == underlying(black_rook) ||
                    op.code == underlying(black_bishop) ||
                    op.code == underlying(black_knight)
                )
            );
    };

    const auto check_pawn_init_move = [&] {
        return
            // move forward
            (
                op.y1 - op.y0 == (game_state.black_turn ? -1 : 1)
                && op.x1 == op.x0
                && !target_occupied_by_friend
                && !target_occupied_by_enemy
            )
            // or skip forward
            || (
                op.y1 - op.y0 == (game_state.black_turn ? -2 : 2)
                && op.x1 == op.x0
                && !target_occupied_by_friend
                && !target_occupied_by_enemy
                && game_state.board_state(op.x0, op.y0 + (game_state.black_turn ? -1 : 1)) == empty
            )
            // or capture
            || (
                op.y1 - op.y0 == (game_state.black_turn ? -1 : 1)
                && abs(op.x1 - op.x0) == 1
                && target_occupied_by_enemy
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

        case white_queen: [[fallthrough]];
        case black_queen:

            if(op.category != Operation::Category::move) {
                return OperationValidationResult { false, "Invalid queen operation." };
            }
            if(!check_diag_move() && !check_cross_move()) {
                return OperationValidationResult { false, "Invalid queen move." };
            }
            break;

        case white_bishop: [[fallthrough]];
        case black_bishop:

            if(op.category != Operation::Category::move) {
                return OperationValidationResult { false, "Invalid bishop operation." };
            }
            if(!check_diag_move()) {
                return OperationValidationResult { false, "Invalid bishop move." };
            }
            break;

        case white_rook: [[fallthrough]];
        case black_rook:

            if(op.category != Operation::Category::move) {
                return OperationValidationResult { false, "Invalid rook operation." };
            }
            if(!check_cross_move()) {
                return OperationValidationResult { false, "Invalid rook move." };
            }
            break;

        case white_knight: [[fallthrough]];
        case black_knight:

            if(op.category != Operation::Category::move) {
                return OperationValidationResult { false, "Invalid knight operation." };
            }
            if(!check_knight_move()) {
                return OperationValidationResult { false, "Invalid knight move." };
            }
            break;

        case white_pawn: [[fallthrough]];
        case black_pawn:

            if(op.y1 == (game_state.black_turn ? 0 : 7)) {
                if(
                    op.category != Operation::Category::promote
                    || !check_pawn_move()
                    || !check_pawn_promote()
                ) {
                    return OperationValidationResult { false, "Invalid pawn promote." };
                }
            }
            else {
                if(
                    op.category != Operation::Category::move
                    || !check_pawn_move()
                ) {
                    return OperationValidationResult { false, "Invalid pawn move." };
                }
            }
            break;

        case white_pawn_init: [[fallthrough]];
        case black_pawn_init:

            if(op.category != Operation::Category::move) {
                return OperationValidationResult { false, "Invalid pawn operation." };
            }
            if(!check_pawn_init_move()) {
                return OperationValidationResult { false, "Invalid pawn move." };
            }
            break;
    }

    return OperationValidationResult { true };
}

// Apply an operation in place without checking for validity.
inline void apply_operation_in_place(GameState& game_state, Operation op) {
    using enum Occupation;

    const auto is_enemy = [&](Occupation o) {
        return (game_state.black_turn ? is_white_piece(o) : is_black_piece(o));
    };

    auto& piece0 = game_state.board_state(op.x0, op.y0);

    if(op.category == Operation::Category::move) {
        auto& piece1 = game_state.board_state(op.x1, op.y1);

        // en passant
        if((piece0 == black_pawn || piece0 == white_pawn) && piece1 == empty && op.x0 != op.x1) {
            game_state.board_state(op.x1, op.y0) = empty; // captured
        }
        // castle disabling
        if(piece0 == white_rook) {
            if(op.x0 == 0 && op.y0 == 0) game_state.board_state.white_castle_queen = false;
            if(op.x0 == 7 && op.y0 == 0) game_state.board_state.white_castle_king = false;
        }
        if(piece0 == black_rook) {
            if(op.x0 == 0 && op.y0 == 7) game_state.board_state.black_castle_queen = false;
            if(op.x0 == 7 && op.y0 == 7) game_state.board_state.black_castle_king = false;
        }
        if(piece0 == white_king) {
            game_state.board_state.white_castle_queen = false;
            game_state.board_state.white_castle_king = false;
            game_state.white_king_x = op.x1;
            game_state.white_king_y = op.y1;
        }
        if(piece0 == black_king) {
            game_state.board_state.black_castle_queen = false;
            game_state.board_state.black_castle_king = false;
            game_state.black_king_x = op.x1;
            game_state.black_king_y = op.y1;
        }

        piece1 = piece0;
        piece0 = empty;

    }
    else if(op.category == Operation::Category::castle) {
        if(op.y1 == 0) {
            if(op.x1 == 2) {
                // white queen
                piece0 = empty;
                game_state.board_state(0, 0) = empty;
                game_state.board_state(2, 0) = white_king;
                game_state.board_state(3, 0) = white_rook;
                game_state.white_king_x = 2;
                game_state.white_king_y = 0;
            }
            else {
                // white king
                piece0 = empty;
                game_state.board_state(7, 0) = empty;
                game_state.board_state(6, 0) = white_king;
                game_state.board_state(5, 0) = white_rook;
                game_state.white_king_x = 6;
                game_state.white_king_y = 0;
            }
            game_state.board_state.white_castle_queen = false;
            game_state.board_state.white_castle_king = false;
        }
        else {
            if(op.x1 == 2) {
                // black queen
                piece0 = empty;
                game_state.board_state(0, 7) = empty;
                game_state.board_state(2, 7) = black_king;
                game_state.board_state(3, 7) = black_rook;
                game_state.black_king_x = 2;
                game_state.black_king_y = 7;
            }
            else {
                // black king
                piece0 = empty;
                game_state.board_state(7, 7) = empty;
                game_state.board_state(6, 7) = black_king;
                game_state.board_state(5, 7) = black_rook;
                game_state.black_king_x = 6;
                game_state.black_king_y = 7;
            }
            game_state.board_state.black_castle_queen = false;
            game_state.board_state.black_castle_king = false;
        }
    }
    else if(op.category == Operation::Category::promote) {
        auto& piece1 = game_state.board_state(op.x1, op.y1);
        piece1 = piece0;
        piece0 = empty;
    }

}


// This function generates all valid moves without checking king checked
// status.
//
// Func: function type with signature (Operation) -> void
template< typename Func >
inline void pseudo_valid_operation_generator(const GameState& game_state, Func&& func) {
    using enum Occupation;

    const auto validate_and_run_func = [&](Operation op) {
        if(validate_operation(game_state, op).okay) {
            func(op);
        }
    };

    const auto gen_move = [&](int x, int y, int dx, int dy) {
        validate_and_run_func(
            Operation { Operation::Category::move, x, y, x + dx, y + dy }
        );
    };
    const auto gen_dir_move = [&](int x, int y, int x_dir, int y_dir) {
        for(int step = 1; step < BoardState::max_side_size; ++step) {
            gen_move(x, y, step * x_dir, step * y_dir);
        }
    };
    const auto gen_diag_move = [&](int x, int y) {
        gen_dir_move(x, y, 1, 1);
        gen_dir_move(x, y, -1, 1);
        gen_dir_move(x, y, -1, -1);
        gen_dir_move(x, y, 1, -1);
    };
    const auto gen_cross_move = [&](int x, int y) {
        gen_dir_move(x, y, 1, 0);
        gen_dir_move(x, y, 0, 1);
        gen_dir_move(x, y, -1, 0);
        gen_dir_move(x, y, 0, -1);
    };

    for(int i = 0; i < BoardState::size; ++i) {
        const auto piece = game_state.board_state.board[i];
        if(black_turn ? is_black_piece(piece) : is_white_piece(piece)) {
            const int [x, y] = BoardState::index_to_coord(i);

            switch(piece) {
                case white_king: [[fallthrough]];
                case black_king:

                    // generate move
                    for(int dx = -1; dx <= 1; ++dx) for(int dy = -1; dy <= 1; ++dy) if(dx || dy) {
                        gen_move(x, y, dx, dy);
                    }
                    // generate castle
                    if(piece == white_king && x == 4 && y == 0) {
                        validate_and_run_func(Operation { Operation::Category::castle, 4, 0, 2, 0 });
                        validate_and_run_func(Operation { Operation::Category::castle, 4, 0, 6, 0 });
                    }
                    if(piece == black_king && x == 4 && y == 7) {
                        validate_and_run_func(Operation { Operation::Category::castle, 4, 7, 2, 7 });
                        validate_and_run_func(Operation { Operation::Category::castle, 4, 7, 6, 7 });
                    }
                    break;

                case white_queen: [[fallthrough]];
                case black_queen:

                    // generate move
                    gen_cross_move(x, y);
                    gen_diag_move(x, y);
                    break;

                case white_rook: [[fallthrough]];
                case black_rook:

                    gen_cross_move(x, y);
                    break;

                case white_bishop: [[fallthrough]];
                case black_bishop:

                    gen_diag_move(x, y);
                    break;

                case white_knight: [[fallthrough]];
                case black_knight:

                    gen_move(x, y, 2, 1);
                    gen_move(x, y, 1, 2);
                    gen_move(x, y, -1, 2);
                    gen_move(x, y, -2, 1);
                    gen_move(x, y, -2, -1);
                    gen_move(x, y, -1, -2);
                    gen_move(x, y, 1, -2);
                    gen_move(x, y, 2, -1);
                    break;

                case white_pawn_init:
                    gen_move(x, y, 0, 2);
                    [[fallthrough]];

                case white_pawn:

                    gen_move(x, y, -1, 1);
                    gen_move(x, y, 0, 1);
                    gen_move(x, y, 1, 1);
                    break;

                case black_pawn_init:
                    gen_move(x, y, 0, -2);
                    [[fallthrough]];

                case black_pawn:

                    gen_move(x, y, -1, -1);
                    gen_move(x, y, 0, -1);
                    gen_move(x, y, 1, -1);
                    break;

            }
        }
    }
}



// game procedure specification

// GetOp: function type that has signature () -> Operation
template< typename GetOp >
inline GameState game_round(const GameState& game_state, GetOp&& get_op) {
    using enum Occupation;

    const Operation op = get_op();

    //---------------------------------
    // operation pre-validation
    //---------------------------------
    const auto op_validation = validate_operation(game_state, op);
    if(!op_validation.okay) {
        std::cout << "Invalid operation: " << op_validation.error_message << std::endl;
        return game_state;
    }

    //---------------------------------
    // apply the operation
    //---------------------------------

    auto new_game_state = game_state;
    apply_operation_in_place(new_game_state, op);

    //---------------------------------
    // post validation
    //---------------------------------
    // check whether king is under attack
    {
        const auto friend_king_x = new_game_state.black_turn ? new_game_state.black_king_x : new_game_state.white_king_x;
        const auto friend_king_y = new_game_state.black_turn ? new_game_state.black_king_y : new_game_state.white_king_y;
        if(new_game_state.board_state.position_attacked(friend_king_x, friend_king_y, !new_game_state.black_turn)) {
            std::cout << "Invalid operation: king will be attacked." << std::endl;
            // reject new game state
            return game_state;
        }
    }

    //---------------------------------
    // post processing
    //---------------------------------

    // update check status
    {
        const auto enemy_king_x = new_game_state.black_turn ? new_game_state.white_king_x : new_game_state.black_king_x;
        const auto enemy_king_y = new_game_state.black_turn ? new_game_state.white_king_y : new_game_state.black_king_y;
        new_game_state.check = new_game_state.board_state.position_attacked(enemy_king_x, enemy_king_y, new_game_state.black_turn);
    }

    // check whether opponent can make any valid move

    //---------------------------------
    // prepare for next turn
    //---------------------------------

    new_game_state.black_turn = !game_state.black_turn;

    // refresh pawn state
    for(auto& p : new_game_state.board_state.board) {
        if(p == (new_game_state.black_turn ? black_pawn_init : white_pawn_ep)) {
            p = (new_game_state.black_turn ? black_pawn : white_pawn);
        }
    }

    return new_game_state;
}


} // namespace chess

#endif
