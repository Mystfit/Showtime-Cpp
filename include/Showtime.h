#pragma once

#include <chrono>
#include <vector>
#include <memory>
#include "ZstExports.h"

class ZstPlug;
class ZstIntPlug;
class ZstPerformer;
class ZstURI;
class ZstEndpoint;
class PlugEvent;

enum RuntimeLanguage {
	NATIVE_RUNTIME,
	PYTHON_RUNTIME,
	DOTNET_RUNTIME
};

class Showtime
{
public:
	ZST_EXPORT ~Showtime();
	ZST_EXPORT static ZstEndpoint & endpoint();
	ZST_EXPORT static void destroy();

	ZST_EXPORT static void set_runtime_language(RuntimeLanguage runtime);
	ZST_EXPORT static RuntimeLanguage get_runtime_language();

	//Init
	ZST_EXPORT static void init();
    ZST_EXPORT static void join(const char * stage_address);

    //Stage methods
    ZST_EXPORT static std::chrono::milliseconds ping_stage();

	//Performers are our local containers for plugs
	ZST_EXPORT static ZstPerformer* create_performer(const char * name);
	ZST_EXPORT static ZstPerformer * get_performer_by_name(const char * performer);

	ZST_EXPORT static PlugEvent pop_plug_event();
	ZST_EXPORT static int plug_event_queue_size();

	template<typename T>
	static T* create_plug(ZstURI * uri) {
		return Showtime::endpoint().create_plug<T>(uri);
	}
	ZST_EXPORT static ZstIntPlug * create_int_plug(ZstURI * uri);
    ZST_EXPORT static void destroy_plug(ZstPlug *plug);
    ZST_EXPORT static std::vector<ZstURI> get_all_plug_addresses(const char * section = "", const char * instrument = "");
	ZST_EXPORT static void connect_plugs(const ZstURI * a, const ZstURI * b);

private:
    Showtime();
	Showtime(const Showtime&); // Prevent construction by copying
	Showtime& operator=(const Showtime&) {}; // Prevent assignment

	//Active runtime
	static RuntimeLanguage _runtime_language;
};


