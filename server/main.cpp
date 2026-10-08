#include "../include/server.hpp"
#include "../include/RequestDispatcher.hpp"
#include "../include/HashTableStorage.hpp"
#include <iostream>
#include <csignal>

KVserver* kvs;


void signal_handler(int sig){
    (void)sig;
    std::cout << "Interrupt Received, Stopping the server..." << std::endl;
    kvs->stop();
}
int main(int argc , char** argv){
    if(argc != 2){
    std::cout << "Arugments must be <port>" << std::endl;
    return 1;
    }

    HashTableStorage store{};
    RequestDispatcher rd{store};
    KVserver kvserver{(uint16_t)atoi(argv[1]), rd};
    kvs = &kvserver;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    kvserver.start();

}