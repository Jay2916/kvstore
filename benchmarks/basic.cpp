//#define NO_DEBUG 1

#include "../include/client.hpp"
#include "../CLIUtils/CLI11/include/CLI/CLI.hpp"
#include "../include/kvprotocol.hpp"

#include <time.h>
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <assert.h>
#define MAX_IN 100000

void print_timpspec(struct timespec ts){
    std::cout << "Time: " << ts.tv_sec << "sec " << ts.tv_nsec/1000 << "ms" <<std::endl;
}
int main(int argc, char* argv[]){
    struct timespec start, end;
    CLI::App app{"KVstore benchmark"};

    std::string serv_addr = "localhost";
    uint16_t serv_port = 1234;
    std::string str = "";
    str.resize(MAX_IN);

    app.add_option("-p, --port", serv_port,"server port");
    app.add_option("-a, --server-address", serv_addr,"server address");
    app.add_option("--in", str,"sequence of operations");

    CLI11_PARSE(app, argc, argv);
    
    if(str.empty()) return 1;

    std::vector<std::string> get_inp = {"hiiiiiiiiiiiiiiiiiiii"};
    std::vector<std::string> del_inp = {"hiiiiiiiiiiiiiiiiiiii"};
    std::vector<std::string> set_inp = {"hiiiiiiiiiiiiiiiiiiii", "hello"};

    KVclient kvc{serv_addr, serv_port};

    clock_gettime(CLOCK_MONOTONIC, &start);

    for(char c : str){
        Response res;
        switch(c){
            case 'G':
                res = kvc.send_query(Command::GET, get_inp);
                break;
            case 'S':
                res = kvc.send_query(Command::SET, set_inp);
                break;
            case 'D':
                res = kvc.send_query(Command::DEL, del_inp);
                break;
            default:
                assert(1);
        }
        std::cout << res << std::endl;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    struct timespec res = {
        .tv_sec = end.tv_sec - start.tv_sec,
        .tv_nsec = end.tv_nsec - start.tv_nsec
    };
    print_timpspec(res);
    return 0;
}