%include "std_string.i"

%module(directors="1", threads="1") Showtime
%{
	#include "ZstExports.h"
	#include "ZstURI.h"
	#include "ZstPlug.h"
	#include "ZstStage.h"
	#include "Showtime.h"
%}

%include <windows.i>
%include "ZstExports.h"
%include ZstStage.h
%include ZstURI.h

%feature("director") ZstPlug;
%feature("director") PlugCallback;
%include ZstPlug.i

%feature("pythonprepend") Showtime::join %{
	Showtime_instance().set_runtime_language(Showtime.PYTHON_RUNTIME);
%} 
%include Showtime.h
%template(create_int_plug) Showtime::create_plug<ZstIntPlug>;

