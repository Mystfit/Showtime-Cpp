#pragma once

#include "ZstURI.h"
#include "ZstConstants.h"
#include "ZstExports.h"
#include "ZstCore.h"

class Showtime
{
public:
	//Destructor
	ZST_CLIENT_EXPORT ~Showtime();

	//Init the library
	ZST_CLIENT_EXPORT static void init(const char * performer);
    ZST_CLIENT_EXPORT static void join(const char * stage_address);

	//Cleanup
	ZST_CLIENT_EXPORT static void destroy();
	ZST_CLIENT_EXPORT static void leave();

	//Poll the event queue - for runtimes that have process events from the main thread
	ZST_CLIENT_EXPORT static void poll_once();

	//Callbacks
	ZST_CLIENT_EXPORT static void attach_callback(ZstClientConnectionEvent * callback);
	ZST_CLIENT_EXPORT static void attach_callback(ZstPerformerEvent * callback, ZstCallbackAction action);
	ZST_CLIENT_EXPORT static void attach_callback(ZstComponentEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void attach_callback(ZstComponentTypeEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void attach_callback(ZstPlugEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void attach_callback(ZstCableEvent * callback, ZstCallbackAction action);

	ZST_CLIENT_EXPORT static void detach_callback(ZstClientConnectionEvent * callback);
	ZST_CLIENT_EXPORT static void detach_callback(ZstPerformerEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void detach_callback(ZstComponentEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void detach_callback(ZstComponentTypeEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void detach_callback(ZstPlugEvent * callback, ZstCallbackAction action);
    ZST_CLIENT_EXPORT static void detach_callback(ZstCableEvent * callback, ZstCallbackAction action);
	
	//Entity activation/deactivation
	ZST_CLIENT_EXPORT static void activate(ZstEntityBase * entity);
	ZST_CLIENT_EXPORT static void deactivate(ZstEntityBase * entity);

	//Hierarchy
    ZST_CLIENT_EXPORT static ZstPerformer* get_root();
	ZST_CLIENT_EXPORT static ZstPerformer* get_performer_by_URI(const ZstURI & path);
	ZST_CLIENT_EXPORT static ZstEntityBase* find_entity(const ZstURI & path);
	
    //Stage methods
	ZST_CLIENT_EXPORT static bool is_connected();
    ZST_CLIENT_EXPORT static int ping();

    //Cable management
	ZST_CLIENT_EXPORT static ZstCable * connect_cable(ZstPlug * a, ZstPlug * b);
	ZST_CLIENT_EXPORT static void destroy_cable(ZstCable * cable);
	ZST_CLIENT_EXPORT static void disconnect_plug(ZstPlug * plug);
	
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
