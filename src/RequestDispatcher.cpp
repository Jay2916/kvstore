#include "../include/RequestDispatcher.hpp"
#include <optional>
#include <assert.h>
#include "../include/helper.hpp"

RequestDispatcher::RequestDispatcher(StorageEngine& store)
    :store(store){}

Response RequestDispatcher::dispatch(Query& query){
    Response res = {Status::RES_ERR, {}};
    switch(query.cmd){
        case Command::GET:{
            if(query.args.size() < 1){
                alert_msg("GET: insufficient arguments");
                return {Status::RES_ERR, {}};
            }
            auto value = store.get(query.args[0]);
            if(value){
                res.status = Status::RES_OK;
                res.data = *value;
            }
            else{
                res.status = Status::RES_NX;
            }
            break;
        }
        case Command::SET:{
            if(query.args.size() < 2){
                alert_msg("SET: insufficient arguments");
                return {Status::RES_ERR, {}};
            }
            store.set(query.args[0], query.args[1]);
            res.status = Status::RES_OK;
            break;
        }
        case Command::DEL:{
            if(query.args.size() < 1){
                alert_msg("DEL: insufficient arguments");
                return {Status::RES_ERR, {}};
            }
            if(store.del(query.args[0])){
                res.status = Status::RES_OK;
            }
            else{
                res.status = Status::RES_NX;
            }
            break;
        }
        default:
            alert_msg("trying to dispatch invalid command");

    }
    return res;
}