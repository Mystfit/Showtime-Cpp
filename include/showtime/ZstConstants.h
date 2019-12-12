#pragma once

// ZstSynchronisable activation status
enum ZstSyncStatus {
	DEACTIVATED = 0,
	ACTIVATING,
	ACTIVATION_QUEUED,
	ACTIVATED,
	DEACTIVATING,
	DEACTIVATION_QUEUED,
	DESTROYED,
	ERR
};

enum ZstTransportRequestBehaviour
{
	PUBLISH = 0,
	SYNC_REPLY,
	ASYNC_REPLY
};

// ZstSynchronisable errors
enum ZstSyncError {
	NO_ERR,
	PERFORMER_NOT_FOUND,
	PARENT_NOT_FOUND,
	ENTITY_ALREADY_EXISTS
};

//Ports
#define STAGE_DISCOVERY_PORT 40003
#define STAGE_ROUTER_PORT 40004
#define CLIENT_UNRELIABLE_PORT 40006
#define CLIENT_MULTICAST_ADDR "239.0.0.8"

//Heartbeats
#define HEARTBEAT_DURATION 1000
#define MAX_MISSED_HEARTBEATS 10
#define MESSAGE_POOL_BLOCK 256

// Number of milliseconds to wait until a timeout is called on a stage operation
#define STAGE_TIMEOUT 5000
#define STAGE_HEARTBEAT_CHECK HEARTBEAT_DURATION * 2

