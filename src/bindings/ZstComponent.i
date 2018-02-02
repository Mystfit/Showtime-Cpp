%nodefaultctor;
%feature("director") ZstComponent;
%ignore ZstComponent::read;
%ignore ZstComponent::write;
%ignore ZstComponent::set_parent;
%include <entities/ZstComponent.h>
%clearnodefaultctor;
