%module(directors="1") showtime
%{
	#include <Showtime.h>
	#include <ShowtimeServer.h>
	#include <schemas/graph_types_generated.h>
	using namespace showtime;
%}

%include "../preflight.i"
%include "ZstBundle.i"
%include "ZstSynchronisable.i"
%include "ZstSerialisable.i"
%include "ZstEntityBase.i"
%include "ZstComponent.i"

%include "showtime_api_extensions.i"
%include "../showtime.i"
%include "ZstURI.i"
