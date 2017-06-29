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

class ZstMessages{
public:
    //Message ID index
    enum Kind{
        SIGNAL,
		STAGE_REGISTER_ENDPOINT,
		STAGE_REGISTER_ENDPOINT_ACK,
        STAGE_REGISTER_PERFORMER,
        STAGE_REGISTER_PLUG,
		STAGE_DESTROY_PLUG,
        STAGE_REGISTER_CONNECTION,
        STAGE_REMOVE_ITEM,
        STAGE_UPDATE,
        ENDPOINT_HEARTBEAT,
		PERFORMER_REGISTER_CONNECTION
    };

	enum Signal {
		OK = 0,
		SYNC,
		ERR_STAGE_ENDPOINT_NOT_FOUND,
		ERR_STAGE_ENDPOINT_ALREADY_EXISTS,
		ERR_STAGE_PERFORMER_NOT_FOUND,
		ERR_STAGE_PERFORMER_ALREADY_EXISTS,
		ERR_STAGE_PLUG_ALREADY_EXISTS,
		ERR_STAGE_BAD_PLUG_CONNECT_REQUEST
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

	struct RegisterEndpoint {
		std::string uuid;
		std::string address;
		MSGPACK_DEFINE(uuid, address);
	};

	struct RegisterEndpointAck {
		std::string assigned_uuid;
		MSGPACK_DEFINE(assigned_uuid);
	};
    
    struct RegisterPerformer{
        std::string name;
		std::string endpoint_uuid;
        MSGPACK_DEFINE(name, endpoint_uuid);
    };
    
    struct RegisterPlug{
		ZstURIWire address;
        MSGPACK_DEFINE(address);
    };

    struct DestroyPlug {
		ZstURIWire address;
        MSGPACK_DEFINE(address);
    };

    struct RegisterConnection{
		ZstURIWire from;
		ZstURIWire to;
        MSGPACK_DEFINE(from, to);
    };
    
    struct PlugOutput{
		ZstURIWire from;
        std::string value;
        MSGPACK_DEFINE(from, value);
    };

	struct StageUpdates {
		std::vector<ZstEvent> updates;
		MSGPACK_DEFINE_ARRAY(updates);
	};
    
    struct Heartbeat {
        std::string from;
        long timestamp;
        MSGPACK_DEFINE(from, timestamp);
    };


	struct ConnectPlugs {
		ZstURIWire first;
		ZstURIWire second;
		MSGPACK_DEFINE(first, second);
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
        zmsg_addstr(msg, from.to_char());
        
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
