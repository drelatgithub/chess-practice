#include <iostream>

#include "chess.hpp"

int main() {

    using namespace std;
    using namespace chess;

    auto gs = game_standard_opening();
    gs.board_state.pretty_print_to(cout);
    cout<<endl;

    return 0;
}
