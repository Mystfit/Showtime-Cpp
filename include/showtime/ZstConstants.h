#pragma once

// ZstSynchronisable activation status
enum class ZstSyncStatus {
	DEACTIVATED = 0,
	ACTIVATING,
	ACTIVATION_QUEUED,
	ACTIVATED,
	DEACTIVATING,
	DEACTIVATION_QUEUED,
	DESTROYED,
	ERR
};

enum class ZstTransportRequestBehaviour
{
	PUBLISH = 0,
	SYNC_REPLY,
	ASYNC_REPLY
};

// ZstSynchronisable errors
enum class ZstSyncError {
	NO_ERR,
	PERFORMER_NOT_FOUND,
	PARENT_NOT_FOUND,
	ENTITY_ALREADY_EXISTS
};

//ZstValue Types
enum class ZstValueType {
	NONE = 0,
	IntList = 1,
	FloatList = 2,
	StrList = 3,
	PlugHandshake = 4
};

//Ports
#define STAGE_DISCOVERY_PORT 40003
#define STAGE_ROUTER_PORT 40004
#define STAGE_WEBSOCKET_PORT 40005
#define CLIENT_UNRELIABLE_PORT 40006
#define CLIENT_MULTICAST_ADDR "239.0.0.8"

//Heartbeats
#define HEARTBEAT_DURATION 1000
#define MAX_MISSED_HEARTBEATS 10
#define MESSAGE_POOL_BLOCK 4096

// Number of milliseconds to wait until a timeout is called on a stage operation
#define STAGE_TIMEOUT 5000
#define STAGE_HEARTBEAT_CHECK HEARTBEAT_DURATION * 2

