#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdio.h>
#include <msgpack.hpp>
#include <ZstCore.h>

#define KIND_FRAME_SIZE 1

//Forwards
class ZstMessagePayload;
typedef struct _zmsg_t zmsg_t;
typedef struct _zframe_t zframe_t;
typedef struct _zuuid_t zuuid_t;
typedef struct _zsock_t zsock_t;

class ZstMessage {
	friend class ZstMessagePool;
public:
    enum Kind  {
		EMPTY = 0,

		//Regular signals
		OK,
        
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
		CLIENT_LEAVE,
		CLIENT_SYNC,
		CLIENT_LEAVING,
		CLIENT_HEARTBEAT,
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

	ZST_EXPORT ~ZstMessage();
	ZST_EXPORT void reset();
	ZST_EXPORT ZstMessage(const ZstMessage & other);
	ZST_EXPORT void copy_id(const ZstMessage * msg);

	//Initialisation
	ZST_EXPORT ZstMessage * init_entity_message(ZstEntityBase * entity);
	ZST_EXPORT ZstMessage * init_message(Kind kind);
	ZST_EXPORT ZstMessage * init_serialisable_message(Kind kind, ZstSerialisable & streamable);

	ZST_EXPORT void send(zsock_t * socket);

	//Message modification
	ZST_EXPORT void append_str(const char * s, size_t len);
	ZST_EXPORT void append_serialisable(Kind k, ZstSerialisable & s);

	//Unpacking
	ZST_EXPORT void unpack(zmsg_t * msg);

	//Attributes
	ZST_EXPORT ZstEntityBase * entity_target();
	ZST_EXPORT zmsg_t * handle();
	ZST_EXPORT const char * id();
	ZST_EXPORT Kind kind();
	
	//Message iteration
	ZST_EXPORT ZstMessagePayload & payload_at(size_t index);
	ZST_EXPORT size_t num_payloads();
	
	template <typename T>
	T unpack_payload_serialisable(size_t payload_index) {
		T serialisable = T();
		size_t offset = 0;
		ZstMessagePayload & payload = payload_at(payload_index);
		serialisable.read(payload.data(), payload.size(), offset);
		return serialisable;
	}
	
private:
	ZstMessage();
		
	//---------------------------------------
	void append_kind_frame(Kind k);
	void append_entity_kind_frame(ZstEntityBase * entity);
	void append_id_frame();
	void append_payload_frame(ZstSerialisable & streamable);
	
	//---------------------------------------
	
	Kind unpack_kind();
	Kind unpack_kind(zframe_t * kind_frame);

	//---------------------------------------
	
	//Common message attributes
	zmsg_t * m_msg_handle;
	zuuid_t * m_msg_id;
	Kind m_msg_kind;
	ZstEntityBase * m_entity_target;
	std::vector<ZstMessagePayload> m_payloads;
};


class ZstMessagePayload {
public:
	ZST_EXPORT ZstMessagePayload(ZstMessage::Kind k, zframe_t * p);
	ZST_EXPORT ZstMessagePayload(const ZstMessagePayload & other);
	ZST_EXPORT ~ZstMessagePayload();
	ZST_EXPORT size_t size();
	ZST_EXPORT char * data();
private:
	ZstMessage::Kind m_kind;
	zframe_t * m_payload;
};

//Register Kind as a msgpack enum
MSGPACK_ADD_ENUM(ZstMessage::Kind);