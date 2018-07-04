%feature("director") ZstComponent;

%inline %{
	ZstComponent* cast_to_component(ZstEntityBase * entity){
		return dynamic_cast<ZstComponent*>(entity);
	}
%}

%feature("nodirector") ZstComponent::add_adaptor;
%feature("nodirector") ZstComponent::remove_adaptor;

%nodefaultctor ZstComponent;
%include <entities/ZstComponent.h>
