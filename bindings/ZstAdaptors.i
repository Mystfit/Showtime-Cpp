%feature("director") ZstEventAdaptor;
%feature("director") ZstConnectionAdaptor;
%feature("director") ZstHierarchyAdaptor;
%feature("director") ZstSessionAdaptor;
%feature("director") ZstSynchronisableAdaptor;
%feature("director") ZstEntityAdaptor;
%feature("director") ZstFactoryAdaptor;

%ignore ZstSessionAdaptor::get_cables;
%ignore ZstSessionAdaptor::hierarchy;
%ignore ZstSessionAdaptor::find_cable;

%shared_ptr(ZstConnectionAdaptor)
%shared_ptr(ZstEntityAdaptor)
%shared_ptr(ZstFactoryAdaptor)
%shared_ptr(ZstHierarchyAdaptor)
%shared_ptr(ZstSessionAdaptor)
%shared_ptr(ZstSynchronisableAdaptor)

%include <adaptors/ZstEventAdaptor.hpp> 
%include <adaptors/ZstConnectionAdaptor.hpp> 
%include <adaptors/ZstHierarchyAdaptor.hpp> 
%include <adaptors/ZstSessionAdaptor.hpp> 
%include <adaptors/ZstSynchronisableAdaptor.hpp> 
%include <adaptors/ZstEntityAdaptor.hpp> 
%include <adaptors/ZstFactoryAdaptor.hpp> 
