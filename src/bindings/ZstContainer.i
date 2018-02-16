%feature("director") ZstContainer;

%inline %{
	ZstContainer* cast_to_container(ZstEntityBase * entity){
		return dynamic_cast<ZstContainer*>(entity);
	}
%}

%nodefaultctor ZstContainer;
%include <entities/ZstContainer.h>
