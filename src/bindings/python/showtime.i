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

	%rename("name_char") name_char();
	const char * name_char();

	const ZstURI::Direction direction();

	std::string to_str() const;
	static ZstURI from_str(char * s);

	bool operator==(const ZstURI& other);
	bool operator!=(const ZstURI& other);
	bool operator< (const ZstURI& b) const;
};

%thread;
%nodefaultctor ZstPlug;
%include "ZstPlug.h"
%nothread;

%feature("pythonprepend") Showtime::join %{
	Showtime_set_runtime_language(PYTHON_RUNTIME);
%} 

%include "Showtime.h"
%template(create_int_plug) Showtime::create_plug<ZstIntPlug>;

