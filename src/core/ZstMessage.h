#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdio.h>
#include <msgpack.hpp>
#include <czmq.h>
#include <ZstExports.h>
#include "ZstMsgID.h"
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
    
    //Error signals starts
    ERR_STAGE_MSG_TYPE_UNKNOWN, //2
    ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST,
    ERR_STAGE_BAD_CABLE_CONNECT_REQUEST,
    ERR_STAGE_PERFORMER_NOT_FOUND,
    ERR_STAGE_PERFORMER_ALREADY_EXISTS,
    ERR_STAGE_ENTITY_NOT_FOUND,
    ERR_STAGE_ENTITY_ALREADY_EXISTS,
    ERR_STAGE_PLUG_ALREADY_EXISTS,
	ERR_STAGE_TIMEOUT,
    
    //Client registration
    CLIENT_JOIN, //11
    CLIENT_SYNC,
    CLIENT_LEAVING,
    CLIENT_HEARTBEAT,
	    
    //Entity registration
    CREATE_ENTITY_FROM_TEMPLATE, //16
    REGISTER_COMPONENT_TEMPLATE,
    UNREGISTER_COMPONENT_TEMPLATE,
    CREATE_COMPONENT,
    CREATE_CONTAINER,
    CREATE_PERFORMER,
    DESTROY_ENTITY,
    
    //Plug registration
    CREATE_PLUG, //23
	    
    //Connection registration
    CREATE_CABLE, //24
    DESTROY_CABLE,
    
    //P2P endpoint connection requests
    START_CONNECTION_HANDSHAKE, //26
    STOP_CONNECTION_HANDSHAKE,
	CONNECTION_HANDSHAKE,
    SUBSCRIBE_TO_PERFORMER,
    SUBSCRIBE_TO_PERFORMER_ACK,
    CREATE_PEER_ENTITY,

	//Generic performance message
	PERFORMANCE_MSG
};
MSGPACK_ADD_ENUM(ZstMsgKind);

static std::map<ZstMsgKind, char const*> ZstMsgNames {
    {EMPTY, "empty"},    
    {OK, "OK"},
    {ERR_STAGE_MSG_TYPE_UNKNOWN, "ERR_STAGE_MSG_TYPE_UNKNOWN"}, 
    {ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST, "ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST"},
    {ERR_STAGE_BAD_CABLE_CONNECT_REQUEST, "ERR_STAGE_BAD_CABLE_CONNECT_REQUEST"},
    {ERR_STAGE_PERFORMER_NOT_FOUND, "ERR_STAGE_PERFORMER_NOT_FOUND"},
    {ERR_STAGE_PERFORMER_ALREADY_EXISTS, "ERR_STAGE_PERFORMER_ALREADY_EXISTS" },
    {ERR_STAGE_ENTITY_NOT_FOUND, "ERR_STAGE_ENTITY_NOT_FOUND"},
    {ERR_STAGE_ENTITY_ALREADY_EXISTS, "ERR_STAGE_ENTITY_ALREADY_EXISTS"},
    {ERR_STAGE_PLUG_ALREADY_EXISTS, "ERR_STAGE_PLUG_ALREADY_EXISTS"},
	{ERR_STAGE_TIMEOUT, "ERR_STAGE_TIMEOUT"},
    {CLIENT_JOIN, "CLIENT_JOIN"},
    {CLIENT_SYNC, "CLIENT_SYNC"},
    {CLIENT_LEAVING, "CLIENT_LEAVING"},
    {CLIENT_HEARTBEAT, "CLIENT_HEARTBEAT"},
    {CREATE_ENTITY_FROM_TEMPLATE, "CREATE_ENTITY_FROM_TEMPLATE"},
    {REGISTER_COMPONENT_TEMPLATE, "REGISTER_COMPONENT_TEMPLATE"},
    {UNREGISTER_COMPONENT_TEMPLATE, "UNREGISTER_COMPONENT_TEMPLATE"},
    {CREATE_COMPONENT, "CREATE_COMPONENT"},
    {CREATE_CONTAINER, "CREATE_CONTAINER"},
    {CREATE_PERFORMER, "CREATE_PERFORMER"},
    {DESTROY_ENTITY, "DESTROY_ENTITY"},
    {CREATE_PLUG, "CREATE_PLUG"},
    {CREATE_CABLE, "CREATE_CABLE"},
    {DESTROY_CABLE, "DESTROY_CABLE"},
    {START_CONNECTION_HANDSHAKE, "START_CONNECTION_HANDSHAKE"},
    {STOP_CONNECTION_HANDSHAKE, "STOP_CONNECTION_HANDSHAKE"},
    {SUBSCRIBE_TO_PERFORMER, "SUBSCRIBE_TO_PERFORMER"},
    {SUBSCRIBE_TO_PERFORMER_ACK, "SUBSCRIBE_TO_PERFORMER_ACK"},
    {CREATE_PEER_ENTITY, "CREATE_PEER_ENTITY"},
	{PERFORMANCE_MSG, "PERFORMANCE_MSG"}
};


/**
 * Class:	ZstMessagePayload
 *
 * Summary:	A single payload frame in a ZstMessage
 */
class ZstMessagePayload {
public:
    ZST_EXPORT ZstMessagePayload(zframe_t * p);
    ZST_EXPORT ZstMessagePayload(const ZstMessagePayload & other);
	ZST_EXPORT ~ZstMessagePayload();

	ZST_EXPORT ZstMessagePayload(ZstMessagePayload && source) noexcept;
	//ZST_EXPORT ZstMessagePayload& operator=(ZstMessagePayload && source);
	ZST_EXPORT ZstMessagePayload & operator=(const ZstMessagePayload & other);
	
    ZST_EXPORT const size_t size();
	ZST_EXPORT const char * data();
	
protected:
	zframe_t * m_payload;	
	size_t m_size;
};

typedef std::unordered_map<std::string, std::string> ZstMsgArgs;

class ZstMessage {
public:
	ZST_EXPORT ZstMessage();
	ZST_EXPORT ~ZstMessage();
	ZST_EXPORT ZstMessage(const ZstMessage & other);

	ZST_EXPORT virtual void init();
	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind);
	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs & args);
	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstSerialisable & serialisable);
	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstSerialisable & serialisable, const ZstMsgArgs & args);

	ZST_EXPORT virtual void reset();
    ZST_EXPORT virtual void set_inactive();
	ZST_EXPORT virtual void unpack(zmsg_t * msg);
	ZST_EXPORT ZstMsgKind unpack_kind(zframe_t * kind_frame);

	ZST_EXPORT void append_args(const ZstMsgArgs & args);
	ZST_EXPORT const std::string & get_arg_s(const char * key);
	ZST_EXPORT const char * get_arg(const char * key);
	ZST_EXPORT size_t get_arg_size(const char * key);

	ZST_EXPORT void log_args();

	ZST_EXPORT virtual ZstMessagePayload & payload_at(size_t index);
	ZST_EXPORT virtual size_t num_payloads();
	ZST_EXPORT zmsg_t * handle();

	ZST_EXPORT const ZstMsgKind kind() const;
	ZST_EXPORT ZstMsgID id() const;
	ZST_EXPORT void set_id(ZstMsgID id);
	ZST_EXPORT void copy_id(const ZstMessage * msg);

	template <typename T>
	T unpack_payload_serialisable(size_t payload_index) {
		T serialisable;
		size_t offset = 0;
		serialisable.read((char*)payload_at(payload_index).data(), payload_at(payload_index).size(), offset);
		return serialisable;
	}

	ZST_EXPORT static ZstMsgKind entity_kind(ZstEntityBase * entity);

protected:
	ZST_EXPORT void append_payload_frame(const ZstSerialisable & streamable);
	ZST_EXPORT void append_kind_frame(ZstMsgKind k);
	ZST_EXPORT void append_serialisable(ZstMsgKind k, const ZstSerialisable & s);
	ZST_EXPORT void prepend_id_frame(ZstMsgID id);
	ZST_EXPORT void append_id_frame(ZstMsgID id);
	ZST_EXPORT void set_handle(zmsg_t * handle);
	std::vector<ZstMessagePayload> m_payloads;
	ZstMsgKind m_msg_kind;
	ZstMsgID m_msg_id;
	ZstMsgArgs m_args;

private:
	zmsg_t * m_msg_handle;

};
