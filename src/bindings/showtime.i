%include "std_string.i"

%include ZstPlug.i
%include ZstStage.i
%include ZstURI.i

%module Showtime
%{
	#include "ZstExports.h"
	#include "Showtime.h"
%}
%include <windows.i>
%include "ZstExports.h"
//%include "Showtime.h"

class Showtime {	
public:

	~Showtime();
	void destroy();
	static Showtime & instance();

	static void join(std::string stage_address);

	std::chrono::milliseconds ping_stage();
	static ZstPerformer* create_performer(std::string name);
	static ZstPerformer * get_performer_by_name(std::string performer);

	template<typename T>
    static T* create_plug(ZstURI uri);
	void destroy_plug(ZstPlug *plug);
	std::vector<ZstURI> get_all_plug_addresses(std::string section = "", std::string instrument = "");
	static void connect_plugs(ZstURI a, ZstURI b);
};
%template(create_int_plug) Showtime::create_plug<ZstIntPlug>;


%extend Showtime {
    Showtime()
    {
		throw("Can't instantiate a Showtime class directly. Use the instance() method to access the singleton.");
        return NULL;
    }
}


