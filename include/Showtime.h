#pragma once

#include <chrono>
#include <utility>
#include <vector>
#include <memory>
#include "ZstExports.h"

class ZstPlug;
class ZstIntPlug;
class ZstPerformer;
class ZstURI;
class ZstEndpoint;
class ZstEvent;
class ZstEventCallback;

enum RuntimeLanguage {
	NATIVE_RUNTIME,
	PYTHON_RUNTIME,
	DOTNET_RUNTIME
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

	//Poll the event queue - for runtimes that have process events from the main thread
	ZST_EXPORT static void poll_once();

	//Plug callbacks
	ZST_EXPORT static void attach_stage_event_callback(ZstEventCallback * callback);
	ZST_EXPORT static void destroy_stage_event_callback(ZstEventCallback * callback);

    //Stage methods
    ZST_EXPORT static std::chrono::milliseconds ping_stage();

	//Performers are our local containers for plugs
	ZST_EXPORT static ZstPerformer* create_performer(const char * name);
	ZST_EXPORT static ZstPerformer * get_performer_by_URI(const ZstURI * uri);

	ZST_EXPORT static ZstEvent pop_event();
	ZST_EXPORT static int event_queue_size();

	ZST_EXPORT static ZstIntPlug * create_int_plug(ZstURI * uri);
    ZST_EXPORT static int destroy_plug(ZstPlug *plug);

	ZST_EXPORT static int connect_cable(const ZstURI * a, const ZstURI * b);
	ZST_EXPORT static int destroy_cable(const ZstURI * a, const ZstURI * b);


private:
    Showtime();
	Showtime(const Showtime&); // Prevent construction by copying
	Showtime& operator=(const Showtime&) {}; // Prevent assignment

	//Active runtime
	static RuntimeLanguage _runtime_language;
};
