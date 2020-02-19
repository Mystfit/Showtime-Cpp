namespace showtime {
	%feature("director") ZstEventAdaptor;
	%feature("director") ZstConnectionAdaptor;
	%feature("director") ZstHierarchyAdaptor;
	%feature("director") ZstSessionAdaptor;
	%feature("nodirector") ZstSessionAdaptor::get_cables;
	%feature("nodirector") ZstSessionAdaptor::find_cable;
	%feature("director") ZstSynchronisableAdaptor;
	%feature("director") ZstEntityAdaptor;
	%feature("director") ZstFactoryAdaptor;
}

%include <adaptors/ZstEventAdaptor.hpp> 
%include <adaptors/ZstConnectionAdaptor.hpp> 
%include <adaptors/ZstHierarchyAdaptor.hpp> 
%include <adaptors/ZstSessionAdaptor.hpp> 
%include <adaptors/ZstSynchronisableAdaptor.hpp> 
%include <adaptors/ZstEntityAdaptor.hpp> 
%include <adaptors/ZstFactoryAdaptor.hpp>
