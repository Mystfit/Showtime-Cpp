%include "std_string.i"
%include <windows.i>
%include "ZstExports.h"

// Importable classes
%include "ZstURI.h"
%include "ZstPlug.h"
%include "ZstEvent.h"
%include "ZstCable.h"

// Entity bases
%feature("director") ZstEntityBase;
%include "entities\ZstEntityBase.h"

%nodefaultctor;
%feature("director") ZstFilter;
%feature("director") ZstPatch;
%include "entities\ZstFilter.h"
%include "entities\ZstPatch.h"
%clearnodefaultctor;

// Entities
%nodefaultctor;
%include "entities\AddFilter.h"
%clearnodefaultctor;

// Callbacks
%feature("director") ZstEventCallback;
%feature("director") ZstPlugDataEventCallback;
%feature("director") ZstPlugEventCallback;
%feature("director") ZstEntityEventCallback;
%feature("director") ZstCableEventCallback;
%include "ZstCallbacks.h"

// Callback queue templates
%include "ZstCallbackQueue.h"
%template(ZstEventCallbackQueue) ZstCallbackQueue<ZstEventCallback, ZstEvent>;
%template(ZstEntityEventCallbackQueue) ZstCallbackQueue<ZstEntityEventCallback, ZstURI>;
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

