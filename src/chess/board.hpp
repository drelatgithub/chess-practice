#ifndef CHESS_CHESS_BOARD_HPP
#define CHESS_CHESS_BOARD_HPP

#include <algorithm> // max
#include <cstdint>
#include <iostream>
#include <ranges>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "utility.hpp"

namespace chess {

//-----------------------------------------------------------------------------
// piece and board definition
//-----------------------------------------------------------------------------

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
        return 0 <= x && x < width && 0 <= y && y < height;
    }
    static constexpr auto coord_to_index(int x, int y) {
        return width * y + x;
    }
    static constexpr auto index_to_coord(int index) {
        return std::tuple { index % width, index / width };
    }

    friend auto operator<=>(const BoardState&, const BoardState&) = default;

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

struct BoardStateZobristTable {
    using HashInt = std::uint64_t;

    HashInt board[BoardState::size][num_occupation_state()] {};

    HashInt white_castle_queen = 0;
    HashInt white_castle_king = 0;
    HashInt black_castle_queen = 0;
    HashInt black_castle_king = 0;

    static auto generate() {
        BoardStateZobristTable res;

        std::uniform_int_distribution< HashInt > dis;

        for(int i = 0; i < BoardState::size; ++i) {
            for(int j = 0; j < num_occupation_state(); ++j) {
                res.board[i][j] = dis(rand_gen);
            }
        }
        res.white_castle_queen = dis(rand_gen);
        res.white_castle_king  = dis(rand_gen);
        res.black_castle_queen = dis(rand_gen);
        res.black_castle_king  = dis(rand_gen);

        return res;
    }
};

constexpr auto hash(const BoardState& board_state, const BoardStateZobristTable& hash_table) {
    BoardStateZobristTable::HashInt res = 0;

    for(int i = 0; i < BoardState::size; ++i) {
        const int j = underlying(board_state.board[i]);
        res ^= hash_table.board[i][j];
    }

    if(board_state.white_castle_queen) res ^= hash_table.white_castle_queen;
    if(board_state.white_castle_king)  res ^= hash_table.white_castle_king;
    if(board_state.black_castle_queen) res ^= hash_table.black_castle_queen;
    if(board_state.black_castle_king)  res ^= hash_table.black_castle_king;

    return res;
}


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

    int        friend_king_x() const { return black_turn ? black_king_x : white_king_x; }
    int        friend_king_y() const { return black_turn ? black_king_y : white_king_y; }
    int        enemy_king_x() const { return black_turn ? white_king_x : black_king_x; }
    int        enemy_king_y() const { return black_turn ? white_king_y : black_king_y; }

    void pretty_print_to(std::ostream& os) const {
        os
            << "game status: "
            << (
                status == Status::active
                    ? (black_turn ? "black turn" : "white turn")
                    : status == Status::white_win ? "white wins"
                    : status == Status::black_win ? "black wins"
                    : "draw"
            ) << '\n'
            << "white: king at " << (char)(white_king_x + 'a') << (char)(white_king_y + '1')
                << " castle "
                << (
                    board_state.white_castle_queen
                        ? (board_state.white_castle_king ? "both" : "queen")
                        : (board_state.white_castle_king ? "king" : "none")
                )
                << "\n"
            << "black: king at " << (char)(black_king_x + 'a') << (char)(black_king_y + '1')
                << " castle "
                << (
                    board_state.black_castle_queen
                        ? (board_state.black_castle_king ? "both" : "queen")
                        : (board_state.black_castle_king ? "king" : "none")
                )
                << "\n"
            << "checked: " << check << '\n'
            << "\n";

        board_state.pretty_print_to(os);
    }
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
    for(int i = 0; i < BoardState::width; ++i) state(i, 1) = white_pawn_init;

    state(0, 7) = state(7, 7) = black_rook;
    state(1, 7) = state(6, 7) = black_knight;
    state(2, 7) = state(5, 7) = black_bishop;
    state(3, 7) = black_queen;
    state(4, 7) = black_king;
    for(int i = 0; i < BoardState::width; ++i) state(i, 6) = black_pawn_init;

    game_state.black_turn = false;
    game_state.white_king_x = 4;
    game_state.white_king_y = 0;
    game_state.black_king_x = 4;
    game_state.black_king_y = 7;

    return game_state;
}


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


    std::vector< GameState > history;

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

    auto hash_board_state(const BoardState& board_state) const {
        return hash(board_state, zobrist_table);
    }

    void push_game_state(const GameState& game_state, BoardStateZobristTable::HashInt board_state_hash) {
        board_state_ref.insert({
            board_state_hash,
            static_cast<int>(history.size())
        });
        history.push_back(game_state);
    }

    auto count_board_state_repetition(const BoardState& board_state, BoardStateZobristTable::HashInt board_state_hash) const {
        const auto same_hash_range = board_state_ref.equal_range({ board_state_hash });
        return std::ranges::count_if(
            same_hash_range.first,
            same_hash_range.second,
            [&, this](const BoardStateRef& rhs) {
                return board_state == history[rhs.index].board_state;
            }
        );
    }
};


} // namespace chess

#endif
