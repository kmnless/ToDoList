#include "ToDoServiceImpl.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <csignal>

bool stop_server = false;

void SignalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nStopping server..." << std::endl;
        stop_server = true;
    }
}

int main(int argc, char** argv) {
    std::signal(SIGINT, SignalHandler);

	// maybe set from command line or config
    std::string server_address("0.0.0.0:50051");

    TodoServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

	builder.SetMaxMessageSize(1024 * 1024 * 10); // 10MB

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    while (!stop_server) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Shutting down server..." << std::endl;
    server->Shutdown();

    return 0;
}