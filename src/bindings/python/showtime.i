%module(threads="1") showtime
%{
	#include "ZstExports.h"
	#include "Showtime.h"
	class ZstURI{};
	class ZstEndpoint{};
%}
%nothread;

%include <windows.i>
%include "ZstExports.h"

%feature("pythonprepend") Showtime::join %{
	Showtime_set_runtime_language(PYTHON_RUNTIME);
%} 

%include "Showtime.h"

%template(create_int_plug) Showtime::create_plug<ZstIntPlug>;
