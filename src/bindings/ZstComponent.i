%ignore ZstComponent::read;
%ignore ZstComponent::write;
%ignore ZstComponent::set_network_interactor;

%nodefaultctor;
%feature("director") ZstComponent;
%include <entities/ZstComponent.h>
%clearnodefaultctor;
