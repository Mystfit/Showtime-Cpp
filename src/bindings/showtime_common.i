%include "std_string.i"
%include <windows.i>
%include "ZstExports.h"
%include "ZstConstants.h"

// Importable classes
%rename(add) ZstURI::operator+;
%rename(less_than) ZstURI::operator<;
%ignore ZstURI::operator==;
%ignore ZstURI::operator=;
%include "ZstURI.h"

%include "ZstPlug.h"

%rename(equal) ZstEvent::operator==;
%ignore ZstEvent::operator!=;
%include "ZstEvent.h"

%rename(equal) ZstCable::operator==;
%ignore ZstCable::operator!=;
%include "ZstCable.h"

%extend ZstPlug{
    std::string char_at(size_t position) {
        char * buf = new char[$self->size_at(position) + 1]();
        $self->char_at(buf,position);
        std::string s = std::string(buf);
        delete[] buf;
        return s;
    }
};

// Callbacks
%feature("director") ZstPlugDataEventCallback;
%feature("director") ZstPlugEventCallback;
%feature("director") ZstEntityEventCallback;
%feature("director") ZstCableEventCallback;
%feature("director") ZstComputeCallback;
%include "ZstCallbacks.h"

// Callback queue templates
%include "ZstCallbackQueue.h"
%template(ZstInputPlugCallbackQueue) ZstCallbackQueue<ZstPlugDataEventCallback, ZstInputPlug*>;
%template(ZstEntityEventCallbackQueue) ZstCallbackQueue<ZstEntityEventCallback, ZstEntityBase*>;
%template(ZstCableCallbackQueue) ZstCallbackQueue<ZstCableEventCallback, ZstCable>;
%template(ZstPlugCallbackQueue) ZstCallbackQueue<ZstPlugEventCallback, ZstURI>;

// Entities
%nodefaultctor;
%feature("director") ZstEntityBase;
%feature("director") ZstComponent;
%feature("director") ZstFilter;
%rename(create) ZstEntityBase::operator new;
%rename(destroy) ZstEntityBase::operator delete;
%newobject ZstEntityBase::create;
%delobject ZstEntityBase::destroy;

%include "entities/ZstEntityBase.h"
%ignore ZstComponent::get_plug_by_URI;
%include "entities/ZstComponent.h"
%include "entities/ZstFilter.h"
%clearnodefaultctor;

// Entities
%nodefaultctor;
%include "entities/AddFilter.h"
%clearnodefaultctor;

