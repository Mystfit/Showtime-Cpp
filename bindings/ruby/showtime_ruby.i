%module(directors="1") showtime
%{
    #include <showtime/Showtime.h>
    #include <showtime/ShowtimeServer.h>
    using namespace showtime;
%}

%include "../showtime.i"
%include "../ZstBundle.i"
