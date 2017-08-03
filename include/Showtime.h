#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ZstExports.h"

class ZstPlug;
class ZstEntityBase;
class ZstInputPlug;
class ZstOutputPlug;
class ZstPerformer;
class ZstURI;
class ZstEndpoint;
class ZstEvent;
class ZstEventCallback;
class ZstEntityEventCallback;
class ZstPlugEventCallback;
class ZstCableEventCallback;

#define STAGE_REP_PORT 6000
#define STAGE_ROUTER_PORT 6001
#define STAGE_PUB_PORT 6002

#define HEARTBEAT_DURATION 1000

enum RuntimeLanguage {
	NATIVE_RUNTIME,
	PYTHON_RUNTIME,
	DOTNET_RUNTIME
};

enum ZstValueType {
	ZST_NONE = 0,
	ZST_INT,
	ZST_FLOAT,
	ZST_STRING
};

enum ZstEntityBehaviour {
	FILTER = 0,
	PATCH,
	COMPONENT,
	PERFORMER
};

class Showtime
{
public:
	//Destructor
	ZST_EXPORT ~Showtime();

	//Endpoint singleton
	ZST_EXPORT static ZstEndpoint & endpoint();

	//Destroy the endpoint instrance and leave
	ZST_EXPORT static void destroy();

	ZST_EXPORT static void set_runtime_language(RuntimeLanguage runtime);
	ZST_EXPORT static RuntimeLanguage get_runtime_language();

	//Init the library
	ZST_EXPORT static void init();
    ZST_EXPORT static void join(const char * stage_address);
	ZST_EXPORT static void leave();
	ZST_EXPORT static bool is_connected();

	//Poll the event queue - for runtimes that have process events from the main thread
	ZST_EXPORT static void poll_once();

	//Callbacks
	ZST_EXPORT static void attach_stage_event_callback(ZstEventCallback * callback);
	ZST_EXPORT static void attach_entity_arriving_callback(ZstEntityEventCallback * callback);
	ZST_EXPORT static void attach_performer_leaving_callback(ZstEntityEventCallback * callback);
	ZST_EXPORT static void attach_plug_arriving_callback(ZstPlugEventCallback * callback);
	ZST_EXPORT static void attach_plug_leaving_callback(ZstPlugEventCallback * callback);
	ZST_EXPORT static void attach_cable_arriving_callback(ZstCableEventCallback *callback);
	ZST_EXPORT static void attach_cable_leaving_callback(ZstCableEventCallback *callback);

	ZST_EXPORT static void remove_stage_event_callback(ZstEventCallback * callback);
	ZST_EXPORT static void remove_performer_arriving_callback(ZstEntityEventCallback * callback);
	ZST_EXPORT static void remove_performer_leaving_callback(ZstEntityEventCallback * callback);
	ZST_EXPORT static void remove_plug_arriving_callback(ZstPlugEventCallback * callback);
	ZST_EXPORT static void remove_plug_leaving_callback(ZstPlugEventCallback * callback);
	ZST_EXPORT static void remove_cable_arriving_callback(ZstCableEventCallback * callback);
	ZST_EXPORT static void remove_cable_leaving_callback(ZstCableEventCallback * callback);

    //Stage methods
    ZST_EXPORT static int ping_stage();

 	//Entities can contain other entities and plugs
	ZST_EXPORT static void register_entity_type(const char * entity_type);

	ZST_EXPORT static ZstEvent pop_event();
	ZST_EXPORT static int event_queue_size();

	ZST_EXPORT static int connect_cable(ZstURI a, ZstURI b);
	ZST_EXPORT static int destroy_cable(ZstURI a, ZstURI b);


private:
    Showtime();
	Showtime(const Showtime&); // Prevent construction by copying
	Showtime& operator=(const Showtime&) {}; // Prevent assignment

	//Active runtime
	static RuntimeLanguage _runtime_language;
};
