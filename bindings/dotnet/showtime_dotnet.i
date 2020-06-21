%module(directors="1") showtime
%{
	#include <showtime/Showtime.h>
	#include <showtime/ShowtimeServer.h>
	#include <showtime/schemas/graph_types_generated.h>
	using namespace showtime;
%}

%include "../preflight.i"
%include "ZstBundle.i"
%include "ZstSynchronisable.i"
%include "ZstSerialisable.i"
%include "ZstEntityBase.i"
%include "ZstComponent.i"

%include "api_extensions.i"
%include "../showtime.i"
%include "ZstURI.i"
