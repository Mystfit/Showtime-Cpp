namespace showtime {
	%feature("director") ZstComponent;

	%feature("nodirector") ZstComponent::add_adaptor;
	%feature("nodirector") ZstComponent::remove_adaptor;
	%feature("nodirector") ZstComponent::serialize_partial;
	%feature("nodirector") ZstComponent::serialize;
	%feature("nodirector") ZstComponent::deserialize;
	%feature("nodirector") ZstComponent::deserialize_partial;
	%nodefaultctor ZstComponent;
	%template(ZstSerialisableComponent) ZstSerialisable<Component, ComponentData>;

	//%ignore ZstComponent::ZstComponent(Component);
	%ignore ZstComponent::serialize;
	%ignore ZstComponent::deserialize;
	%ignore ZstComponent::serialize_partial;
	%ignore ZstComponent::deserialize_partial;
}

%inline %{
	showtime::ZstComponent* cast_to_component(showtime::ZstSynchronisable * synchronisable){
		return dynamic_cast<showtime::ZstComponent*>(synchronisable);
	}
%}

%include <entities/ZstComponent.h>
