#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ZstExports.h"
#include "ZstConstants.h"

class ZstPlug;
class ZstEntityBase;
class ZstInputPlug;
class ZstOutputPlug;
class ZstPerformer;
class ZstURI;
class ZstEndpoint;
class ZstEvent;
class ZstEntityEventCallback;
class ZstPlugEventCallback;
class ZstCableEventCallback;

class Showtime
{
public:
	//Destructor
	ZST_EXPORT ~Showtime();

	//Endpoint singleton
	ZST_EXPORT static ZstEndpoint & endpoint();

	//Destroy the endpoint instrance and leave
	ZST_EXPORT static void destroy();

	//Init the library
	ZST_EXPORT static void init();
    ZST_EXPORT static void join(const char * stage_address);
	ZST_EXPORT static void leave();
	ZST_EXPORT static bool is_connected();

	//Poll the event queue - for runtimes that have process events from the main thread
	ZST_EXPORT static void poll_once();

	//Callbacks
	ZST_EXPORT static void attach_entity_arriving_callback(ZstEntityEventCallback * callback);
	ZST_EXPORT static void attach_entity_leaving_callback(ZstEntityEventCallback * callback);
	ZST_EXPORT static void attach_plug_arriving_callback(ZstPlugEventCallback * callback);
	ZST_EXPORT static void attach_plug_leaving_callback(ZstPlugEventCallback * callback);
	ZST_EXPORT static void attach_cable_arriving_callback(ZstCableEventCallback *callback);
	ZST_EXPORT static void attach_cable_leaving_callback(ZstCableEventCallback *callback);

	ZST_EXPORT static void remove_entity_arriving_callback(ZstEntityEventCallback * callback);
	ZST_EXPORT static void remove_entity_leaving_callback(ZstEntityEventCallback * callback);
	ZST_EXPORT static void remove_plug_arriving_callback(ZstPlugEventCallback * callback);
	ZST_EXPORT static void remove_plug_leaving_callback(ZstPlugEventCallback * callback);
	ZST_EXPORT static void remove_cable_arriving_callback(ZstCableEventCallback * callback);
	ZST_EXPORT static void remove_cable_leaving_callback(ZstCableEventCallback * callback);

    //Stage methods
    ZST_EXPORT static int ping_stage();

 	//Entities can contain other entities and plugs
	ZST_EXPORT static void register_entity_type(const char * entity_type);

	ZST_EXPORT static ZstEvent * pop_event();
	ZST_EXPORT static size_t event_queue_size();

	ZST_EXPORT static int connect_cable(ZstURI a, ZstURI b);
	ZST_EXPORT static int destroy_cable(ZstURI a, ZstURI b);


private:
    Showtime();
	Showtime(const Showtime&); // Prevent construction by copying
	Showtime& operator=(const Showtime&) {}; // Prevent assignment
};
