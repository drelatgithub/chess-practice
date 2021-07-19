#ifndef CHESS_CLIENT_HPP
#define CHESS_CLIENT_HPP

#include <atomic>
#include <cstdint>
#include <memory> // shared_ptr, unique_ptr
#include <thread>
#include <utility> // move

#include <grpcpp/grpcpp.h>

#include "proto/helloworld.grpc.pb.h"
#include "utility.hpp"

namespace chess {

struct ChessClient {
    using PtrRW = std::shared_ptr<grpc::ClientReaderWriter<chess_proto::ChessRequest, chess_proto::ChessReply> >;

    std::uint64_t id = 0;

    std::unique_ptr<chess_proto::ChessServer::Stub> stub;

    // synchronization

    ChessClient(std::shared_ptr<grpc::Channel> channel) :
        stub(chess_proto::ChessServer::NewStub(channel)),
        id(std::uniform_int_distribution<std::uint64_t>(1)(rand_gen))
    {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    void send_command(PtrRW stream, std::string command) {

        // grpc::ClientContext context;

        // PtrRW stream(stub_->RouteChat(&context));

        chess_proto::ChessRequest req;
        req.set_id(id);
        req.set_command(std::move(command));

        stream->Write(req);
    }
};

inline void run_client(std::string target) {
    using namespace std;
    using namespace grpc;
    using namespace chess_proto;

    ChessClient client(CreateChannel(std::move(target), InsecureChannelCredentials()));
    ClientContext context;

    ChessClient::PtrRW stream(client.stub->Command(&context));
    std::atomic_bool read_finish { false };

    // Create a thread receiving message.
    std::thread reader([stream, &read_finish]() {
        ChessReply rep;
        while (stream->Read(&rep) && !read_finish) {
            std::cout << rep.message();
        }
        std::cout<< "Server receive finished"<<std::endl;
    });

    // Send initialization to server.
    client.send_command(stream, "init");

    // Start playing.
    while(true) {
        string command;
        getline(cin, command);

        client.send_command(stream, command);
        if(command == "exit") {
            break;
        }
    }

    // Finishing up.
    read_finish = true;
    reader.join();
    Status status = stream->Finish();
    if(!status.ok()) {
        cout << "RPC failed." << endl;
    }
}

} // namespace chess

#endif
