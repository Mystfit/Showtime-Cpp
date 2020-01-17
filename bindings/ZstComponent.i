namespace showtime {
	class Component;
	%feature("director") ZstComponent;

	%feature("nodirector") ZstComponent::add_adaptor;
	%feature("nodirector") ZstComponent::remove_adaptor;
	%feature("nodirector") ZstComponent::serialize_partial const;
	%feature("nodirector") ZstComponent::serialize const;
	%feature("nodirector") ZstComponent::deserialize;
	%feature("nodirector") ZstComponent::deserialize_partial;
	%nodefaultctor ZstComponent;
	%ignore Component;
}

%inline %{
	showtime::ZstComponent* cast_to_component(showtime::ZstSynchronisable * synchronisable){
		return dynamic_cast<showtime::ZstComponent*>(synchronisable);
	}
%}

%include <entities/ZstComponent.h>
