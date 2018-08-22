#pragma once

// ZstValue types
enum ZstValueType {
	ZST_NONE = 0,
	ZST_INT,
	ZST_FLOAT,
	ZST_STRING
};

// ZstSynchronisable activation status
enum ZstSyncStatus {
	DEACTIVATED = 0,
	ACTIVATING,
	ACTIVATION_QUEUED,
	ACTIVATED,
	DEACTIVATING,
	DEACTIVATION_QUEUED,
	ERR
};

// ZstSynchronisable errors
enum ZstSyncError {
	NO_ERR,
	PERFORMER_NOT_FOUND,
	PARENT_NOT_FOUND,
	ENTITY_ALREADY_EXISTS
};

enum ZstPlugDirection {
	NONE = 0,
	IN_JACK,
	OUT_JACK
};

//Ports
#define STAGE_ROUTER_PORT 10006
#define STAGE_PUB_PORT 10007
#define CLIENT_UNRELIABLE_PORT 10008

//Heartbeats
#define HEARTBEAT_DURATION 1000
#define MAX_MISSED_HEARTBEATS 10
#define MESSAGE_POOL_BLOCK 256

// Number of milliseconds to wait until a timeout is called on a stage operation
#define STAGE_TIMEOUT 5000
#define STAGE_HEARTBEAT_CHECK HEARTBEAT_DURATION * 2

