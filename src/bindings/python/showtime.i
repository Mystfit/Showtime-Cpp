%module(directors="1", threads="1") showtime
%{
	#include "ZstExports.h"
	#include "Showtime.h"
	#include "ZstURI.h"
	#include "ZstPlug.h"
	#include "ZstEvent.h"
	#include "ZstValue.h"
	#include "ZstCable.h"
	#include "ZstCallbacks.h"
	#include "ZstCallbackQueue.h"
	#include "entities/ZstEntityBase.h"
	#include "entities/ZstPatch.h"
	#include "entities/ZstFilter.h"
	#include "entities/AddFilter.h"
	class ZstEndpoint{};
%}
%nothread;

%feature("pythonprepend") Showtime::join %{
	Showtime_set_runtime_language(PYTHON_RUNTIME);
%}

%include "showtime_common.i" 
%include "Showtime.h"
