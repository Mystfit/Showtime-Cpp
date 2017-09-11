%module(directors="1") showtime
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
	#include "entities/ZstComponent.h"
	#include "entities/ZstFilter.h"
	#include "entities/AddFilter.h"
%}

%include "../showtime_common.i" 
%include "Showtime.h"
