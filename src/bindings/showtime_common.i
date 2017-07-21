%include "std_string.i"
%include <windows.i>
%include "ZstExports.h"

// ZstURI class definition
class ZstURI {
public:
    ZstURI();
    ZstURI(const char *  performer, const char *  instrument, const char *  name);

    %rename("performer") performer_char();
    const char * performer_char();

    %rename("instrument") instrument_char();
    const char * instrument_char();

    %rename("name") name_char();
    const char * name_char();

    bool is_empty();

    %rename("to_str") to_char();
    const char * to_char() const;

    %rename("from_str") from_char();
    static ZstURI from_char(char * s);

    bool operator==(const ZstURI& other);
    bool operator!=(const ZstURI& other);
    bool operator< (const ZstURI& b) const;
};

// Importable classes
%include "ZstPlug.h"
%include "ZstEvent.h"
%include "ZstCable.h"

// Callbacks
%feature("director") ZstEventCallback;
%feature("director") ZstPlugDataEventCallback;
%feature("director") ZstPlugEventCallback;
%feature("director") ZstPerformerEventCallback;
%feature("director") ZstCableEventCallback;
%include "ZstCallbacks.h"

// Callback queue templates
%include "ZstCallbackQueue.h"
%template(ZstEventCallbackQueue) ZstCallbackQueue<ZstEventCallback, ZstEvent>;
%template(ZstPerformerCallbackQueue) ZstCallbackQueue<ZstPerformerEventCallback, ZstURI>;
%template(ZstCableCallbackQueue) ZstCallbackQueue<ZstCableEventCallback, ZstCable>;
%template(ZstPlugCallbackQueue) ZstCallbackQueue<ZstPlugEventCallback, ZstURI>;
%template(ZstInputPlugCallbackQueue) ZstCallbackQueue<ZstPlugDataEventCallback, ZstInputPlug*>;

// ZstValue extensions
%nodefaultctor ZstValue;
%extend ZstValue{
    std::string char_at(size_t position) {
        char * buf = new char[$self->size_at(position) + 1]();
        $self->char_at(buf,position);
        std::string s = std::string(buf);
        delete[] buf;
        return s;
    }
};
%include "ZstValue.h"
%ignore ZstValue::char_at;

