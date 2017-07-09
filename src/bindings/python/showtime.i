%module(directors="1", threads="1") showtime
%{
	#include "ZstExports.h"
	#include "Showtime.h"
	#include "ZstURI.h"
	#include "ZstPlug.h"
	#include "ZstEvent.h"
	class ZstEndpoint{};
%}
%nothread;

%feature("pythonprepend") Showtime::join %{
	Showtime_set_runtime_language(PYTHON_RUNTIME);
%}


%include <std_vector.i>
%template(EventList) std::vector<ZstEvent>;

%include "showtime_common.i" 
%include "Showtime.h"
