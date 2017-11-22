#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <stdio.h>
#include <msgpack.hpp>
#include <czmq.h>
#include "ZstURIWire.h"
#include "ZstPlug.h"

class ZstMessages{
public:
    //Message ID index
//    enum Kind{
//        SIGNAL,
//		
//		//Endpoint registration
//		ENDPOINT_JOIN,
//		ENDPOINT_JOIN_ACK,
//
//		//Entity registration
//        CREATE_ENTITY_FROM_TEMPLATE,
//        REGISTER_ENTITY_TEMPLATE,
//        CREATE_ENTITY,
//		DESTROY_ENTITY,
//		
//		//Plug registration
//        CREATE_PLUG,
//		DESTROY_PLUG,
//		
//		//Connection registration
//        CREATE_CABLE,
//		DESTROY_CABLE,
//		
//		//Misc messages
//        BATCH_GRAPH_UPDATE,
//        ENDPOINT_HEARTBEAT,
//
//		//P2P endpoint connection requests
//		CREATE_PEER_CONNECTION,
//        CREATE_PEER_ENTITY
//    };
    
    enum class Kind : char {
        SIGNAL = 'a',
        BATCH_GRAPH_UPDATE = 'b',
        
        //Endpoint registration
        ENDPOINT_JOIN = 'c',
        ENDPOINT_JOIN_ACK = 'd',
        ENDPOINT_HEARTBEAT = 'e',
        
        //Entity registration
        CREATE_ENTITY_FROM_TEMPLATE = 'f',
        REGISTER_ENTITY_TEMPLATE = 'g',
        CREATE_ENTITY = 'h',
        DESTROY_ENTITY = 'i',
        
        //Plug registration
        CREATE_PLUG = 'j',
        DESTROY_PLUG = 'k',

        //Connection registration
        CREATE_CABLE = 'l',
        DESTROY_CABLE = 'm',
        
        //P2P endpoint connection requests
        CREATE_PEER_CONNECTION = 'n',
        CREATE_PEER_ENTITY = 'o'
    };
    
    struct MessagePair {
        ZstMessages::Kind kind;
        std::string packed;
    };

	enum Signal {
        ERR_STAGE_MSG_TYPE_UNKNOWN = -8,
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
    
    struct CreatePlug{
		ZstURIWire address;
        PlugDirection dir;
        MSGPACK_DEFINE(address, dir);
    };

    struct DestroyURI {
		ZstURIWire address;
        MSGPACK_DEFINE(address);
    };

	struct CableURI {
		ZstURIWire first;
		ZstURIWire second;
		MSGPACK_DEFINE(first, second);
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
    
    struct BatchGraphUpdate {
        std::string unpack_order;
        std::vector<std::string> packed_messages;
        MSGPACK_DEFINE(packed_messages);
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
        zframe_destroy(&payload);
		return converted_struct;
	}

	//Creates a msgpacked message
	static zmsg_t * build_message(MessagePair payload) {
		zmsg_t *msg = zmsg_new();
		zframe_t * kind = build_message_kind_frame(payload.kind);
		zmsg_append(msg, &kind);
        zframe_t *payload_frame = zframe_new(payload.packed.c_str(), payload.packed.size());
		zmsg_append(msg, &payload_frame);
		return msg;
	}
    
    template<typename T>
    static MessagePair pack_message(ZstMessages::Kind kind, T item)
    {
        msgpack::sbuffer buf;
        msgpack::pack(buf, item);
        MessagePair p = {kind, std::string(buf.data())};
        return p;
    }
    
    template <typename T>
    static zmsg_t * build_graph_message(const ZstURI & from, const T & data) {
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
        char * signal_s = zmsg_popstr(msg);
        Signal sig =(Signal)std::atoi(signal_s);
        zstr_free(&signal_s);
		return sig;
	}
};

//Enums for MsgPack
MSGPACK_ADD_ENUM(ZstMessages::Signal);
MSGPACK_ADD_ENUM(PlugDirection);
