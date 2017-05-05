#pragma once

#include <tuple>
#include <vector>
#include <iostream>
#include <msgpack.hpp>
#include <stdio.h>


#include "ZstExports.h"
#include "czmq.h"

namespace ZstMessages{
    //Message ID index
    enum MessageIds{
        OK,
        ERR,
        STAGE_REGISTER_SECTION,         //to stage
        STAGE_REGISTER_SECTION_ACK,     //from stage
        STAGE_REGISTER_PLUG,            //to stage
        STAGE_REGISTER_CONNECTION,      //to stage
        STAGE_REMOVE_ITEM,              //to stage
        STAGE_GRAPH_UPDATES,            //from stage
        SECTION_UPDATE_PLUG,            //to sections
        SECTION_HEARTBEAT               //to stage
    };
    
    //Build a message id from the message ID enum
    static zframe_t * build_message_id_frame(MessageIds msg_id){
        char id[2];
        sprintf(id, "%d", (int)msg_id);
        return zframe_from(id);
    }
    
    static MessageIds pop_message_id(zmsg_t * msg){
        return (ZstMessages::MessageIds)atoi(zmsg_popstr(msg));
    }
    
    //Pack a msgpack struct to a buffer
    template <typename T>
    static msgpack::sbuffer pack_message_struct(T msgdata){
        msgpack::sbuffer sbuf;
        msgpack::pack(sbuf, msgdata);
        return sbuf;
    }
    
    
    //Unpack a msgpack stream to a message struct
    template <typename T>
    static T unpack_message_struct(zmsg_t * msg) {
        T rvec;
        msgpack::object_handle result;
        
        std::string msg_payload = zmsg_popstr(msg);
        unpack(result, msg_payload.c_str(), msg_payload.size());
        result.get().convert(rvec);
        return rvec;
    }

    template <typename T>
    static zmsg_t * build_message(MessageIds message_id, T message_args, zframe_t * target_identity = NULL){
        zmsg_t *msg = zmsg_new();
        if(target_identity){
            zmsg_add(msg, target_identity);
            zmsg_add(msg, zframe_new_empty());
        }
        
        zmsg_add(msg, build_message_id_frame(message_id));
        zmsg_addstr(msg, ZstMessages::pack_message_struct<T>(message_args).data());
        return msg;
    }
    
    
    //Special message property enums
    
    enum class PlugDirection{
        INPUT,
        OUTPUT
    };
    
    enum GraphItemUpdateType{
        ARRIVING,
        LEAVING
    };

    
    //Message structs
    //---------------
    struct OK{
        bool empty;
        MSGPACK_DEFINE(empty);
    };
    
    struct RegisterSection{
        std::string name;
        std::string endpoint;
        MSGPACK_DEFINE(name, endpoint);
    };
    
    struct RegisterSectionAck{
        int assigned_stage_port;
        MSGPACK_DEFINE(assigned_stage_port);
    };
    
    struct RegisterPlug{
        std::string address;
        int primitive;
        PlugDirection direction;
        MSGPACK_DEFINE(address, primitive, direction);
    };

    struct RegisterConnection{
        std::string from;
        std::string to;
        MSGPACK_DEFINE(from, to);
    };
    
    struct GraphUpdates{
        std::vector<std::tuple<int, GraphItemUpdateType> > updates;
        MSGPACK_DEFINE_ARRAY(updates);
    };

    struct PlugOutput{
        std::string from;
        std::string value;
        MSGPACK_DEFINE(from, value);
    };
    
    struct Heartbeat {
        std::string from;
        uint timestamp;
        MSGPACK_DEFINE(from, timestamp);
    };
}


//Enums for MsgPack
MSGPACK_ADD_ENUM(ZstMessages::PlugDirection);
MSGPACK_ADD_ENUM(ZstMessages::GraphItemUpdateType);
