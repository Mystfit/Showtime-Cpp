%ignore ZstContainer::read;
%ignore ZstContainer::write;
%ignore ZstContainer::set_network_interactor;

%nodefaultctor;
%feature("director") ZstContainer;
%include <entities/ZstContainer.h>
%clearnodefaultctor;
