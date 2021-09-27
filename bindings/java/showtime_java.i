%module(directors="1") showtime
%{
  #include <showtime/ShowtimeClient.h>
  #include <showtime/ShowtimeServer.h>
  #include <showtime/ZstLogging.h>
  #include <showtime/schemas/messaging/graph_types_generated.h>
  using namespace showtime;
%}
%feature("autodoc", "1");

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
