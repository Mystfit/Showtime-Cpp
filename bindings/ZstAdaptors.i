namespace showtime {
	%feature("director") ZstEventAdaptor;
	%feature("nodirector") ZstEventAdaptor::contains_event_source;
	%ignore ZstEventAdaptor::contains_event_source;

	%feature("director") ZstConnectionAdaptor;

	%feature("director") ZstHierarchyAdaptor;
	%feature("nodirector") ZstHierarchyAdaptor::activate_entity;
	%ignore ZstHierarchyAdaptor::activate_entity(ZstEntityBase, ZstTransportRequestBehaviour);
	%feature("nodirector") ZstHierarchyAdaptor::deactivate_entity;
	%ignore ZstHierarchyAdaptor::deactivate_entity(ZstEntityBase, ZstTransportRequestBehaviour);
	%feature("nodirector") ZstHierarchyAdaptor::find_entity;
	%ignore ZstHierarchyAdaptor::find_entity(ZstURI);
	%feature("nodirector") ZstHierarchyAdaptor::update_entity_URI;
	%ignore ZstHierarchyAdaptor::update_entity_URI(ZstEntityBase, ZstURI);
	%feature("nodirector") ZstHierarchyAdaptor::get_local_performer;
	%ignore ZstHierarchyAdaptor::get_local_performer;

	%feature("director") ZstSessionAdaptor;
	%feature("nodirector") ZstSessionAdaptor::get_cables;
	%ignore ZstSessionAdaptor::get_cables;
	%ignore ZstSessionAdaptor::get_cables(ZstCableBundle);
	%feature("nodirector") ZstSessionAdaptor::find_cable;
	%ignore ZstSessionAdaptor::find_cable(Cable);

	%feature("director") ZstSynchronisableAdaptor;
	%feature("director") ZstEntityAdaptor;
	%feature("director") ZstFactoryAdaptor;
}

%include <showtime/adaptors/ZstEventAdaptor.hpp> 
%include <showtime/adaptors/ZstConnectionAdaptor.hpp> 
%include <showtime/adaptors/ZstHierarchyAdaptor.hpp> 
%include <showtime/adaptors/ZstSessionAdaptor.hpp> 
%include <showtime/adaptors/ZstSynchronisableAdaptor.hpp> 
%include <showtime/adaptors/ZstEntityAdaptor.hpp> 
%include <showtime/adaptors/ZstFactoryAdaptor.hpp>
