#include "bedrock_echo.pb.h"
#include "bedrock_echo.grpc.pb.h"
#include "echo_service_impl.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/create_channel.h>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iostream>

using namespace bedrock::echo;

int main() {
    const std::string server_address("localhost:50051");
    
    // Create Echo service implementation
    EchoServiceImpl service;
    
    // Build and start server
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        std::cerr << "Failed to start server on " << server_address << std::endl;
        return 1;
    }
    
    // Start server in background thread
    std::thread server_thread([&server]() {
        server->Wait();
    });
    
    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Create client channel
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    auto stub = EchoService::NewStub(channel);
    
    // Prepare request
    EchoRequest request;
    request.set_message("hello");
    
    // Call Echo RPC
    EchoReply reply;
    grpc::ClientContext context;
    grpc::Status status = stub->Echo(&context, request, &reply);
    
    // Verify response
    if (!status.ok()) {
        std::cerr << "Echo RPC failed: " << status.error_message() << std::endl;
        server->Shutdown();
        server_thread.join();
        return 1;
    }
    
    if (reply.message() != request.message()) {
        std::cerr << "Echo response mismatch. Expected: " << request.message()
                  << ", Got: " << reply.message() << std::endl;
        server->Shutdown();
        server_thread.join();
        return 1;
    }
    
    // Shutdown server
    server->Shutdown();
    server_thread.join();
    
    return 0;
}

