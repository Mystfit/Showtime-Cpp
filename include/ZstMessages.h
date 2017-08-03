#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <stdio.h>
#include <msgpack.hpp>
#include <czmq.h>
#include "ZstEvent.h"
#include "ZstURIWire.h"
#include "ZstEventWire.h"
#include "ZstPlug.h"

class ZstMessages{
public:
    //Message ID index
    enum Kind{
        SIGNAL,
		
		//Endpoint registration
		STAGE_CREATE_ENDPOINT,
		STAGE_CREATE_ENDPOINT_ACK,
		
		//Performer registration
        STAGE_CREATE_PERFORMER,
		STAGE_DESTROY_PERFORMER,

		//Entity registration
		STAGE_REGISTER_ENTITY_TYPE,
		STAGE_CREATE_ENTITY,
		STAGE_DESTROY_ENTITY,
		
		//Plug registration
        STAGE_CREATE_PLUG,
		STAGE_DESTROY_PLUG,
		
		//Connection registration
        STAGE_CREATE_CABLE,
		STAGE_DESTROY_CABLE,
		
		//Misc messages
        STAGE_UPDATE,
        ENDPOINT_HEARTBEAT,

		//P2P endpoint connection requests
		PERFORMER_REGISTER_CONNECTION
    };

	enum Signal {
		ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST = -7,
		ERR_STAGE_BAD_CABLE_CONNECT_REQUEST = -6,
		ERR_STAGE_ENDPOINT_NOT_FOUND = -5,
		ERR_STAGE_ENDPOINT_ALREADY_EXISTS = -4,
		ERR_STAGE_ENTITY_NOT_FOUND = -3,
		ERR_STAGE_ENTITY_ALREADY_EXISTS = -2,
		ERR_STAGE_PLUG_ALREADY_EXISTS = -1,
		//--
		OK = 1,
		SYNC = 2,
		LEAVING = 3,
		HEARTBEAT = 4
	};

    //Message structs
    //---------------
    struct OKAck{
        bool empty;
        MSGPACK_DEFINE(empty);
    };

	struct SignalAck {
		Signal sig;
		MSGPACK_DEFINE(sig);
	};

	struct CreateEndpoint {
		std::string uuid;
		std::string address;
		MSGPACK_DEFINE(uuid, address);
	};

	struct CreateEndpointAck {
		std::string assigned_uuid;
		MSGPACK_DEFINE(assigned_uuid);
	};

	struct RegisterEntityType {
		std::string performer;
		std::string entity_type;
		MSGPACK_DEFINE(performer, entity_type);
	};

	struct CreateEntity {
		std::string entity_type;
		ZstURIWire address;
		std::string endpoint_uuid;
		MSGPACK_DEFINE(entity_type, address, endpoint_uuid);
	};
    
    struct CreatePlug{
		ZstURIWire address;
        PlugDirection dir;
        MSGPACK_DEFINE(address, dir);
    };

    struct DestroyURI {
		ZstURIWire address;
        MSGPACK_DEFINE(address);
    };

	struct CreateCable {
		ZstURIWire first;
		ZstURIWire second;
		MSGPACK_DEFINE(first, second);
	};

	struct StageUpdates {
		std::vector<ZstEventWire> updates;
		MSGPACK_DEFINE_ARRAY(updates);
	};
    
    struct Heartbeat {
        std::string from;
        long timestamp;
        MSGPACK_DEFINE(from, timestamp);
    };

	struct PerformerConnection {
		std::string endpoint;
		ZstURIWire output_plug;
		ZstURIWire input_plug;
		MSGPACK_DEFINE(endpoint, output_plug, input_plug);
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
	static zmsg_t * build_message(Kind message_id, T message_args) {
		zmsg_t *msg = zmsg_new();
		zframe_t * kind = build_message_kind_frame(message_id);
		zmsg_append(msg, &kind);

		msgpack::sbuffer buf;
		msgpack::pack(buf, message_args);
		zframe_t *payload = zframe_new(buf.data(), buf.size());
		zmsg_append(msg, &payload);
		return msg;
	}
    
    template <typename T>
    static zmsg_t * build_graph_message(ZstURI from, T data) {
        zmsg_t *msg = zmsg_new();
        zmsg_addstr(msg, from.path());
        
        msgpack::sbuffer buf;
        msgpack::pack(buf, data);
        zframe_t *payload = zframe_new(buf.data(), buf.size());
        zmsg_append(msg, &payload);
        return msg;
    }

	static zmsg_t * build_signal(ZstMessages::Signal signal) {
		zmsg_t *msg = zmsg_new();
		zframe_t * kind = build_message_kind_frame(ZstMessages::Kind::SIGNAL);
		zmsg_append(msg, &kind);
		zmsg_addstr(msg, std::to_string(signal).c_str());
		return msg;
	}

	static Signal unpack_signal(zmsg_t * msg) {
		return (Signal)std::atoi(zmsg_popstr(msg));
	}
};

//Enums for MsgPack
MSGPACK_ADD_ENUM(ZstMessages::Signal);
MSGPACK_ADD_ENUM(PlugDirection);
