#include "../include/cli.hpp"
#include "../include/client.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void cli_loop(KVclient& kvc) {
    std::string line;

    while (true) {
        std::cout << "> ";

        if (!std::getline(std::cin, line)) {
            break;
        }
        if (line == "exit" || line == "quit") {
            break;
        }
        if (line.empty()) {
            continue;
        }
        std::istringstream iss(line);

        std::string command;
        iss >> command;

        if (command == "set") {
            std::string key;
            std::string value;

            if (!(iss >> key >> value)) {
                std::cout << "Usage: set <key> <value>\n";
                continue;
            }

            std::vector<std::string> args{key, value};

            Response res = kvc.send_query(Command::SET, args);
            std::cout << res << '\n';
        }
        else if (command == "get") {
            std::string key;

            if (!(iss >> key)) {
                std::cout << "Usage: get <key>\n";
                continue;
            }

            std::vector<std::string> args{key};

            Response res = kvc.send_query(Command::GET, args);
            std::cout << res << '\n';
        }
        else if (command == "del") {
            std::string key;

            if (!(iss >> key)) {
                std::cout << "Usage: del <key>\n";
                continue;
            }

            std::vector<std::string> args{key};

            Response res = kvc.send_query(Command::DEL, args);
            std::cout << res << '\n';
        }
        else {
            std::cout << "Unknown command\n";
        }
    }
}