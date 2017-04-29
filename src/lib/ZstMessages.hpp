#pragma once

#include <tuple>
#include <vector>
#include <iostream>
#include <msgpack.hpp>
#include <stdio.h>


#include "ZstExports.h"
#include "czmq.h"

namespace Showtime{
    using namespace std;

    namespace Messages{
        //Message ID index
        enum MessageIds{
            OK,
            ERROR,
            STAGE_REGISTER_SECTION,         //to stage
            STAGE_REGISTER_PLUG,            //to stage
            STAGE_REGISTER_CONNECTION,      //to stage
            STAGE_REMOVE_ITEM,              //to stage
            STAGE_GRAPH_UPDATES,            //from stage
            SECTION_UPDATE_PLUG,            //to sections
            SECTION_HEARTBEAT               //to stage
        };
        
        //Build a message id from the message ID enum
        static zframe_t * build_message_id_frame(MessageIds msg_id){
            char id[1];
            sprintf(id, "%d", (int)msg_id);
            return zframe_from(id);
        }
        
        //Unpack a msgpack stream to a message struct
        template <typename T>
        static T unpack_message_struct(zmsg_t * msg) {
            T rvec;
            msgpack::object_handle result;
            
            string msg_payload = zmsg_popstr(msg);
            unpack(result, msg_payload.c_str(), msg_payload.size());
            result.get().convert(rvec);
            return rvec;
        }
        
        //--
        //Section registration messages
        //--
        struct RegisterSection{
            string name;
            string endpoint;
            MSGPACK_DEFINE(name, endpoint);
        };
        
        static zmsg_t* build_register_section_message(RegisterSection args){
            //Pack payload
            msgpack::sbuffer sbuf;
            msgpack::pack(sbuf, args);
            
            //Build message
            zmsg_t *msg = zmsg_new();
            zmsg_add(msg, build_message_id_frame(MessageIds::STAGE_REGISTER_SECTION));
            zmsg_addstr(msg, sbuf.data());
            return msg;
        };
    
        
        //--
        //Plug registration messages
        //--
        enum class PlugDirection{
            INPUT,
            OUTPUT
        };
        
        struct RegisterPlug{
            string address;
            int primitive;
            PlugDirection direction;
            MSGPACK_DEFINE(address, primitive, direction);
        };
        
        static zmsg_t* build_register_plug_message(RegisterPlug args){
            //Pack payload
            msgpack::sbuffer sbuf;
            msgpack::pack(sbuf, args);
            
            //Build message
            zmsg_t *msg = zmsg_new();
            zmsg_add(msg, build_message_id_frame(MessageIds::STAGE_REGISTER_PLUG));
            zmsg_addstr(msg, sbuf.data());
            return msg;
        };
        

        //--
        //Connection registration messages
        //--
        struct RegisterConnection{
            string from;
            string to;
            MSGPACK_DEFINE(from, to);
        };
        
        static zmsg_t* build_register_connection_message(RegisterConnection args){
            //Pack payload
            msgpack::sbuffer sbuf;
            msgpack::pack(sbuf, args);
            
            //Build message
            zmsg_t *msg = zmsg_new();
            zmsg_add(msg, build_message_id_frame(MessageIds::STAGE_REGISTER_CONNECTION));
            zmsg_addstr(msg, sbuf.data());
            return msg;
        };
        
        
        //--
        //Graph update messages
        //--
        enum GraphItemUpdateType{
            ARRIVING,
            LEAVING
        };

        struct GraphUpdates{
            vector<tuple<int, GraphItemUpdateType> > updates;
            MSGPACK_DEFINE_ARRAY(updates);
        };
        
        static zmsg_t* build_graph_update_message(GraphUpdates args){
            //Pack payload
            msgpack::sbuffer sbuf;
            msgpack::pack(sbuf, args);
            
            //Build message
            zmsg_t *msg = zmsg_new();
            zmsg_add(msg, build_message_id_frame(MessageIds::STAGE_GRAPH_UPDATES));
            zmsg_addstr(msg, sbuf.data());
            return msg;
        };
        
        
        //--
        //Plug output messages
        //--
        struct PlugOutput{
            string from;
            string value;
            MSGPACK_DEFINE(from, value);
        };
        
        static zmsg_t* build_plug_output_message(GraphUpdates args){
            //Pack payload
            msgpack::sbuffer sbuf;
            msgpack::pack(sbuf, args);
            
            //Build message
            zmsg_t *msg = zmsg_new();
            zmsg_add(msg, build_message_id_frame(MessageIds::SECTION_UPDATE_PLUG));
            zmsg_addstr(msg, sbuf.data());
            return msg;
        };

        
        //--
        //Heartbeat messages
        //--
        struct Heartbeat {
            string from;
            uint timestamp;
            MSGPACK_DEFINE(from, timestamp);
        };
        
        static zmsg_t* build_heartbeat_message(Heartbeat args){
            //Pack payload
            msgpack::sbuffer sbuf;
            msgpack::pack(sbuf, args);
            
            //Build message
            zmsg_t *msg = zmsg_new();
            zframe_t *id = build_message_id_frame(MessageIds::SECTION_HEARTBEAT);
            zmsg_add(msg, id);
            zmsg_addstr(msg, sbuf.data());
            return msg;
        }
    }
}

//Enums for MsgPack
MSGPACK_ADD_ENUM(Showtime::Messages::PlugDirection);
MSGPACK_ADD_ENUM(Showtime::Messages::GraphItemUpdateType);
