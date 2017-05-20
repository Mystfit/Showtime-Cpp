#pragma once

#include <vector>
#include <iostream>
#include <stdio.h>
#include <msgpack.hpp>
#include <czmq.h>
#include "ZstPlug.h"

class ZstMessages{
public:
    //Message ID index
    enum Kind{
        OK,
        ERR,
        STAGE_REGISTER_PERFORMER,         //to stage
        STAGE_REGISTER_PERFORMER_ACK,     //from stage
        STAGE_REGISTER_PLUG,            //to stage
		STAGE_DESTROY_PLUG,            //to stage
        STAGE_REGISTER_CONNECTION,      //to stage
        STAGE_REMOVE_ITEM,              //to stage
        STAGE_GRAPH_UPDATES,            //from stage
        STAGE_LIST_PLUGS,
        STAGE_LIST_PLUGS_ACK,
        PERFORMER_UPDATE_PLUG,            //to performers
        PERFORMER_HEARTBEAT               //to stage
    };

	//Special message property enums
	enum GraphItemUpdateType {
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
        ZstPlug::Direction direction;
        MSGPACK_DEFINE(performer, instrument, name, direction);
    };

    struct DestroyPlug {
        ZstPlug::Address address;
        MSGPACK_DEFINE(address);
    };

    struct RegisterConnection{
        ZstPlug::Address from;
        ZstPlug::Address to;
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
        long timestamp;
        MSGPACK_DEFINE(from, timestamp);
    };
    
    struct ListPlugs{
        std::string performer;
        std::string instrument;
        MSGPACK_DEFINE(performer, instrument);
    };
    
    struct ListPlugsAck{
        std::vector<ZstPlug::Address> plugs;
        MSGPACK_DEFINE_ARRAY(plugs);
    };
    

	//---------------------------------------


	//Build a message id from the message ID enum
	static zframe_t * build_message_kind_frame(Kind msg_id) {
		char id[sizeof(Kind)];
		sprintf(id, "%d", (int)msg_id);
		return zframe_from(id);
	}

	//Pops message id from fromt of message
	static Kind pop_message_kind_frame(zmsg_t * msg);

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
	static zmsg_t * build_message(Kind message_id, T message_args, zframe_t * target_identity = NULL) {
		zmsg_t *msg = zmsg_new();
		if (target_identity) {
			zmsg_add(msg, target_identity);
			zmsg_add(msg, zframe_new_empty());
		}

		zmsg_add(msg, build_message_kind_frame(message_id));

		msgpack::sbuffer buf;
		msgpack::pack(buf, message_args);
		zframe_t *payload = zframe_new(buf.data(), buf.size());
		zmsg_add(msg, payload);
		return msg;
	}
};

//Enums for MsgPack
MSGPACK_ADD_ENUM(ZstMessages::GraphItemUpdateType);