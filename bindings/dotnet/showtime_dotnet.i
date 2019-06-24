%module(directors="1") showtime
%{
    #include <Showtime.h>
    #include <ShowtimeServer.h>
%}

%include "../preflight.i"
%include "ZstURI.i"
%include "ZstBundle.i"
%include "ZstSynchronisable.i"
%include "ZstEntityBase.i"
%include "ZstSerialisable.i"

%include "showtime_api_extensions.i"
%include "../showtime.i"
