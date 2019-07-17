%feature("director") ZstEventAdaptor;
%feature("director") ZstHierarchyAdaptor;
%feature("director") ZstSessionAdaptor;
%feature("director") ZstSynchronisableAdaptor;
%feature("director") ZstEntityAdaptor;
%feature("director") ZstFactoryAdaptor;

%ignore ZstSessionAdaptor::get_cables;
%ignore ZstSessionAdaptor::hierarchy;
%ignore ZstSessionAdaptor::find_cable;

%include <adaptors/ZstEventAdaptor.hpp> 
%include <adaptors/ZstHierarchyAdaptor.hpp> 
%include <adaptors/ZstSessionAdaptor.hpp> 
%include <adaptors/ZstSynchronisableAdaptor.hpp> 
%include <adaptors/ZstEntityAdaptor.hpp> 
%include <adaptors/ZstFactoryAdaptor.hpp> 
