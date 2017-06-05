%include "std_string.i"

%module(directors="1", threads="1") Showtime
%{
	#define SWIG_FILE_WITH_INIT
	#include "ZstExports.h"
	#include "Showtime.h"
	//#include "ZstURI.h"
	//#include "ZstPlug.h"
	//#include "ZstStage.h"
%}

%include <windows.i>
#%include "ZstExports.h"

%feature("pythonprepend") Showtime::join %{
	Showtime_instance().set_runtime_language(Showtime.PYTHON_RUNTIME);
%} 

%nodefaultctor;
class Showtime {
public:
	~Showtime();
	void destroy_singleton();
	static void destroy();
	static Showtime & instance();
    static void join(std::string stage_address);

	static ZstPerformer* create_performer(std::string name);
	static ZstPerformer * get_performer_by_name(std::string performer);
	static void connect_plugs(ZstURI a, ZstURI b);
	
	template<typename T>
    static T* create_plug(ZstURI uri);
    
    void destroy_plug(ZstPlug *plug);


	enum RuntimeLanguage {
		NATIVE,
		PYTHON_RUNTIME,
		DOTNET_RUNTIME
	};
	
	void set_runtime_language(RuntimeLanguage runtime);
	RuntimeLanguage get_runtime_language();
    std::vector<ZstURI> get_all_plug_addresses(std::string section = "", std::string instrument = "");
};
%clearnodefaultctor;
%template(create_int_plug) Showtime::create_plug<ZstIntPlug>;


%include ZstStage.i
%include ZstURI.i

%feature("director") ZstPlug;
%feature("director") PlugCallback;
%include ZstPlug.i
