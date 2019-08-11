%module(directors="1") showtime
%{
    #include <Showtime.h>
    #include <ShowtimeServer.h>
%}

%include "../preflight.i"

%include "../ZstBundle.i"
%include "../showtime.i"
