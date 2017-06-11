%module(directors="1", threads="1") showtime
%{
	#include "ZstExports.h"
	#include "Showtime.h"
	#include "ZstURI.h"
	#include "ZstPlug.h"
	class ZstEndpoint{};
%}
%nothread;

%include "std_string.i"
%include <std_shared_ptr.i>
%include <windows.i>
%include "ZstExports.h"

%nodefaultctor ZstURI;
%thread;
class ZstURI {
public:
	enum Direction {
		IN_JACK = 0,
		OUT_JACK
	};

	static ZstURI * create(const char * performer, const char * instrument, const char * name, Direction direction);
	static void destroy(ZstURI * uri);

	%rename("performer") performer_char();
	const char * performer_char();

	%rename("instrument") instrument_char();
	const char * instrument_char();

	%rename("name") name_char();
	const char * name_char();

	const ZstURI::Direction direction();

	%rename("to_str") to_char();
	const char * to_char() const;
	static ZstURI from_str(char * s);

	bool operator==(const ZstURI& other);
	bool operator!=(const ZstURI& other);
	bool operator< (const ZstURI& b) const;
};
%nothread;


%feature("pythonprepend") ZstPlug::attach_recv_callback(PlugCallback * callback) %{
   if len(args) == 1 and (not isinstance(args[0], PlugCallback) and callable(args[0])):
      class CallableWrapper(PlugCallback):
         def __init__(self, f):
            super(CallableWrapper, self).__init__()
            self.f_ = f
         def run(self, obj):
            self.f_(obj)

      args = tuple([CallableWrapper(args[0])])
      args[0].__disown__()
   elif len(args) == 1 and isinstance(args[0], PlugCallback):
      args[0].__disown__()
%}

%feature("director") PlugCallback;
%thread;
class PlugCallback {
public:
	PlugCallback();
	virtual ~PlugCallback();
    virtual void run(ZstPlug * plug);
};

%nodefaultctor ZstIntPlug;
class ZstIntPlug {
public:
	void fire(int value);
	void recv(msgpack::object object);
	void attach_recv_callback(PlugCallback * callback);
	int get_value();	
};
%nothread;

%feature("pythonprepend") Showtime::join %{
	Showtime_set_runtime_language(PYTHON_RUNTIME);
%} 

%include "Showtime.h"

