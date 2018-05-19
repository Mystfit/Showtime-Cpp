#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdio.h>
#include <msgpack.hpp>
#include <czmq.h>
#include <ZstExports.h>
#include "liasons/ZstPlugLiason.hpp"

#define KIND_FRAME_SIZE 1
#define ZSTMSG_UUID_LENGTH 33	//Size of a CZMQ uuid (32 bytes + null terminator)

/**
 * Enum:	ZstMsgKind
 *
 * Summary:	Values that represent possible messages.
 */
enum ZstMsgKind  {
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
	ERR_STAGE_TIMEOUT,
    
    //Client registration
    CLIENT_JOIN,
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
	    
    //Connection registration
    CREATE_CABLE,
    DESTROY_CABLE,
    
    //P2P endpoint connection requests
    SUBSCRIBE_TO_PERFORMER,
    CREATE_PEER_ENTITY,

	//Plug values
	PLUG_VALUE
};
MSGPACK_ADD_ENUM(ZstMsgKind);


/**
 * Struct:	ZstMessageReceipt
 *
 * Summary:	Message response from a message sent to the server.
 */
struct ZstMessageReceipt {
	ZstMsgKind status;
	bool async;
};


/**
 * Class:	ZstMessagePayload
 *
 * Summary:	A single payload frame in a ZstMessage
 */
class ZstMessagePayload {
public:
    ZST_EXPORT ZstMessagePayload(ZstMsgKind k, zframe_t * p);
    ZST_EXPORT ZstMessagePayload(const ZstMessagePayload & other);
	ZST_EXPORT ~ZstMessagePayload();

	ZST_EXPORT ZstMessagePayload(ZstMessagePayload && source);
	ZST_EXPORT ZstMessagePayload& operator=(ZstMessagePayload && source);
	ZST_EXPORT ZstMessagePayload& operator=(ZstMessagePayload & other);
	
    ZST_EXPORT const ZstMsgKind kind();
    ZST_EXPORT const size_t size();
	ZST_EXPORT const char * data();
	
protected:
	zframe_t * m_payload;	
	size_t m_size;
	ZstMsgKind m_kind;
};


class ZstMessage {
public:
	ZST_EXPORT ZstMessage();
	ZST_EXPORT ~ZstMessage();
	ZST_EXPORT ZstMessage(const ZstMessage & other);

	ZST_EXPORT virtual void reset();
	ZST_EXPORT virtual void unpack(zmsg_t * msg);
	ZST_EXPORT virtual void append_str(const char * s, size_t len);

	ZST_EXPORT virtual ZstMessagePayload & payload_at(size_t index);
	ZST_EXPORT virtual size_t num_payloads();
	ZST_EXPORT zmsg_t * handle();

	template <typename T>
	T unpack_payload_serialisable(size_t payload_index) {
		T serialisable;
		size_t offset = 0;
		serialisable.read((char*)payload_at(payload_index).data(), payload_at(payload_index).size(), offset);
		return serialisable;
	}

protected:
	ZST_EXPORT void append_payload_frame(const ZstSerialisable & streamable);
	ZST_EXPORT void set_handle(zmsg_t * handle);
	std::vector<ZstMessagePayload> m_payloads;

private:
	zmsg_t * m_msg_handle;
};
