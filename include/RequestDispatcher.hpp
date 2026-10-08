#pragma once
#include "../include/kvprotocol.hpp"
#include "../include/StorageEngine.hpp"

class RequestDispatcher{
public:
    RequestDispatcher(StorageEngine& store);
    Response dispatch(Query& query);
private:
    StorageEngine& store;
     
};