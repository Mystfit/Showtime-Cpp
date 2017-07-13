%module(directors="1") showtime_dotnet
%{
	#include "ZstExports.h"
	#include "Showtime.h"
	#include "ZstURI.h"
	#include "ZstPlug.h"
	#include "ZstEvent.h"
	#include "ZstValue.h"
%}

%include "../showtime_common.i"

%include "Showtime.h"
