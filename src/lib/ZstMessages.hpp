#pragma once

#include <tuple>
#include <vector>
#include <iostream>
#include <msgpack.hpp>
#include <stdio.h>

#include "ZstPlug.h"
#include "ZstExports.h"
#include "czmq.h"


//Graph strucutre
struct ZstPlugAddress{
    std::string performer;
    std::string instrument;
    std::string name;
    MSGPACK_DEFINE(performer, instrument, name);
};

namespace ZstMessages{
    //Message ID index
    enum MessageIds{
        OK,
        ERR,
        STAGE_REGISTER_PERFORMER,         //to stage
        STAGE_REGISTER_PERFORMER_ACK,     //from stage
        STAGE_REGISTER_PLUG,            //to stage
        STAGE_REGISTER_CONNECTION,      //to stage
        STAGE_REMOVE_ITEM,              //to stage
        STAGE_GRAPH_UPDATES,            //from stage
        STAGE_LIST_PLUGS,
        STAGE_LIST_PLUGS_ACK,
        PERFORMER_UPDATE_PLUG,            //to performers
        PERFORMER_HEARTBEAT               //to stage
    };
    
    //Build a message id from the message ID enum
    static zframe_t * build_message_id_frame(MessageIds msg_id){
        char id[2];
        sprintf(id, "%d", (int)msg_id);
        return zframe_from(id);
    }
    
    //Pops message id from fromt of message
    static MessageIds pop_message_id(zmsg_t * msg){
        return (ZstMessages::MessageIds)atoi(zmsg_popstr(msg));
    }
    
    //Unpack a msgpack stream to a message struct
    template <typename T>
    static T unpack_message_struct(zmsg_t * msg) {
        T converted_struct;
        msgpack::object_handle result;
        
        zframe_t * payload = zmsg_pop(msg);
        
        unpack(result, (char*)zframe_data(payload), zframe_size(payload));
        result.get().convert(converted_struct);
        return converted_struct;
    }
    
    //Creates a msgpacked message
    template <typename T>
    static zmsg_t * build_message(MessageIds message_id, T message_args, zframe_t * target_identity = NULL){
        zmsg_t *msg = zmsg_new();
        if(target_identity){
            zmsg_add(msg, target_identity);
            zmsg_add(msg, zframe_new_empty());
        }
        
        zmsg_add(msg, build_message_id_frame(message_id));
        
        msgpack::sbuffer buf;
        msgpack::pack(buf, message_args);
        zframe_t *payload = zframe_new(buf.data(), buf.size());
        zmsg_add(msg, payload);
        return msg;
    }
    
    
    //Special message property enums
    enum GraphItemUpdateType{
        ARRIVING,
        LEAVING
    };

    
    //Message structs
    //---------------
    struct OKAck{
        bool empty;
        MSGPACK_DEFINE(empty);
    };
    
    struct RegisterPerformer{
        std::string name;
        std::string endpoint;
        MSGPACK_DEFINE(name, endpoint);
    };
    
    struct RegisterPerformerAck{
        int assigned_stage_port;
        MSGPACK_DEFINE(assigned_stage_port);
    };
    
    struct RegisterPlug{
        std::string performer;
        std::string instrument;
        std::string name;
        int primitive;
        ZstPlug::PlugDirection direction;
        MSGPACK_DEFINE(performer, instrument, name, primitive, direction);
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
    
    struct ListPlugs{
        std::string performer;
        std::string instrument;
        MSGPACK_DEFINE(performer, instrument);
    };
    
    struct ListPlugsAck{
        std::vector<ZstPlugAddress> plugs;
        MSGPACK_DEFINE_ARRAY(plugs);
    };
    
    struct ConnectInputPlugToOutput{
        
    };
    
    struct ConnectInputPlugToOutputAck{
        
    };
}


//Enums for MsgPack
MSGPACK_ADD_ENUM(ZstPlug::PlugDirection);
MSGPACK_ADD_ENUM(ZstMessages::GraphItemUpdateType);
