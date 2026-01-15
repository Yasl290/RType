#include "Server.hpp"
#include "protocol/Protocol.hpp"
#include <iostream>
#include <csignal>

std::unique_ptr<RType::Server::Server> g_server;

void signalHandler(int signal)
{
    (void)signal;
    std::cout << "\nShutting down server..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

int main(int argc, char* argv[])
{
    uint16_t port = 4242;
    
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        g_server = std::make_unique<RType::Server::Server>(port);
        g_server->start();
        
        std::cout << "Server running on port " << port << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        std::cout << "sizeof(PacketHeader) = " 
                  << sizeof(RType::Protocol::PacketHeader) 
                  << " bytes" << std::endl;
        
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
