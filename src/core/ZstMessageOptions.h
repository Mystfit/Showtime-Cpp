#pragma once

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/assign/list_of.hpp>
#include <nlohmann/json.hpp>
#include "ZstLogging.h"

using namespace nlohmann;


/**
* Enum:    ZstMsgKind
*
* Summary: Values that represent possible messages.
*/
enum ZstMsgKind {
	EMPTY = 0,

	//Regular signals
	OK,

	//Error signals starts
	ERR_MSG_TYPE_UNKNOWN, //2
	ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST,
	ERR_STAGE_BAD_CABLE_CONNECT_REQUEST,
	ERR_STAGE_PERFORMER_NOT_FOUND,
	ERR_STAGE_PERFORMER_ALREADY_EXISTS,
	ERR_STAGE_PERFORMER_ALREADY_CONNECTED,
	ERR_STAGE_REQUEST_MISSING_ARG,
	ERR_ENTITY_NOT_FOUND,
	ERR_ENTITY_ALREADY_EXISTS,
	ERR_CABLE_PLUGS_NOT_FOUND,
	ERR_STAGE_TIMEOUT,

	//Client registration
	CLIENT_JOIN, //11
	CLIENT_SYNC,
	CLIENT_LEAVING,
	CLIENT_HEARTBEAT,
    
    //Beacons
    SERVER_BEACON,

	//Entity registration
	CREATE_ENTITY_FROM_FACTORY, //16
	REGISTER_COMPONENT_TEMPLATE,
	UNREGISTER_COMPONENT_TEMPLATE,
	CREATE_COMPONENT,
	CREATE_PERFORMER,
	CREATE_FACTORY,
	DESTROY_ENTITY,

	//Entity updates
	UPDATE_ENTITY,

	//Plug registration
	CREATE_PLUG, //23
	AQUIRE_ENTITY_OWNERSHIP,
	RELEASE_ENTITY_OWNERSHIP,

	//Connection registration
	CREATE_CABLE, //24
	DESTROY_CABLE,
	OBSERVE_ENTITY,

	//P2P endpoint connection requests
	START_CONNECTION_HANDSHAKE, //26
	STOP_CONNECTION_HANDSHAKE,
	CONNECTION_HANDSHAKE,
	SUBSCRIBE_TO_PERFORMER,
	CREATE_PEER_ENTITY,

	//Generic performance message
	PERFORMANCE_MSG
};

typedef boost::bimaps::bimap<ZstMsgKind, std::string> ZstMsgKindMap;
static ZstMsgKindMap ZstMsgNames = boost::assign::list_of<ZstMsgKindMap::relation>
	(EMPTY, "empty")
	(OK, "OK")
	(ERR_MSG_TYPE_UNKNOWN, "ERR_MSG_TYPE_UNKNOWN")
	(ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST, "ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST")
	(ERR_STAGE_BAD_CABLE_CONNECT_REQUEST, "ERR_STAGE_BAD_CABLE_CONNECT_REQUEST")
	(ERR_STAGE_PERFORMER_NOT_FOUND, "ERR_STAGE_PERFORMER_NOT_FOUND")
	(ERR_STAGE_PERFORMER_ALREADY_EXISTS, "ERR_STAGE_PERFORMER_ALREADY_EXISTS")
	(ERR_STAGE_PERFORMER_ALREADY_CONNECTED, "ERR_STAGE_PERFORMER_ALREADY_CONNECTED")
	(ERR_STAGE_REQUEST_MISSING_ARG, "ERR_STAGE_REQUEST_MISSING_ARG")
	(ERR_ENTITY_NOT_FOUND, "ERR_ENTITY_NOT_FOUND")
	(ERR_ENTITY_ALREADY_EXISTS, "ERR_ENTITY_ALREADY_EXISTS")
	(ERR_CABLE_PLUGS_NOT_FOUND, "ERR_CABLE_PLUGS_NOT_FOUND")
	(ERR_STAGE_TIMEOUT, "ERR_STAGE_TIMEOUT")
	(CLIENT_JOIN, "CLIENT_JOIN")
	(CLIENT_SYNC, "CLIENT_SYNC")
	(CLIENT_LEAVING, "CLIENT_LEAVING")
	(CLIENT_HEARTBEAT, "CLIENT_HEARTBEAT")
    (SERVER_BEACON, "SERVER_BEACON")
	(CREATE_ENTITY_FROM_FACTORY, "CREATE_ENTITY_FROM_FACTORY")
	(REGISTER_COMPONENT_TEMPLATE, "REGISTER_COMPONENT_TEMPLATE")
	(UNREGISTER_COMPONENT_TEMPLATE, "UNREGISTER_COMPONENT_TEMPLATE")
	(CREATE_COMPONENT, "CREATE_COMPONENT")
	(CREATE_PERFORMER, "CREATE_PERFORMER")
	(CREATE_FACTORY, "CREATE_FACTORY")
	(DESTROY_ENTITY, "DESTROY_ENTITY")
	(UPDATE_ENTITY, "UPDATE_ENTITY")
	(CREATE_PLUG, "CREATE_PLUG")
	(AQUIRE_ENTITY_OWNERSHIP, "AQUIRE_ENTITY_OWNERSHIP")
	(RELEASE_ENTITY_OWNERSHIP, "RELEASE_ENTITY_OWNERSHIP")
	(CREATE_CABLE, "CREATE_CABLE")
	(DESTROY_CABLE, "DESTROY_CABLE")
	(OBSERVE_ENTITY, "OBSERVE_ENTITY")
	(START_CONNECTION_HANDSHAKE, "START_CONNECTION_HANDSHAKE")
	(STOP_CONNECTION_HANDSHAKE, "STOP_CONNECTION_HANDSHAKE")
	(SUBSCRIBE_TO_PERFORMER, "SUBSCRIBE_TO_PERFORMER")
	(CREATE_PEER_ENTITY, "CREATE_PEER_ENTITY")
	(PERFORMANCE_MSG, "PERFORMANCE_MSG");


enum ZstMsgArg {
	NAME = 0,
	KIND,
    ADDRESS,
    ADDRESS_PORT,
	GRAPH_RELIABLE_OUTPUT_ADDRESS,
	GRAPH_UNRELIABLE_INPUT_ADDRESS,
	INPUT_PATH,
	OUTPUT_PATH,
	PATH,
	UNRELIABLE,
	REQUEST_ID,
	MSG_ID,
	//SENDER,
	SENDER_SHORT,
	//DESTINATION,
	PAYLOAD,
	PAYLOAD_SHORT,
	UNKNOWN
};

typedef boost::bimaps::bimap<ZstMsgArg, std::string> ZstMsgArgMap;
static ZstMsgArgMap ZstMsgArgNames = boost::assign::list_of<ZstMsgArgMap::relation>
	( NAME, "name" )
	( KIND, "kind" )
    ( ADDRESS, "address" )
    ( ADDRESS_PORT, "address_port" )
	( GRAPH_RELIABLE_OUTPUT_ADDRESS, "graph_out_reliable_addr" )
	( GRAPH_UNRELIABLE_INPUT_ADDRESS, "graph_in_unreliable_addr" )
	( INPUT_PATH, "in_path" )
	( OUTPUT_PATH, "out_path" )
	( PATH, "path" )
	( REQUEST_ID, "request_id" )
	( MSG_ID, "msg_id" )
	//( SENDER, "sender" )
	( SENDER_SHORT, "s")
	//( DESTINATION , "destination" )
	( PAYLOAD , "payload" )
	( PAYLOAD_SHORT, "p");

static inline const std::string & get_msg_name(const ZstMsgKind & msg_kind){
	return ZstMsgNames.left.at(msg_kind);
}

static inline const ZstMsgKind & get_msg_kind(const std::string msg_kind_str) {
	return ZstMsgNames.right.at(msg_kind_str);
}

static inline const std::string & get_msg_arg_name(const ZstMsgArg & msg_arg) {
	return ZstMsgArgNames.left.at(msg_arg);
}

static inline const ZstMsgArg & get_msg_arg(const std::string & msg_arg_str) {
	return ZstMsgArgNames.right.at(msg_arg_str);
}

typedef json ZstMsgArgs;
typedef json ZstMsgPayload;
