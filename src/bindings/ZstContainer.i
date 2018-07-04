%feature("director") ZstContainer;

%inline %{
	ZstContainer* cast_to_container(ZstEntityBase * entity){
		return dynamic_cast<ZstContainer*>(entity);
	}
%}

%feature("nodirector") ZstContainer::add_adaptor;
%feature("nodirector") ZstContainer::remove_adaptor;

%nodefaultctor ZstContainer;
%include <entities/ZstContainer.h>
