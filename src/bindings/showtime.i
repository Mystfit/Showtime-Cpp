%module(directors="1") showtime
%{
	#include <Showtime.h>
%}

%include <ZstExports.h>
%include <ZstConstants.h>

%include "ZstURI.i"
%include "ZstSynchronisable.i"
%include "ZstEntityBase.i"
%include "ZstPlug.i"
%include "ZstCable.i"
%include "ZstComponent.i"
%include "ZstContainer.i"
%include "ZstPerformer.i"
%include "ZstEvents.i"

%rename("%(strip:[zst_])s") "";
%include <Showtime.h>
//%include "showtime_api.i"

//Entities
%include "AddFilter.i"
