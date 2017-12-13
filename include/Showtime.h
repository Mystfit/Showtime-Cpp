#pragma once

#include "ZstURI.h"
#include "ZstConstants.h"
#include "ZstExports.h"

class ZstEntityBase;
class ZstContainer;
class ZstComponent;
class ZstCable;
class ZstPlug;
class ZstClient;
class ZstComponentEvent;
class ZstComponentTypeEvent;
class ZstPlugEvent;
class ZstCableEvent;

class Showtime
{
public:
	//Destructor
	ZST_CLIENT_EXPORT ~Showtime();

	//Destroy the endpoint instrance and leave
	ZST_CLIENT_EXPORT static void destroy();

	//Init the library
	ZST_CLIENT_EXPORT static void init(const char * performer);
    ZST_CLIENT_EXPORT static void join(const char * stage_address);
	ZST_CLIENT_EXPORT static void leave();
	ZST_CLIENT_EXPORT static bool is_connected();

	//Poll the event queue - for runtimes that have process events from the main thread
	ZST_CLIENT_EXPORT static void poll_once();

	//Callbacks
	ZST_CLIENT_EXPORT static void attach(ZstComponentEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void attach(ZstComponentTypeEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void attach(ZstPlugEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void attach(ZstCableEvent * callback, ZstCallbackAction action);
    
    ZST_CLIENT_EXPORT static void detach(ZstComponentEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void detach(ZstComponentTypeEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void detach(ZstPlugEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void detach(ZstCableEvent * callback, ZstCallbackAction action);

	//Entity activation/deactivation
	ZST_CLIENT_EXPORT static int activate(ZstEntityBase * component);
	ZST_CLIENT_EXPORT static int destroy(ZstEntityBase * component);

	//Hierarchy
    ZST_CLIENT_EXPORT static ZstContainer* get_root();
	ZST_CLIENT_EXPORT static ZstEntityBase* get_performer_by_URI(const ZstURI & path);

    //Stage methods
    ZST_CLIENT_EXPORT static int ping_stage();

    //Cable management
	ZST_CLIENT_EXPORT static int connect_cable(ZstPlug * a, ZstPlug * b);
	ZST_CLIENT_EXPORT static int destroy_cable(ZstCable * cable);
    
    //Creatables
    ZST_CLIENT_EXPORT static void register_component_type(ZstComponent * component_template);
    ZST_CLIENT_EXPORT static void unregister_component_type(ZstComponent * component_template);
    ZST_CLIENT_EXPORT static void run_component_template(ZstComponent * entity);
    ZST_CLIENT_EXPORT static void run_component_template(ZstComponent * entity, bool wait);
private:
    Showtime();
	Showtime(const Showtime&); // Prevent construction by copying
	Showtime& operator=(const Showtime&) {}; // Prevent assignment
};
