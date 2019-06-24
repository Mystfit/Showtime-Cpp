%ignore ZstEntityBase::set_parent;
%ignore ZstEntityBase::get_child_cables;
%ignore ZstEntityBase::acquire_cable_bundle;
%ignore ZstEntityBase::release_cable_bundle;
%ignore ZstEntityBase::update_URI;

%rename(create) ZstEntityBase::operator new;
%rename(destroy) ZstEntityBase::operator delete;
%newobject ZstEntityBase::create;
%delobject ZstEntityBase::destroy;
%nodefaultctor ZstEntityBase;

%include <entities/ZstEntityBase.h>
