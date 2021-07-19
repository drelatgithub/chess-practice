#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

#include "client.hpp"
#include "server.hpp"

int main(int argc, char** argv) {
    using namespace std;
    using namespace chess;

    if(argc >= 2) {
        string arg_val = argv[1];
        if(arg_val == "serve") {
            run_server();
        }
        else {
            run_client(arg_val);
        }
    }
    else {
        run_client("localhost:50051");
    }

    return 0;
}

