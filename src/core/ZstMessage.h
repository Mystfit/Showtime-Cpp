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


/**
 * Enum:    ZstMsgKind
 *
 * Summary: Values that represent possible messages.
 */
enum ZstMsgKind  {
    EMPTY = 0,
    
    //Regular signals
    OK,
    
    //Error signals starts
    ERR_MSG_TYPE_UNKNOWN, //2
    ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST,
    ERR_STAGE_BAD_CABLE_CONNECT_REQUEST,
    ERR_STAGE_PERFORMER_NOT_FOUND,
    ERR_STAGE_PERFORMER_ALREADY_EXISTS,
    ERR_ENTITY_NOT_FOUND,
    ERR_ENTITY_ALREADY_EXISTS,
    ERR_STAGE_TIMEOUT,
    
    //Client registration
    CLIENT_JOIN, //11
    CLIENT_SYNC,
    CLIENT_LEAVING,
    CLIENT_HEARTBEAT,
        
    //Entity registration
    CREATE_ENTITY_FROM_FACTORY, //16
    REGISTER_COMPONENT_TEMPLATE,
    UNREGISTER_COMPONENT_TEMPLATE,
    CREATE_COMPONENT,
    CREATE_CONTAINER,
    CREATE_PERFORMER,
	CREATE_FACTORY,
    DESTROY_ENTITY,

	//Entity updates
	UPDATE_ENTITY,
    
    //Plug registration
    CREATE_PLUG, //23
        
    //Connection registration
    CREATE_CABLE, //24
    DESTROY_CABLE,
	OBSERVE_ENTITY,
    
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
    {ERR_MSG_TYPE_UNKNOWN, "ERR_MSG_TYPE_UNKNOWN"}, 
    {ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST, "ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST"},
    {ERR_STAGE_BAD_CABLE_CONNECT_REQUEST, "ERR_STAGE_BAD_CABLE_CONNECT_REQUEST"},
    {ERR_STAGE_PERFORMER_NOT_FOUND, "ERR_STAGE_PERFORMER_NOT_FOUND"},
    {ERR_STAGE_PERFORMER_ALREADY_EXISTS, "ERR_STAGE_PERFORMER_ALREADY_EXISTS" },
    {ERR_ENTITY_NOT_FOUND, "ERR_ENTITY_NOT_FOUND"},
    {ERR_ENTITY_ALREADY_EXISTS, "ERR_ENTITY_ALREADY_EXISTS"},
    {ERR_STAGE_TIMEOUT, "ERR_STAGE_TIMEOUT"},
    {CLIENT_JOIN, "CLIENT_JOIN"},
    {CLIENT_SYNC, "CLIENT_SYNC"},
    {CLIENT_LEAVING, "CLIENT_LEAVING"},
    {CLIENT_HEARTBEAT, "CLIENT_HEARTBEAT"},
    {CREATE_ENTITY_FROM_FACTORY, "CREATE_ENTITY_FROM_FACTORY"},
    {REGISTER_COMPONENT_TEMPLATE, "REGISTER_COMPONENT_TEMPLATE"},
    {UNREGISTER_COMPONENT_TEMPLATE, "UNREGISTER_COMPONENT_TEMPLATE"},
    {CREATE_COMPONENT, "CREATE_COMPONENT"},
    {CREATE_CONTAINER, "CREATE_CONTAINER"},
    {CREATE_PERFORMER, "CREATE_PERFORMER"},
	{CREATE_FACTORY, "CREATE_FACTORY"},
    {DESTROY_ENTITY, "DESTROY_ENTITY"},
	{UPDATE_ENTITY, "UPDATE_ENTITY"},
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


enum ZstMsgArg {
	NAME,
    GRAPH_RELIABLE_OUTPUT_ADDRESS,
	GRAPH_UNRELIABLE_INPUT_ADDRESS,
    INPUT_PATH,
    OUTPUT_PATH,
    PATH,
	UNRELIABLE,
    REQUEST_ID,
    MSG_ID,
    SENDER_IDENTITY,
	DESTINATION_IDENTITY,
};
MSGPACK_ADD_ENUM(ZstMsgArg);

static std::map<ZstMsgArg, char const*> ZstMsgArgNames{
	{ NAME, "name" },
	{ GRAPH_RELIABLE_OUTPUT_ADDRESS, "grphout_reliableaddr" },
	{ GRAPH_UNRELIABLE_INPUT_ADDRESS, "grphin_unreliableaddr" },
    { INPUT_PATH, "inpth" },
    { OUTPUT_PATH, "outpth" },
    { PATH, "pth" },
    { REQUEST_ID, "connID" },
    { MSG_ID, "msgID" },
    { SENDER_IDENTITY, "sndr" },
	{ DESTINATION_IDENTITY , "dest" }
};

namespace std {
    template<>
    struct hash<ZstMsgArg>
    {
       size_t operator () (const ZstMsgArg& x) const
       {
          using type = typename std::underlying_type<ZstMsgArg>::type;
          return std::hash<type>()(static_cast<type>(x));
       }
    };
}

typedef std::unordered_map<ZstMsgArg, std::string> ZstMsgArgs;



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
    ZST_EXPORT void set_inactive();
    ZST_EXPORT virtual void unpack(zmsg_t * msg);
	ZST_EXPORT virtual void unpack(zframe_t * frame);

    ZST_EXPORT void append_empty_args();
    ZST_EXPORT void append_args(const ZstMsgArgs & args);
	ZST_EXPORT void append_args(const ZstMsgArgs & args, std::stringstream & buffer);

    ZST_EXPORT void set_local_arg(const ZstMsgArg & key, const std::string & value);
    ZST_EXPORT const std::string & get_arg_s(const ZstMsgArg & key) const;
	ZST_EXPORT bool has_arg(const ZstMsgArg & key) const;
    ZST_EXPORT const char * get_arg(const ZstMsgArg & key) const;
    ZST_EXPORT size_t get_arg_size(const ZstMsgArg & key) const;

    ZST_EXPORT const char * payload_data();
	ZST_EXPORT const size_t payload_size();
	ZST_EXPORT size_t & payload_offset();

    ZST_EXPORT zmsg_t * handle();
	ZST_EXPORT zframe_t * payload_frame();

    ZST_EXPORT const ZstMsgKind kind() const;
    ZST_EXPORT ZstMsgID id() const;
    ZST_EXPORT void set_id(ZstMsgID id);
    ZST_EXPORT void copy_id(const ZstMessage * msg);

    template <typename T>
    T unpack_payload_serialisable() {
        T serialisable;
        serialisable.read(payload_data(), payload_size(), payload_offset());
        return serialisable;
    }

    ZST_EXPORT static ZstMsgKind entity_kind(const ZstEntityBase & entity);

protected:
    ZST_EXPORT void append_payload(const ZstSerialisable & streamable);
	ZST_EXPORT void append_payload(const ZstSerialisable & streamable, std::stringstream & buffer);

    ZST_EXPORT void append_kind(ZstMsgKind k);
	ZST_EXPORT void append_kind(ZstMsgKind k, std::stringstream & buffer);

    ZST_EXPORT void prepend_id_frame(ZstMsgID id);
    ZST_EXPORT void append_id_frame(ZstMsgID id);
    ZST_EXPORT void set_handle(zmsg_t * handle);

	ZST_EXPORT void unpack_next_kind(zmsg_t * msg);
	ZST_EXPORT void unpack_next_kind(const char * data, size_t size, size_t & offset);
	ZST_EXPORT void unpack_next_args(zmsg_t * msg);
	ZST_EXPORT void unpack_next_args(const char * data, size_t size, size_t & offset);
	ZST_EXPORT void unpack_next_payload(zmsg_t * msg);
	ZST_EXPORT void unpack_next_payload(char * data, size_t size, size_t & offset);
	
	//Payload as char* with offset
	char * m_payload;
	size_t m_payload_size;
	size_t m_payload_offset;

	//Payload as zframe_t*
	zframe_t * m_payload_frame;

	//Message info
    ZstMsgKind m_msg_kind;
    ZstMsgID m_msg_id;
    ZstMsgArgs m_args;

private:
    zmsg_t * m_msg_handle;

};
