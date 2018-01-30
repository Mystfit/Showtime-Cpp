%ignore ZstSerialisable;
%ignore ZstEntityBase::read;
%ignore ZstEntityBase::write;
%rename(create) ZstEntityBase::operator new;
%rename(destroy) ZstEntityBase::operator delete;
%newobject ZstEntityBase::create;
%delobject ZstEntityBase::destroy;

%nodefaultctor;
%include <entities/ZstEntityBase.h>
%clearnodefaultctor;
