#ifndef CHESS_CHESS_OPERATION_HPP
#define CHESS_CHESS_OPERATION_HPP

#include <iostream>
#include <string>
#include <tuple>

#include "chess/board.hpp"
#include "utility.hpp"

namespace chess {

//-----------------------------------------------------------------------------
// Rules and state changes
//-----------------------------------------------------------------------------

struct Operation {
    enum class Category {
        none,
        move, castle, promote, resign, draw_accept
    };
    inline static constexpr int code2_normal     = 0;
    inline static constexpr int code2_draw_offer = 1;
    inline static constexpr int code2_draw_claim = 2;

    Category category = Category::none;
    int x0 = 0, y0 = 0;
    int x1 = 0, y1 = 0;

    // Special numbers for different category

    // promote: index of promoted piece
    int code = 0;

    // move/castle/promote:
    //   0: normal
    //   1: draw offer
    //   2: draw claim
    int code2 = code2_normal;
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
//   - This function does not check whether a draw claim is valid.
inline auto validate_operation(const GameState& game_state, Operation op) {
    using enum Occupation;

    const bool black_turn = game_state.board_state.black_turn;

    // early termination with special categories
    if(op.category == Operation::Category::none) {
        return OperationValidationResult { false, "Null operation not allowed." };
    }
    else if(op.category == Operation::Category::resign) {
        return OperationValidationResult { true };
    }
    else if(op.category == Operation::Category::draw_accept) {
        if(game_state.draw_offer) {
            return OperationValidationResult { true };
        }
        else {
            return OperationValidationResult { false, "Draw not offered." };
        }
    }


    auto occu_before = game_state.board_state(op.x0, op.y0);

    if(occu_before == empty) {
        return OperationValidationResult { false, "Not a piece." };
    }
    if(op.x0 == op.x1 && op.y0 == op.y1) {
        return OperationValidationResult { false, "Not a valid move." };
    }
    if(is_white_piece(occu_before) && black_turn) {
        return OperationValidationResult { false, "Black turn." };
    }
    if(is_black_piece(occu_before) && !black_turn) {
        return OperationValidationResult { false, "White turn." };
    }
    // check target valid
    if(!BoardState::is_location_valid(op.x1, op.y1)) {
        return OperationValidationResult { false, "Dst out of range." };
    }

    auto occu_after = game_state.board_state(op.x1, op.y1);

    const bool target_occupied_by_friend = black_turn ? is_black_piece(occu_after) : is_white_piece(occu_after);
    const bool target_occupied_by_enemy  = black_turn ? is_white_piece(occu_after) : is_black_piece(occu_after);

    // auxiliary functions
    const auto check_king_move = [&] {
        return (abs(op.x0 - op.x1) <= 1 && abs(op.y0 - op.y1) <= 1)
            && !target_occupied_by_friend
            && !game_state.board_state.position_attacked(op.x1, op.y1, !black_turn);
    };
    const auto check_king_castle = [&] {
        return 
            (
                // white castle
                (
                    !black_turn && (
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
                    black_turn && (
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
                op.y1 - op.y0 == (black_turn ? -1 : 1)
                && op.x1 == op.x0
                && !target_occupied_by_friend
                && !target_occupied_by_enemy
            )
            // or skip forward
            || (
                op.y0    == (black_turn ? 6 : 1)
                && op.y1 == (black_turn ? 4 : 3)
                && op.x1 == op.x0
                && !target_occupied_by_friend
                && !target_occupied_by_enemy
                && game_state.board_state(op.x0, op.y0 + (black_turn ? -1 : 1)) == empty
            )
            // or capture
            || (
                op.y1 - op.y0 == (black_turn ? -1 : 1)
                && abs(op.x1 - op.x0) == 1
                && target_occupied_by_enemy
            )
            // or en passant capture
            || (
                op.y0    == (black_turn ? 3 : 4)
                && op.y1 == (black_turn ? 2 : 5)
                && abs(op.x1 - op.x0) == 1
                && op.x1 == game_state.board_state.en_passant_column
                && game_state.board_state(op.x1, op.y0) == (black_turn ? white_pawn : black_pawn)
                && !target_occupied_by_friend
                && !target_occupied_by_enemy
            );
    };
    const auto check_pawn_promote = [&] {
        return
            (
                !black_turn && (
                    op.code == underlying(white_queen) ||
                    op.code == underlying(white_rook) ||
                    op.code == underlying(white_bishop) ||
                    op.code == underlying(white_knight)
                )
            )
            || (
                black_turn && (
                    op.code == underlying(black_queen) ||
                    op.code == underlying(black_rook) ||
                    op.code == underlying(black_bishop) ||
                    op.code == underlying(black_knight)
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

            if(op.y1 == (black_turn ? 0 : 7)) {
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
    }

    return OperationValidationResult { true };
}

// Apply an operation in place without checking for validity.
//
// Returns the new board state hash
inline auto apply_operation_in_place(
    GameState&                      game_state,
    BoardStateZobristTable::HashInt board_state_hash,
    Operation                       op,
    const BoardStateZobristTable&   hash_table
) {
    using enum Occupation;

    auto& board_state = game_state.board_state;
    const bool black_turn = board_state.black_turn;

    const auto is_enemy = [&](Occupation o) {
        return (black_turn ? is_white_piece(o) : is_black_piece(o));
    };
    const auto set_piece = [&](int x, int y, Occupation o) {
        aux_hash_set_board_piece(board_state_hash, board_state, hash_table, x, y, o);
    };
    const auto disable_white_castle_queen = [&] { aux_hash_set_bool(board_state_hash, board_state.white_castle_queen, hash_table.white_castle_queen, false); };
    const auto disable_white_castle_king  = [&] { aux_hash_set_bool(board_state_hash, board_state.white_castle_king,  hash_table.white_castle_king,  false); };
    const auto disable_black_castle_queen = [&] { aux_hash_set_bool(board_state_hash, board_state.black_castle_queen, hash_table.black_castle_queen, false); };
    const auto disable_black_castle_king  = [&] { aux_hash_set_bool(board_state_hash, board_state.black_castle_king,  hash_table.black_castle_king,  false); };

    const auto piece0 = board_state(op.x0, op.y0);

    // reset draw offer
    game_state.draw_offer = false;
    // reset en passant column
    aux_hash_set_en_passant_column(board_state_hash, board_state, hash_table, -1);

    // event flags
    bool pawn_moved = false;
    bool capture_made = false;

    if(op.category == Operation::Category::move) {
        const auto piece1 = board_state(op.x1, op.y1);

        // pawn special
        if(piece0 == black_pawn || piece0 == white_pawn) {
            // en passant
            if(piece1 == empty && op.x0 != op.x1) {
                // captured
                set_piece(op.x1, op.y0, empty);
                capture_made = true;
            }
            // initial skip
            if(abs(op.y1 - op.y0) == 2) {
                // check enemy pawn immediately at left or right
                const auto has_enemy_pawn = [&](int nx, int ny) {
                    return BoardState::is_location_valid(nx, ny)
                        && board_state(nx, ny) == (black_turn ? white_pawn : black_pawn);
                };
                if(has_enemy_pawn(op.x1 - 1, op.y1) || has_enemy_pawn(op.x1 + 1, op.y1)) {
                    // set en passant column
                    aux_hash_set_en_passant_column(board_state_hash, board_state, hash_table, op.x0);
                }
            }

            pawn_moved = true;
        }

        // castle disabling
        if(piece0 == white_rook) {
            if(op.x0 == 0 && op.y0 == 0) disable_white_castle_queen();
            if(op.x0 == 7 && op.y0 == 0) disable_white_castle_king();
        }
        if(piece0 == black_rook) {
            if(op.x0 == 0 && op.y0 == 7) disable_black_castle_queen();
            if(op.x0 == 7 && op.y0 == 7) disable_black_castle_king();
        }
        if(piece0 == white_king) {
            disable_white_castle_queen();
            disable_white_castle_king();
            game_state.white_king_x = op.x1;
            game_state.white_king_y = op.y1;
        }
        if(piece0 == black_king) {
            disable_black_castle_queen();
            disable_black_castle_king();
            game_state.black_king_x = op.x1;
            game_state.black_king_y = op.y1;
        }

        // check capture
        if(piece1 != empty) { capture_made = true; }

        set_piece(op.x1, op.y1, piece0);
        set_piece(op.x0, op.y0, empty);

        // draw offer
        if(op.code2 == Operation::code2_draw_offer) {
            game_state.draw_offer = true;
        }

    }
    else if(op.category == Operation::Category::castle) {
        if(op.y1 == 0) {
            if(op.x1 == 2) {
                // white queen
                set_piece(op.x0, op.y0, empty);
                set_piece(0,     0,     empty);
                set_piece(2,     0,     white_king);
                set_piece(3,     0,     white_rook);
                game_state.white_king_x = 2;
                game_state.white_king_y = 0;
            }
            else {
                // white king
                set_piece(op.x0, op.y0, empty);
                set_piece(7,     0,     empty);
                set_piece(6,     0,     white_king);
                set_piece(5,     0,     white_rook);
                game_state.white_king_x = 6;
                game_state.white_king_y = 0;
            }
            disable_white_castle_queen();
            disable_white_castle_king();
        }
        else {
            if(op.x1 == 2) {
                // black queen
                set_piece(op.x0, op.y0, empty);
                set_piece(0,     7,     empty);
                set_piece(2,     7,     black_king);
                set_piece(3,     7,     black_rook);
                game_state.black_king_x = 2;
                game_state.black_king_y = 7;
            }
            else {
                // black king
                set_piece(op.x0, op.y0, empty);
                set_piece(7,     7,     empty);
                set_piece(6,     7,     black_king);
                set_piece(5,     7,     black_rook);
                game_state.black_king_x = 6;
                game_state.black_king_y = 7;
            }
            disable_black_castle_queen();
            disable_black_castle_king();
        }

        // draw offer
        if(op.code2 == Operation::code2_draw_offer) {
            game_state.draw_offer = true;
        }
    }
    else if(op.category == Operation::Category::promote) {
        pawn_moved = true;
        if(board_state(op.x1, op.y1) != empty) { capture_made = true; }

        set_piece(op.x1, op.y1, static_cast<Occupation>(op.code));
        set_piece(op.x0, op.y0, empty);

        // draw offer
        if(op.code2 == Operation::code2_draw_offer) {
            game_state.draw_offer = true;
        }
    }
    else if(op.category == Operation::Category::resign) {
        game_state.status = black_turn ? GameState::Status::white_win : GameState::Status::black_win;
    }
    else if(op.category == Operation::Category::draw_accept) {
        game_state.status = GameState::Status::draw;
    }


    if(pawn_moved || capture_made) {
        game_state.no_capture_no_pawn_move_streak = 0;
    } else {
        ++game_state.no_capture_no_pawn_move_streak;
    }

    return board_state_hash;
}


// This function generates all valid moves without checking king checked
// status.
//
// Func: function type with signature (Operation) -> void
//
// Note:
//   - This function does not generate promotion operation for pawns
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
        if(game_state.board_state.black_turn ? is_black_piece(piece) : is_white_piece(piece)) {
            const auto [x, y] = BoardState::index_to_coord(i);

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

                case white_pawn:

                    gen_move(x, y, 0, 2);
                    gen_move(x, y, -1, 1);
                    gen_move(x, y, 0, 1);
                    gen_move(x, y, 1, 1);
                    break;

                case black_pawn:

                    gen_move(x, y, 0, -2);
                    gen_move(x, y, -1, -1);
                    gen_move(x, y, 0, -1);
                    gen_move(x, y, 1, -1);
                    break;

            }
        }
    }
}

inline int count_valid_operations(
    const GameState&                game_state,
    const BoardStateZobristTable&   hash_table,
    BoardStateZobristTable::HashInt board_state_hash
) {
    int count = 0;

    pseudo_valid_operation_generator(
        game_state,
        [&](Operation op) {
            auto new_game_state = game_state;
            apply_operation_in_place(new_game_state, board_state_hash, op, hash_table);

            if(!new_game_state.board_state.position_attacked(new_game_state.friend_king_x(), new_game_state.friend_king_y(), !new_game_state.board_state.black_turn)) {
                ++count;
            }
        }
    );
    return count;
}

//-----------------------------------------------------------------------------
// game procedure specification
//-----------------------------------------------------------------------------



struct GameHistory {

    struct BoardStateRef {
        // the hashed value of board
        BoardStateZobristTable::HashInt hash_res = 0;
        // the index of the board state in the game state history
        int                             index = -1;
    };
    // equality by hash_res only
    struct BoardStateRefEqual {
        constexpr bool operator()(const BoardStateRef& lhs, const BoardStateRef& rhs) const noexcept {
            return lhs.hash_res == rhs.hash_res;
        }
    };
    // hash by hash_res only
    struct BoardStateRefHash {
        constexpr std::size_t operator()(const BoardStateRef& val) const noexcept {
            return val.hash_res;
        }
    };

    struct GameHistoryItem {
        Operation                       op;
        GameState                       game_state;
        BoardStateZobristTable::HashInt board_state_hash = 0;
    };

    std::vector< GameHistoryItem > history;

    // The board state hash.
    //
    // Each item refers to the board state corresponding to a game state with
    // index in the game history.
    //
    // The equal range of any key represents the possible range of board states
    // that may compare equal.
    std::unordered_multiset<
        BoardStateRef,
        BoardStateRefHash,
        BoardStateRefEqual
    > board_state_ref;

    // Note:
    //   - Changing this value may break the board_state_ref multiset.
    BoardStateZobristTable zobrist_table = BoardStateZobristTable::generate();

    // Default constructor to initialize with the standard opening.
    GameHistory() {
        auto new_game_state = game_standard_opening();
        push_game_state(
            Operation {},
            new_game_state,
            hash_board_state(new_game_state.board_state)
        );
    }

    // This function gives the current game_state situation. This function
    // hides the implementation detail of the history vector. For example, if
    // the game allows undoing and redoing moves, the current state might be
    // not at the back of the vector.
    //
    // Returns null if there is no current item.
    auto ptr_current_item() const {
        return history.empty() ? nullptr : &history.back();
    }

    BoardStateZobristTable::HashInt hash_board_state(const BoardState& board_state) const {
        return hash(board_state, zobrist_table);
    }

    void push_game_state(const Operation& op, const GameState& game_state, BoardStateZobristTable::HashInt board_state_hash) {
        if constexpr(debug) {
            if(hash_board_state(game_state.board_state) != board_state_hash) {
                throw std::logic_error("Board state hash does not match.");
            }
        }

        board_state_ref.insert({
            board_state_hash,
            static_cast<int>(history.size())
        });
        history.push_back({ op, game_state, board_state_hash });
    }

    auto count_board_state_repetition(const BoardState& board_state, BoardStateZobristTable::HashInt board_state_hash) const {
        if constexpr(debug) {
            if(hash_board_state(board_state) != board_state_hash) {
                throw std::logic_error("Board state hash does not match.");
            }
        }

        const auto same_hash_range = board_state_ref.equal_range({ board_state_hash });
        return std::ranges::count_if(
            same_hash_range.first,
            same_hash_range.second,
            [&, this](const BoardStateRef& rhs) {
                return board_state == history[rhs.index].game_state.board_state;
            }
        );
    }
};


// Returns whether the operation is valid.
//
// Note:
//   - New game state will be pushed only if the operation is valid. Otherwise,
//     no progress will be made in game.
inline bool game_round(GameHistory& game_history, const Operation& op) {
    using enum Occupation;

    auto p_current_item = game_history.ptr_current_item();
    if(p_current_item == nullptr) {
        std::cout << "Game history is empty." << std::endl;
        return false;
    }

    const auto& game_state       = p_current_item->game_state;
    const auto  board_state_hash = p_current_item->board_state_hash;

    //---------------------------------
    // operation pre-validation
    //---------------------------------
    const auto op_validation = validate_operation(game_state, op);
    if(!op_validation.okay) {
        std::cout << "Invalid operation: " << op_validation.error_message << std::endl;
        return false;
    }

    //---------------------------------
    // apply the operation
    //---------------------------------

    auto new_game_state = game_state;
    auto new_board_state_hash = apply_operation_in_place(new_game_state, board_state_hash, op, game_history.zobrist_table);

    //---------------------------------
    // post validation
    //---------------------------------
    if(new_game_state.status == GameState::Status::active) {
        // check whether king is under attack
        if(new_game_state.board_state.position_attacked(new_game_state.friend_king_x(), new_game_state.friend_king_y(), !new_game_state.board_state.black_turn)) {
            std::cout << "Invalid operation: king will be attacked." << std::endl;
            // reject new game state
            return false;
        }

    }

    // toggle turn
    aux_hash_set_bool(new_board_state_hash, new_game_state.board_state.black_turn, game_history.zobrist_table.black_turn, !new_game_state.board_state.black_turn);

    const int num_repetition = game_history.count_board_state_repetition(new_game_state.board_state, new_board_state_hash);

    if(new_game_state.status == GameState::Status::active) {
        // check whether draw claim is valid
        if(op.code2 == Operation::code2_draw_claim) {
            if(
                // threefold repetition
                num_repetition >= 3
                // fifty-move
                || new_game_state.no_capture_no_pawn_move_streak >= 100
            ) {
                // draw claim is valid, game ends in draw
                new_game_state.status = GameState::Status::draw;
            }
            else {
                // draw claim invalid
                std::cout << "Invalid operation: cannot claim draw." << std::endl;
                return false;
            }
        }
    }


    //---------------------------------
    // post processing
    //---------------------------------

    if(new_game_state.status == GameState::Status::active) {

        // update check status
        new_game_state.check = new_game_state.board_state.position_attacked(new_game_state.friend_king_x(), new_game_state.friend_king_y(), !new_game_state.board_state.black_turn);

        // check whether this player can make any valid move
        const int num_valid_op = count_valid_operations(new_game_state, game_history.zobrist_table, new_board_state_hash);
        if(num_valid_op == 0) {
            if(new_game_state.check) {
                // checkmate, the opponent (ie the player of this function) wins
                new_game_state.status = new_game_state.board_state.black_turn ? GameState::Status::white_win : GameState::Status::black_win;
            }
            else {
                // stalemate, draw
                new_game_state.status = GameState::Status::draw;
            }
        }
        else {
            // check special draw status
            if(
                // fivefold repetition
                num_repetition >= 5
                // 75-move
                || new_game_state.no_capture_no_pawn_move_streak >= 150
            ) {
                new_game_state.status = GameState::Status::draw;
            }
        }
    }

    //---------------------------------
    // prepare for next turn
    //---------------------------------

    game_history.push_game_state(op, new_game_state, new_board_state_hash);

    return true;
}


} // namespace chess

#endif
