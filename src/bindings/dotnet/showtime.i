%module(directors="1") showtime_dotnet
%{
	#include "ZstExports.h"
	#include "Showtime.h"
	#include "ZstURI.h"
	#include "ZstPlug.h"
	#include "ZstPerformer.h"
	#include "ZstEvent.h"
	#include "ZstValue.h"
	#include "ZstCable.h"
	#include "ZstCallbacks.h"
	#include "ZstCallbackQueue.h"
%}

%include "../showtime_common.i"

%include "Showtime.h"
