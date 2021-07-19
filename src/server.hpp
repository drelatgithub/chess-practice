#ifndef CHESS_SERVER_HPP
#define CHESS_SERVER_HPP

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "chess/game.hpp"
#include "proto/helloworld.grpc.pb.h"

namespace chess {

struct ServedGame {
    // raw data
    GameHistory game_history;

    // synchronization
    std::mutex me_data;
    std::condition_variable cv_progress;

    // client identity
    std::uint64_t black_player_id = 0;
    std::uint64_t white_player_id = 0;
};


// Logic and data behind the server's behavior.
struct ChessServiceImpl final : public chess_proto::ChessServer::Service {

    ServedGame served_game;

    grpc::Status Command(grpc::ServerContext* context, grpc::ServerReaderWriter<chess_proto::ChessReply, chess_proto::ChessRequest>* stream) override {
        using namespace std;
        using namespace chess_proto;
        using namespace grpc;

        ChessRequest req;
        while(stream->Read(&req)) {
            ostringstream oss_message;
            bool progressed = false;
            {
                std::unique_lock<std::mutex> lock(served_game.me_data);
                progressed = server_game_step(served_game.game_history, true /*context black?*/, req.command(), oss_message);
            }
            cout << oss_message.str();
            if(progressed) {
                // good. inform both sides the updated board.
            }
            else {
                // return message to sender.
                ChessReply rep;
                rep.set_message(oss_message.str());
                stream->Write(rep);
            }
        }
        return Status::OK;
    }
};

void run_server() {
    std::string server_address("0.0.0.0:50051");
    ChessServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    auto server = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

} // namespace chess

#endif
