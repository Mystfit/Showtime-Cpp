%module(directors="1", threads="1") showtime
%{
	#include "ZstExports.h"
	#include "Showtime.h"
	#include "ZstURI.h"
	#include "ZstPlug.h"
	class ZstEndpoint{};
%}
%nothread;

%feature("pythonprepend") Showtime::join %{
	Showtime_set_runtime_language(PYTHON_RUNTIME);
%}


%include <std_pair.i>
%include <std_vector.i>
%template() std::pair<ZstURI,ZstURI>;
%template(ConnectionPair) std::vector<std::pair<ZstURI,ZstURI> >;

%include "showtime_common.i" 

%thread;
%include "Showtime.h"
%nothread;
