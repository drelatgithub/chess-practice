#include <iostream>

#include "chess.hpp"

int main() {

    using namespace std;
    using namespace chess;

    auto bs = board_standard_opening();
    bs.pretty_print_to(cout);
    cout<<endl;

    return 0;
}
