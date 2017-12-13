#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <stdio.h>
#include <msgpack.hpp>
#include <czmq.h>
#include "ZstConstants.h"
#include "ZstStreamable.h"
#include "entities/ZstPerformer.h"
#include "entities/ZstComponent.h"
#include "entities/ZstContainer.h"
#include "entities/ZstPlug.h"

class ZstMessages{
public:
    enum Kind  {
        SIGNAL,
        GRAPH_SNAPSHOT,
        
        //Client registration
        CLIENT_JOIN,
        CLIENT_JOIN_ACK,
		CLIENT_LEAVE,
        
        //Entity registration
        CREATE_ENTITY_FROM_TEMPLATE,
        REGISTER_COMPONENT_TEMPLATE,
		UNREGISTER_COMPONENT_TEMPLATE,
        CREATE_COMPONENT,
		CREATE_CONTAINER,
		CREATE_PERFORMER,
        DESTROY_ENTITY,
        
        //Plug registration
        CREATE_PLUG,
        DESTROY_PLUG,

        //Connection registration
        CREATE_CABLE,
        DESTROY_CABLE,
        
        //P2P endpoint connection requests
        SUBSCRIBE_TO_PERFORMER,
        CREATE_PEER_ENTITY
    };
    
    struct MessagePair {
        ZstMessages::Kind kind;
        std::string packed;
    };

	enum Signal {
        ERR_STAGE_MSG_TYPE_UNKNOWN = -8,
		ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST = -7,
		ERR_STAGE_BAD_CABLE_CONNECT_REQUEST = -6,
		ERR_STAGE_PERFORMER_NOT_FOUND = -5,
		ERR_STAGE_PERFORMER_ALREADY_EXISTS = -4,
		ERR_STAGE_ENTITY_NOT_FOUND = -3,
		ERR_STAGE_ENTITY_ALREADY_EXISTS = -2,
		ERR_STAGE_PLUG_ALREADY_EXISTS = -1,
		//--
		OK = 1,
		SYNC = 2,
		LEAVING = 3,
		HEARTBEAT = 4
	};
    
   
	//---------------------------------------
	static zframe_t * build_entity_kind_frame(ZstEntityBase * entity) {
		Kind k;
		if (strcmp(entity->entity_type(), COMPONENT_TYPE)) {
			k = Kind::CREATE_COMPONENT;
		}
		else if (strcmp(entity->entity_type(), CONTAINER_TYPE)) {
			k = Kind::CREATE_CONTAINER;
		}
		else if (strcmp(entity->entity_type(), PERFORMER_TYPE)) {
			k = Kind::CREATE_PERFORMER;
		}
		else if (strcmp(entity->entity_type(), PLUG_TYPE)) {
			k = Kind::CREATE_PLUG;
		}
		return build_message_kind_frame(k);
	}

	//Build a message id from the message ID enum
	static zframe_t * build_message_kind_frame(Kind msg_id) {
		char id[sizeof(Kind)];
		sprintf(id, "%d", (int)msg_id);
		return zframe_from(id);
	}

	//Pops message id from fromt of message
	static Kind pop_message_kind_frame(zmsg_t * msg){
		char * kind_str = zmsg_popstr(msg);
		ZstMessages::Kind k = static_cast<ZstMessages::Kind>(kind_str[0]);
		zstr_free(&kind_str);
		return k;
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

	template <typename T>
	static T* unpack_entity(const char * buffer, size_t length) {
		T* entity = new T();
		size_t offset = 0;
		entity->read(buffer, length, offset);
		return entity;
	}

	template <typename T>
	static T unpack_streamable(const char * buffer, size_t length) {
		T streamable = T();
		size_t offset = 0;
		streamable.read(buffer, length, offset);
		return streamable;
	}
};

//Enums for MsgPack
MSGPACK_ADD_ENUM(ZstMessages::Signal);
