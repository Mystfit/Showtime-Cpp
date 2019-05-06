%feature("director") ZstComponent;

%inline %{
	ZstComponent* cast_to_component(ZstSynchronisable * synchronisable){
		return dynamic_cast<ZstComponent*>(synchronisable);
	}
%}

%feature("nodirector") ZstComponent::add_adaptor;
%feature("nodirector") ZstComponent::remove_adaptor;

%nodefaultctor ZstComponent;
%include <entities/ZstComponent.h>
