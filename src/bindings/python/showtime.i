%module(threads="1") showtime
%{
	#include "ZstExports.h"
	#include "Showtime.h"
	#include "ZstURI.h"
	//class ZstURI{};
	class ZstEndpoint{};
%}
%nothread;

%include "std_string.i"
%include <windows.i>
%include "ZstExports.h"

%thread;
%include "ZstURI.h"
%nothread;

%feature("pythonprepend") Showtime::join %{
	Showtime_set_runtime_language(PYTHON_RUNTIME);
%} 

%include "Showtime.h"
%template(create_int_plug) Showtime::create_plug<ZstIntPlug>;

