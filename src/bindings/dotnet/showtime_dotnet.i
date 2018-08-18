%module(directors="1") showtime
%{
	#include <Showtime.h>
%}

%extend ZstURI
{
    const int ZstURI::GetHashCode() const
    {
        return static_cast<int>(ZstURIHash{}(*self));
    }

    const char * ZstURI::ToString() const
    {
    	return self->path();
    }
};

%include "../showtime.i"

%csmethodmodifiers ZstURI::GetHashCode() const "public override";
%csmethodmodifiers ZstURI::ToString() const "public override";
