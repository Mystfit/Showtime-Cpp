%module(directors="1", threads="1") showtime
%{
	#include "ZstExports.h"
	#include "Showtime.h"
	#include "ZstURI.h"
	#include "ZstPlug.h"
	#include "ZstPerformer.h"
	#include "ZstEvent.h"
	#include "ZstValue.h"
	#include "ZstCable.h"
	#include "ZstCallbackQueue.h"
	class ZstEndpoint{};
%}
%nothread;

%feature("pythonprepend") Showtime::join %{
	Showtime_set_runtime_language(PYTHON_RUNTIME);
%}

%include "showtime_common.i" 
%include "Showtime.h"
