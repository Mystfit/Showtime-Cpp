%ignore ZstContainer::read;
%ignore ZstContainer::write;
%ignore ZstContainer::set_network_interactor;

%nodefaultctor;
%include <entities/ZstContainer.h>
%clearnodefaultctor;
