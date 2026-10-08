#include "../include/RequestDispatcher.hpp"
#include "../include/HashTableStorage.hpp"
#include "../include/kvprotocol.hpp"
#include "../include/helper.hpp"
#include <iostream>
#include <vector>

using namespace std;
int main(){

    HashTableStorage store;
    RequestDispatcher rd{store};
    std::vector<Query> queries = {
        // GET
        {Command::GET, {toBytes("key")}},
        {Command::GET, {toBytes("")}},
        {Command::GET, {toBytes("hello")}},
        {Command::GET, {toBytes("long_key_12345")}},

        // SET
        {Command::SET, {toBytes("key"), toBytes("value")}},
        {Command::SET, {toBytes(""), toBytes("value")}},
        {Command::SET, {toBytes("key"), toBytes("")}},
        {Command::SET, {toBytes(""), toBytes("")}},
        {Command::SET, {toBytes("username"), toBytes("jay")}},

        // DEL
        {Command::DEL, {toBytes("key")}},
        {Command::DEL, {toBytes("")}},
        {Command::DEL, {toBytes("long_key_12345")}},

        // Invalid argument counts
        {Command::GET, {}},
        {Command::GET, {toBytes("key"), toBytes("extra")}},
        {Command::SET, {}},
        {Command::SET, {toBytes("key")}},
        {Command::SET, {toBytes("key"), toBytes("value"), toBytes("extra")}},
        {Command::DEL, {}},
        {Command::GET, {toBytes("key")}},

        // Invalid command
        {static_cast<Command>(255), {}}
    };

    for(Query& q : queries){
        cout << q;
        Response res = rd.dispatch(q);
        cout << res << endl << endl;
    }
    
    return 0;
}