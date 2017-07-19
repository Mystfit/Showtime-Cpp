%include "std_string.i"
%include <windows.i>
%include "ZstExports.h"

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

%feature("director") ZstInputPlugEventCallback;
%include "ZstPlug.h"

%feature("director") ZstEventCallback;
%include "ZstEvent.h"

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

%include "ZstCallbackQueue.h"
%template(ZstEventCallbackQueue) ZstCallbackQueue<ZstEventCallback, ZstEvent>;
%template(ZstPerformerCallbackQueue) ZstCallbackQueue<ZstPerformerEventCallback, ZstURI>;
%template(ZstCableCallbackQueue) ZstCallbackQueue<ZstCableEventCallback, ZstCable>;
%template(ZstPlugCallbackQueue) ZstCallbackQueue<ZstPlugEventCallback, ZstURI>;
%template(ZstInputPlugCallbackQueue) ZstCallbackQueue<ZstInputPlugEventCallback, ZstInputPlug*>;

class ZstInputPlug : public ZstPlug {
public:
    ZstInputPlug(ZstURI uri, ZstValueType t);
    ~ZstInputPlug();
    void recv(ZstValue * val);
    ZstCallbackQueue<ZstInputPlugEventCallback, ZstInputPlug*> * input_events();
};