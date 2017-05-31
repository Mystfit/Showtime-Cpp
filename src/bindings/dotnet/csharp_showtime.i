%include "std_string.i"

//%include ../ZstURI.i

%module showtime_dotnet
%{
	#include "ZstExports.h"
	#include "ZstURI.h"
	#include "Showtime.h"
%}

%include <windows.i>
%include "ZstExports.h"
%include "Showtime.h"
%template(create_int_plug) Showtime::create_plug<ZstIntPlug>;
