%include "std_string.i"

%include <windows.i>
%include "ZstExports.h"


class ZstURI {
public:
	enum Direction {
		NONE = 0,
		IN_JACK,
		OUT_JACK
	};

	ZstURI();
	ZstURI(const char *  performer, const char *  instrument, const char *  name, Direction direction);

	static ZstURI * create(const char * performer, const char * instrument, const char * name, Direction direction);
	static void destroy(ZstURI * uri);

	%rename("performer") performer_char();
	const char * performer_char();

	%rename("instrument") instrument_char();
	const char * instrument_char();

	%rename("name") name_char();
	const char * name_char();

	const ZstURI::Direction direction();

	bool is_empty();

	%rename("to_str") to_char();
	const char * to_char() const;

	%rename("from_str") from_char();
	static ZstURI from_char(char * s);

	bool operator==(const ZstURI& other);
	bool operator!=(const ZstURI& other);
	bool operator< (const ZstURI& b) const;
};

%include "ZstPlug.h"

%feature("director") ZstEventCallback;
%nodefaultctor ZstEvent;

%include "ZstEvent.h"

%inline %{
	ZstIntPlug * convert_to_int_plug(ZstPlug * plug) {
	   return dynamic_cast<ZstIntPlug*>(plug);
	}
%}

