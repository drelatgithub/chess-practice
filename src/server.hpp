#ifndef CHESS_SERVER_HPP
#define CHESS_SERVER_HPP

#include <cstdint>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "chess/game.hpp"
#include "proto/helloworld.grpc.pb.h"

namespace chess {

struct ServedGame {
    // raw data
    GameHistory game_history;

    // client identity
    std::uint64_t player_ids[2] {}; // white, black
};


// Logic and data behind the server's behavior.
struct ChessServiceImpl {

    // Class encompasing the state and logic needed to serve a request.
    struct CallSession {
        enum class Event { connect, read, write, finish, client_finish, client_disconnect };

        struct State {
            struct ReplyItem {
                chess_proto::ChessReply rep;
                bool finish_only = false;
            };

            bool finished = false;
            bool can_write = true;
            chess_proto::ChessRequest req_cache;
            std::deque< ReplyItem > rep_queue;

            grpc::ServerContext ctx;
            grpc::ServerAsyncReaderWriter<chess_proto::ChessReply, chess_proto::ChessRequest> stream;

            State() : stream(&ctx) {}
        };

        std::weak_ptr<State> p_state;
        Event event = Event::connect;
    };

    // Game data
    ServedGame served_game;

    // All call sessions
    bool running = true;

    // grpc related stuff
    chess_proto::ChessServer::AsyncService service;
    std::unique_ptr<grpc::ServerCompletionQueue> cq;
    std::unique_ptr<grpc::Server> server;

    ~ChessServiceImpl() {
        server->Shutdown();
        // Always shutdown the completion queue after the server.
        cq->Shutdown();
    }

    void run(std::string server_address) {
        using namespace std;
        using namespace grpc;

        EnableDefaultHealthCheckService(true);
        reflection::InitProtoReflectionServerBuilderPlugin();
        ServerBuilder builder;
        // Listen on the given address without any authentication mechanism.
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        // Register "service" as the instance through which we'll communicate with
        // clients. In this case it corresponds to an *synchronous* service.
        builder.RegisterService(&service);
        cq = builder.AddCompletionQueue();
        // Finally assemble the server.
        server = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        // Proceed to server main loop.
        main_loop();
    }

    void main_loop() {
        using namespace std;

        // Tag management.
        uint64_t next_tag = 1;
        unordered_map< uint64_t, CallSession > tags;
        const auto add_tag = [&](CallSession cs) {
            auto cur_tag = next_tag++;
            tags.insert({ cur_tag, cs });
            return (void*) cur_tag;
        };
        const auto get_tag = [&](void* tag) {
            auto tag_it = tags.find((uint64_t) tag);
            if(tag_it == tags.end()) {
                cout << "Error: tag " << tag_it->first << " is not found in tags.";
                throw runtime_error("Invalid tag");
            }
            else {
                auto cs = tag_it->second;
                tags.erase(tag_it);
                return cs;
            }
        };

        // Make new session.
        const auto make_session_state = [&, this]() {
            auto state = make_shared<CallSession::State>();
            service.RequestCommand(&state->ctx, &state->stream, cq.get(), cq.get(), add_tag({ state, CallSession::Event::connect }));
            state->ctx.AsyncNotifyWhenDone(add_tag({ state, CallSession::Event::client_disconnect }));
            return state;
        };

        // Spawn a new session to serve new clients.
        auto new_state = make_session_state();
        // Running sessions are stored here.
        unordered_set< shared_ptr< CallSession::State > > states;


        // Function to read from client.
        const auto session_async_read = [&](weak_ptr<CallSession::State> p_state) {
            auto state = p_state.lock();
            if(state) {
                state->stream.Read(&state->req_cache, add_tag({ p_state, CallSession::Event::read }));
            }
            else {
                cout << "Warning: session has been deleted when trying to read." << endl;
            }
        };

        // Write next reply in queue to stream.
        const auto async_write_next_reply = [&](weak_ptr<CallSession::State> p_state) {
            auto state = p_state.lock();
            if(state) {
                if(state->can_write && !state->rep_queue.empty()) {
                    auto rep = move(state->rep_queue.front());
                    state->rep_queue.pop_front();
                    state->can_write = false;
                    if(rep.finish_only) {
                        state->stream.Finish(grpc::Status::OK, add_tag({ state, CallSession::Event::client_finish }));
                    }
                    else {
                        state->stream.Write(rep.rep, add_tag({ state, CallSession::Event::write }));
                    }
                }
            }
            else {
                cout << "Warning: session has been deleted when trying to write next reply." << endl;
            }
        };

        // Function that handles received message and writes to client.
        const auto session_gen_respond = [&](weak_ptr<CallSession::State> p_state) {
            auto state = p_state.lock();
            if(state) {
                auto [ msg, broadcast, repeated_msg, client_finish ] = chess_respond(state->req_cache);

                if(broadcast) {
                    for(const auto& each_state : states) {
                        if(each_state != state) {
                            chess_proto::ChessReply rep_repeated;
                            rep_repeated.set_message(repeated_msg);
                            each_state->rep_queue.push_back({ move(rep_repeated), false });
                        }
                        chess_proto::ChessReply rep;
                        rep.set_message(msg);
                        each_state->rep_queue.push_back({ move(rep), false });
                        async_write_next_reply(each_state);
                    }
                }
                else {
                    chess_proto::ChessReply rep;
                    rep.set_message(msg);
                    state->rep_queue.push_back({ move(rep), false });
                    async_write_next_reply(state);
                }

                if(client_finish) {
                    // Add empty finish action.
                    state->rep_queue.push_back({ chess_proto::ChessReply{}, true });
                }
            }
            else {
                cout << "Error: session has been deleted when trying to generate response." << endl;
            }
        };

        const auto finish_session_state = [&](weak_ptr<CallSession::State> p_state) {
            auto state = p_state.lock();
            if(state) {
                state->rep_queue.push_back({ chess_proto::ChessReply{}, true });
                async_write_next_reply(state);
            }
            else {
                cout << "Error: session state has already been deleted on finishing." << endl;
            }
        };

        const auto remove_session_state = [&](weak_ptr<CallSession::State> p_state) {
            auto state = p_state.lock();
            if(state) {
                states.erase(state);
            }
            else {
                cout << "Error: session state has already been deleted on session deletion." << endl;
            }
        };


        void* tag;  // uniquely identifies a request.
        bool ok;
        while (running) {
            // Block waiting to read the next event from the completion queue. The
            // event is uniquely identified by its tag, which in this case is the
            // memory address of a CallSession instance.
            // The return value of Next should always be checked. This return value
            // tells us whether there is any kind of event or cq_ is shutting down.
            GPR_ASSERT(cq->Next(&tag, &ok));
            if (!ok) {
                cout << "[Queue] Warning: invalid completion queue item." << endl;
                continue;
            }
            auto session = get_tag(tag);

            switch(session.event) {
                using enum CallSession::Event;

                case connect:
                    cout << "[Queue] Client connected." << endl;
                    // Create a new session state for new connections.
                    states.insert(move(new_state));
                    new_state = make_session_state();

                    // Read from stream of this session.
                    session_async_read(session.p_state);
                    break;

                case read:
                    cout << "[Queue] Received client message." << endl;

                    // Process and generate replies to message.
                    session_gen_respond(session.p_state);

                    // Continue reading.
                    session_async_read(session.p_state);
                    break;

                case write:
                    cout << "[Queue] Message sent to client." << endl;

                    // Restore can_write state.
                    {
                        auto state = session.p_state.lock();
                        if(state) {
                            state->can_write = true;
                            // Continue writing.
                            async_write_next_reply(session.p_state);
                        }
                        else {
                            cout << "Warning: session closed when writing completes." << endl;
                        }
                    }

                    break;

                case finish:
                    // Currently never used.
                    cout << "[Queue] Server finished." << endl;
                    running = false;
                    break;

                case client_finish:
                    cout << "[Queue] Client finished." << endl;

                    // Remove session states.
                    remove_session_state(session.p_state);
                    break;

                case client_disconnect:
                    cout << "[Queue] Client disconnected." << endl;

                    // Remove session states.
                    finish_session_state(session.p_state);
                    break;

                default:
                    cout << "[Queue] Error: unknown session event " << underlying(session.event) << endl;
            }
        }
    }

    // Returns the message, whether the message should be broadcasted, the repeated player message, and whether the client finishes.
    // The repeated player message will not be replied to the sending player.
    std::tuple< std::string, bool, std::string, bool > chess_respond(const chess_proto::ChessRequest& req) {
        using namespace std;

        ostringstream oss_message;
        ostringstream oss_repeated;
        bool broadcast = false;
        bool client_finish = false;
        int who = 0; // 0: not ready, 1: white, 2: black

        const auto server_log_game_players = [&, this] {
            cout << "[Game] Players: 白" << (served_game.player_ids[0] ? "○" : "×") << " 黑" << (served_game.player_ids[1] ? "○" : "×") << endl;
        };
        const auto print_game_status = [&, this] {
            served_game.game_history.ptr_current_item()->game_state.pretty_print_to(oss_message);
            oss_message << endl;
        };

        // General check
        if(req.id() == 0) {
            oss_message << "Error: invalid player id: " << req.id() << endl;
        }
        else if(req.command() == "init") {
            if(served_game.player_ids[0] == 0) {
                served_game.player_ids[0] = req.id();
                oss_message << "Player " << req.id() << " registered as white." << endl;
                if(served_game.player_ids[1]) {
                    oss_message << "Game starts.\n";
                    print_game_status();
                }
                broadcast = true;
            }
            else if(served_game.player_ids[0] == req.id()) {
                oss_message << "Error: player " << req.id() << " is already registered as white." << endl;
            }
            else if(served_game.player_ids[1] == 0) {
                served_game.player_ids[1] = req.id();
                oss_message << "Player " << req.id() << " registered as black." << endl;
                if(served_game.player_ids[0]) {
                    oss_message << "Game starts.\n";
                    print_game_status();
                }
                broadcast = true;
            }
            else if(served_game.player_ids[1] == req.id()) {
                oss_message << "Error: player " << req.id() << " is already registered as black." << endl;
            }
            else {
                oss_message << "Error: cannot register new player." << endl;
            }
            server_log_game_players();
        }
        else if(req.command() == "exit") {
            oss_message << "Player " << req.id() << " left the game." << endl;
            for(auto& id : served_game.player_ids) {
                if(id == req.id()) {
                    broadcast = true;
                    id = 0;
                }
            }
            client_finish = true;
            server_log_game_players();
        }
        else if(served_game.player_ids[0] == 0 || served_game.player_ids[1] == 0) {
            oss_message << "Error: waiting for other players." << endl;
        }
        else if(req.id() == served_game.player_ids[0]) {
            who = 1; // white
        }
        else if(req.id() == served_game.player_ids[1]) {
            who = 2; // black
        }

        if(who) {
            oss_repeated << (who == 2 ? "black> " : "white> ") << req.command() << endl;
            broadcast = server_game_step(served_game.game_history, who == 2, req.command(), oss_message);
        }

        // Server debug.
        cout << "Player " << req.id() << "> " << req.command() << endl;
        cout << oss_message.str();

        return { oss_message.str(), broadcast, oss_repeated.str(), client_finish };
    }

};

inline void run_server() {
    std::string server_address("0.0.0.0:50051");

    ChessServiceImpl server;
    server.run(server_address);
}

} // namespace chess

#endif
