#include "../include/server.hpp"

#include <iostream>
#include <csignal>

KVserver kvs{(uint16_t const)1234};


void signal_handler(int sig){
    std::cout << "Interrupt Received, Stopping the server..." << std::endl;
    kvs.stop();
}
int main(int argc , char** argv){
    if(argc != 2){
        std::cout << "Arugments must be <port>" << std::endl;
        return 1;
    }

    signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    kvs.start();

}