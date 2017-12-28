#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <stdio.h>
#include <msgpack.hpp>
#include <czmq.h>
#include <ZstCore.h>

#define KIND_FRAME_SIZE 1


class ZstMessage {
	friend class ZstStage;
public:
    enum Kind  {
		EMPTY = 0,

		//Regular signals
		OK,
		SYNC,
		LEAVING,
		HEARTBEAT,
        
		//Error signals
		ERR_STAGE_MSG_TYPE_UNKNOWN,
		ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST,
		ERR_STAGE_BAD_CABLE_CONNECT_REQUEST,
		ERR_STAGE_PERFORMER_NOT_FOUND,
		ERR_STAGE_PERFORMER_ALREADY_EXISTS,
		ERR_STAGE_ENTITY_NOT_FOUND,
		ERR_STAGE_ENTITY_ALREADY_EXISTS,
		ERR_STAGE_PLUG_ALREADY_EXISTS,
        
        //Client registration
        CLIENT_JOIN,
        CLIENT_JOIN_ACK,
		CLIENT_LEAVE,
		GRAPH_SNAPSHOT,
        
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

	ZstMessage(zmsg_t * msg);
	~ZstMessage();
	static void destroy(ZstMessage * msg);

	static ZstMessage create_entity_message(ZstEntityBase * entity);
	static ZstMessage create_signal_message(Kind kind);
	static ZstMessage create_streamable_message(Kind kind, ZstStreamable & streamable);
	
private:
	ZstMessage();

	//---------------------------------------
	void append_identity_frame();
	void append_kind_frame(Kind msg_id);
	void append_entity_kind_frame(ZstEntityBase * entity);
	void append_id_frame();
	void append_payload_frame(ZstStreamable & streamable);
	
	void unpack(zmsg_t * msg);

	ZstStreamable * get_payload_entity();
	ZstStreamable & get_payload_streamable();
		
	template <typename T>
	T* unpack_entity(zframe_t * frame) {
		T* entity = new T();
		size_t offset = 0;
		entity->read((char*)zframe_data(frame), zframe_size(frame), offset);
		return entity;
	}

	template <typename T>
	T unpack_streamable(zframe_t * frame) {
		T streamable = T();
		size_t offset = 0;
		streamable.read((char*)zframe_data(frame), zframe_size(frame), offset);
		return streamable;
	}

	// -----------

	size_t m_sender_length;
	char * m_sender;
	zmsg_t * m_msg_handle;
	zuuid_t * m_msg_id;
	Kind m_msg_kind;
	ZstStreamable * m_msg_target;
};
