#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

#include "chess/operation.hpp"

int main() {

    using namespace std;
    using namespace chess;

    auto gs = game_standard_opening();
    bool print_status = true;

    while(gs.status == GameState::Status::active) {
        if(print_status) {
            gs.pretty_print_to(cout);
            cout << endl;
        }
        print_status = false;

        // prompt for input
        cout << (gs.black_turn ? "(black turn)" : "(white turn)") << " > " << flush;

        // parse input
        string line;
        getline(cin, line);
        istringstream iss(line);
        vector< string > words;
        string tmp_word;
        while(iss >> tmp_word) {
            words.push_back(tmp_word);
        }

        if(words.empty()) continue;
        if(words[0] == "exit") break;

        if(words[0] == "help") {
            cout
                << "    move: mv <source> <dst> [<promote>]\n"
                << "        promotion can be one of (q)ueen, (r)ook, (b)ishop, k(n)ight\n"
                << "        example: mv e2 e4\n"
                << "        example: mv a7 a8 q\n"
                << "    castle: <0-0 (king side) or 0-0-0 (queen side)>\n"
                << "        example: 0-0-0\n"
                << "\n"
                << "    print board: show\n"
                << "    quit: exit\n"
                << endl;
            continue;
        }
        if(words[0] == "show") {
            print_status = true;
            continue;
        }

        if(words[0] == "mv") {
            if(words.size() < 3 || words.size() > 4) {
                cout << "Invalid mv command." << endl;
            }
            else {
                const auto get_coord = [&](const string& s) -> optional< tuple<int, int> > {
                    if(s.size() != 2 || s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8') {
                        cout << "Unrecognized coordinate " << s << endl;
                        return {};
                    }
                    else return {{ s[0] - 'a', s[1] - '1' }};
                };
                Operation op { Operation::Category::move };
                const auto src = get_coord(words[1]);
                if(src.has_value()) {
                    tie(op.x0, op.y0) = *src;
                } else {
                    continue;
                }
                const auto dst = get_coord(words[2]);
                if(dst.has_value()) {
                    tie(op.x1, op.y1) = *dst;
                } else {
                    continue;
                }

                if(words.size() == 4) {
                    if(words[3].size() != 1) {
                        cout << "Unrecognized promotion " << words[3] << endl;
                        continue;
                    } else {
                        op.category = Operation::Category::promote;
                        const char p = words[3][0];
                        if(p == 'q') {
                            op.code = underlying(gs.black_turn ? Occupation::black_queen : Occupation::white_queen);
                        } else if(p == 'r') {
                            op.code = underlying(gs.black_turn ? Occupation::black_rook : Occupation::white_rook);
                        } else if(p == 'b') {
                            op.code = underlying(gs.black_turn ? Occupation::black_bishop : Occupation::white_bishop);
                        } else if(p == 'n') {
                            op.code = underlying(gs.black_turn ? Occupation::black_knight : Occupation::white_knight);
                        } else {
                            cout << "Unrecognized promotion " << p << endl;
                            continue;
                        }
                    }
                }

                gs = game_round(
                    gs,
                    [&]() { return op; }
                );
                print_status = true;
            }
        }
        else if(words[0] == "0-0") {
            const int king_y = gs.black_turn ? 7 : 0;
            gs = game_round(
                gs,
                [&]() {
                    return Operation {
                        Operation::Category::castle,
                        4, king_y,
                        6, king_y
                    };
                }
            );
            print_status = true;
        }
        else if(words[0] == "0-0-0") {
            const int king_y = gs.black_turn ? 7 : 0;
            gs = game_round(
                gs,
                [&]() {
                    return Operation {
                        Operation::Category::castle,
                        4, king_y,
                        2, king_y
                    };
                }
            );
            print_status = true;
        }
        else {
            cout << "Unrecognized command " << words[0] << endl;
            continue;
        }

    }

    return 0;
}
