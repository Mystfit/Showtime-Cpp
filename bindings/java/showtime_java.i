%module(directors="1") ShowtimeJava
%{
    #include <showtime/Showtime.h>
    #include <showtime/ShowtimeServer.h>
    using namespace showtime;
%}

//static ZstEntityBundle get_performers();
%typemap(javacode) ShowtimeJava %{
  public static ZstEntityBundle get_performers() {
    ZstEntityBundle bundle = new ZstEntityBundle();
    ShowtimeJava.get_performers(bundle);
    return bundle;
  }
%}

%pragma(java) jniclasscode=%{
  static{
      try{
          System.loadLibrary("ShowtimeJavaWrapper");
      } catch(UnsatisfiedLinkError e){
          System.err.println("Native code library failed to load. \n" + e);
      }
  }
%}

%include "../preflight.i"
%include "ZstURI.i"
//%include "ZstBundle.i"
%include "../showtime.i"
