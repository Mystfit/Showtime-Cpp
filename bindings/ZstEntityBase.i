%include "ZstAdaptorPointers.i"

namespace showtime {
	%feature("director") ZstEntityBase;

	%ignore ZstEntityBase::set_parent;
	%ignore ZstEntityBase::get_child_cables;
	%ignore ZstEntityBase::acquire_cable_bundle;
	%ignore ZstEntityBase::release_cable_bundle;
	%ignore ZstEntityBase::update_URI;
	%ignore ZstEntityBase::set_owner;
	%ignore ZstEntityBase::entity_events;
	%nodefaultctor ZstEntityBase;

	%template(ZstSerialisableEntity) ZstSerialisable<Entity, EntityData>;

	%ignore ZstEntityBase::serialize;
	%ignore ZstEntityBase::deserialize;
	%ignore ZstEntityBase::serialize_partial;
	%ignore ZstEntityBase::deserialize_partial;
}

%feature("nodirector") showtime::ZstEntityBase::add_adaptor;
%feature("nodirector") showtime::ZstEntityBase::remove_adaptor;

%rename(add_entity_adaptor) showtime::ZstEntityBase::add_adaptor(std::shared_ptr<ZstEntityAdaptor>);
%rename(remove_entity_adaptor) showtime::ZstEntityBase::remove_adaptor(std::shared_ptr<ZstEntityAdaptor>);

%ignore showtime::ZstEntityBase::add_adaptor(std::shared_ptr<ZstHierarchyAdaptor>);
%ignore showtime::ZstEntityBase::add_adaptor(std::shared_ptr<ZstSessionAdaptor>);
%ignore showtime::ZstEntityBase::remove_adaptor(std::shared_ptr<ZstHierarchyAdaptor>);
%ignore showtime::ZstEntityBase::remove_adaptor(std::shared_ptr<ZstSessionAdaptor>);


%include <showtime/entities/ZstEntityBase.h>
