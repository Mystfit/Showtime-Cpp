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

%feature("director") PlugCallback;
%include "ZstPlug.h"

%inline %{
	ZstIntPlug * convert_to_int_plug(ZstPlug * plug) {
	   return dynamic_cast<ZstIntPlug*>(plug);
	}
%}

