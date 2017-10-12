#pragma once

enum ZstValueType {
	ZST_NONE = 0,
	ZST_INT,
	ZST_FLOAT,
	ZST_STRING
};

enum ZstEntityBehaviour {
	FILTER = 0,
	PATCH,
	COMPONENT
};

enum ZstCallbackAction {
    ARRIVING = 0,
    LEAVING
};


#define STAGE_REP_PORT 6000
#define STAGE_ROUTER_PORT 6001
#define STAGE_PUB_PORT 6002

#define HEARTBEAT_DURATION 1000
