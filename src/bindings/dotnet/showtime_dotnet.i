%module(directors="1") showtime
%{
    #include <Showtime.h>
    #include <ShowtimeServer.h>
%}

//static ZstEntityBundle get_performers();
%typemap(cscode) showtime %{
  public static ZstEntityBundle get_performers() {
    ZstEntityBundle bundle = new ZstEntityBundle();
    showtime.get_performers(bundle);
    return bundle;
  }
%}

%include "../preflight.i"
%include "ZstURI.i"
%include "ZstBundle.i"
%include "../showtime.i"

%csmethodmodifiers ZstURI::GetHashCode() const "public override";
%csmethodmodifiers ZstURI::ToString() const "public override";
