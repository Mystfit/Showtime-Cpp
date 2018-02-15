%feature("director") ZstComponent;

%inline %{
	ZstComponent* cast_to_component(ZstEntityBase * entity){
		return dynamic_cast<ZstComponent*>(entity);
	}
%}

%nodefaultctor ZstComponent;
%include <entities/ZstComponent.h>
