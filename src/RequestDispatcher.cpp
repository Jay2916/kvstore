#include "../include/RequestDispatcher.hpp"
#include <optional>

RequestDispatcher::RequestDispatcher(StorageEngine store)
    :store(store){}

Response RequestDispatcher::dispatch(Query query){
    Response res = {Status::RES_ERR, {}};
    switch(query.cmd){
        case Command::GET:
            auto value = store.get(query.args[0]);
            if(value != std::nullopt){
                res.status = Status::RES_OK;
                res.data = value;
            }
            else{
                res.status = RES_NX;
            }
            break;
        case Command::SET:
            store.set(query.args[0], query.args[1]);
            res.status = Status::RES_OK
            break;
        case Command::DEL:
            if(store.del(query.args[0])){
                res.status = Status::RES_OK;
            }
            else{
                res.status = Status::RES_NX;
            }
            break;
    }
    return res;
}