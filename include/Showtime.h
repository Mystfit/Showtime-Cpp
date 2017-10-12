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
class ZstComposer;

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
	ZST_EXPORT static void attach(ZstEntityEventCallback * callback, ZstCallbackAction action);
    ZST_EXPORT static void attach(ZstPlugEventCallback * callback, ZstCallbackAction action);
    ZST_EXPORT static void attach(ZstCableEventCallback * callback, ZstCallbackAction action);
    
    ZST_EXPORT static void detach(ZstEntityEventCallback * callback, ZstCallbackAction action);
    ZST_EXPORT static void detach(ZstPlugEventCallback * callback, ZstCallbackAction action);
    ZST_EXPORT static void detach(ZstCableEventCallback * callback, ZstCallbackAction action);
    
	//Hierarchy
	ZST_EXPORT static ZstEntityBase* get_entity_by_URI(const ZstURI & path);

    //Stage methods
    ZST_EXPORT static int ping_stage();

    //Event queue management
	ZST_EXPORT static ZstEvent * pop_event();
	ZST_EXPORT static size_t event_queue_size();

    //Cable management
	ZST_EXPORT static int connect_cable(ZstURI a, ZstURI b);
	ZST_EXPORT static int destroy_cable(ZstURI a, ZstURI b);
    
    //Creatables
    ZST_EXPORT static void register_composer(ZstComposer * composer);
    ZST_EXPORT static void unregister_composer(ZstComposer * composer);

private:
    Showtime();
	Showtime(const Showtime&); // Prevent construction by copying
	Showtime& operator=(const Showtime&) {}; // Prevent assignment
};
