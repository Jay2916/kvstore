#include "../include/cli.hpp"
#include "../include/client.hpp"

#include <iostream>

int main(int argc, char** argv){
    if(argc != 3){
        std::cout << "Argument must be <server> <port>" << std::endl;
        return 1;
    }
    std::string serv_addr = argv[1];
    uint16_t serv_port = atoi(argv[2]);
    KVclient kvc{serv_addr, serv_port};
    
    cli_loop(kvc);

    return 0;
}