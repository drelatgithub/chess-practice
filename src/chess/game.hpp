#ifndef CHESS_CHESS_GAME_HPP
#define CHESS_CHESS_GAME_HPP

#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

#include "chess/operation.hpp"
#include "utility.hpp"

namespace chess {

// Validates and progresses the game.
// Returns whether the command is valid and progresses the game.
// If the game progresses, contents in os_message will be displayed to everyone. Otherwise, they will be returned to the sender only.
inline bool server_game_step(GameHistory& gh, bool from_black, std::string command, std::ostream& os_message) {
    using namespace std;

    const auto gs = [&]() -> const GameState& { return gh.ptr_current_item()->game_state; };
    const auto bh = [&] { return gh.ptr_current_item()->board_state_hash; };

    if(gs().status == GameState::Status::active) {

        // Print board status
        const auto print_status = [&] {
            gs().pretty_print_to(os_message);
            os_message << "board hash: " << bh() << '\n';
            os_message << "board repetition: " << gh.count_board_state_repetition(gs().board_state, bh()) << '\n';
            os_message << endl;
        };
        const auto command_prompt = [&] { return from_black ? "black> " : "white> "; };

        // parse input
        istringstream iss(command);
        vector< string > words;
        string tmp_word;
        while(iss >> tmp_word) {
            words.push_back(tmp_word);
        }

        // General commands.
        if(words.empty()) return false;
        else if(words[0] == "exit") return false;

        else if(words[0] == "help") {
            os_message
                << "    move: mv/dcmv/domv <source> <dst> [<promote>]\n"
                << "        Promotion can be one of (q)ueen, (r)ook, (b)ishop, k(n)ight\n"
                << "        Use dcmv to make a draw claim, and domv to make a draw offer.\n"
                << "        example: mv e2 e4\n"
                << "        example: mv a7 a8 q\n"
                << "    castle: <0-0 (king side) or 0-0-0 (queen side)>\n"
                << "        example: 0-0-0\n"
                << "    draw accept: da\n"
                << "    resign: resign\n"
                << "\n"
                << "    print board: show\n"
                << "    quit: exit\n"
                << endl;
            return false;
        }
        else if(words[0] == "show") {
            print_status();
            return false;
        }

        // Commands below require one's turn.
        if(gs().board_state.black_turn != from_black) {
            os_message << "Error: not your turn." << endl;
            return false;
        }


        if(words[0] == "resign") {
            bool valid = game_round(
                gh,
                Operation { Operation::Category::resign }
            );
            if(valid) os_message << command_prompt() << " resigned.";
            return valid;
        }
        else if(words[0] == "da") {
            bool valid = game_round(
                gh,
                Operation { Operation::Category::draw_accept }
            );
            if(valid) os_message << command_prompt() << " accepted draw.";
            return valid;
        }

        else if(words[0] == "mv" || words[0] == "dcmv" || words[0] == "domv") {
            int code2 = 
                words[0] == "dcmv" ? Operation::code2_draw_claim
                : words[0] == "domv" ? Operation::code2_draw_offer
                : Operation::code2_normal;

            if(words.size() < 3 || words.size() > 4) {
                os_message << "Invalid mv command." << endl;
                return false;
            }
            else {
                const auto get_coord = [&](const string& s) -> optional< tuple<int, int> > {
                    if(s.size() != 2 || s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8') {
                        os_message << "Unrecognized coordinate " << s << endl;
                        return {};
                    }
                    else return {{ s[0] - 'a', s[1] - '1' }};
                };
                Operation op { Operation::Category::move };
                op.code2 = code2;
                const auto src = get_coord(words[1]);
                if(src.has_value()) {
                    tie(op.x0, op.y0) = *src;
                } else {
                    return false;
                }
                const auto dst = get_coord(words[2]);
                if(dst.has_value()) {
                    tie(op.x1, op.y1) = *dst;
                } else {
                    return false;
                }

                if(words.size() == 4) {
                    if(words[3].size() != 1) {
                        os_message << "Unrecognized promotion " << words[3] << endl;
                        return false;
                    } else {
                        op.category = Operation::Category::promote;
                        const char p = words[3][0];
                        if(p == 'q') {
                            op.code = underlying(gs().board_state.black_turn ? Occupation::black_queen : Occupation::white_queen);
                        } else if(p == 'r') {
                            op.code = underlying(gs().board_state.black_turn ? Occupation::black_rook : Occupation::white_rook);
                        } else if(p == 'b') {
                            op.code = underlying(gs().board_state.black_turn ? Occupation::black_bishop : Occupation::white_bishop);
                        } else if(p == 'n') {
                            op.code = underlying(gs().board_state.black_turn ? Occupation::black_knight : Occupation::white_knight);
                        } else {
                            os_message << "Unrecognized promotion " << p << endl;
                            return false;
                        }
                    }
                }

                bool valid = game_round(gh, op);
                if(valid) print_status();
                return valid;
            }
        }
        else if(words[0] == "0-0") {
            const int king_y = gs().board_state.black_turn ? 7 : 0;
            bool valid = game_round(
                gh,
                Operation {
                    Operation::Category::castle,
                    4, king_y,
                    6, king_y
                }
            );
            if(valid) print_status();
            return valid;
        }
        else if(words[0] == "0-0-0") {
            const int king_y = gs().board_state.black_turn ? 7 : 0;
            bool valid = game_round(
                gh,
                Operation {
                    Operation::Category::castle,
                    4, king_y,
                    2, king_y
                }
            );
            if(valid) print_status();
            return valid;
        }
        else {
            os_message << "Unrecognized command " << words[0] << endl;
            return false;
        }


    }

    else {
        // Not active game status.
        gs().pretty_print_to(os_message);
        os_message << endl;
        return false;
    }
}

} // namespace chess

#endif
