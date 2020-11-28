#include <iostream>

#include "chess.hpp"

int main() {

    using namespace std;
    using namespace chess;

    auto gs = game_standard_opening();

    gs.board_state.pretty_print_to(cout);
    cout<<endl;
    auto new_gs = game_round(gs, []{return Operation{Operation::Category::move, 4,1, 4,3};});
    new_gs.board_state.pretty_print_to(cout);
    cout<<endl;

    return 0;
}
