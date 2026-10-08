#include "../include/cli.hpp"
#include "../include/client.hpp"
#include "../include/helper.hpp"

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
        Query query = {};
        if (command == "set") {
            std::string key;
            std::string value;

            if (!(iss >> key >> value)) {
                std::cout << "Usage: set <key> <value>\n";
                continue;
            }

            query.cmd = Command::SET;
            query.args.push_back(toBytes(key));
            query.args.push_back(toBytes(value));

        }
        else if (command == "get") {
            std::string key;

            if (!(iss >> key)) {
                std::cout << "Usage: get <key>\n";
                continue;
            }

            query.cmd = Command::GET;
            query.args.push_back(toBytes(key));

        }
        else if (command == "del") {
            std::string key;

            if (!(iss >> key)) {
                std::cout << "Usage: del <key>\n";
                continue;
            }

            query.cmd = Command::DEL;
            query.args.push_back(toBytes(key));

        }
        else {
            std::cout << "Unknown command\n";
        }
        Response res = kvc.send_query(query);
        std::cout << res << '\n';
    }
}