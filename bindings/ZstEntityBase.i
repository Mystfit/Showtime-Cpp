namespace showtime {
	%ignore ZstEntityBase::set_parent;
	%ignore ZstEntityBase::get_child_cables;
	%ignore ZstEntityBase::acquire_cable_bundle;
	%ignore ZstEntityBase::release_cable_bundle;
	%ignore ZstEntityBase::update_URI;
	%ignore ZstEntityBase::set_owner;
	%nodefaultctor ZstEntityBase;
}

%include <entities/ZstEntityBase.h>
