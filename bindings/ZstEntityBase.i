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

	//%ignore ZstEntityBase::ZstEntityBase(EntityData);
	%ignore ZstEntityBase::serialize;
	%ignore ZstEntityBase::deserialize;
	%ignore ZstEntityBase::serialize_partial;
	%ignore ZstEntityBase::deserialize_partial;
	//%feature("nodirector") ZstEntityBase::factory_events;
}

%include <entities/ZstEntityBase.h>
