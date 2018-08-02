%feature("director") ZstContainer;

%inline %{
	ZstContainer* cast_to_container(ZstSynchronisable * synchronisable){
		return dynamic_cast<ZstContainer*>(synchronisable);
	}
%}

%feature("nodirector") ZstContainer::add_adaptor;
%feature("nodirector") ZstContainer::remove_adaptor;

%nodefaultctor ZstContainer;
%include <entities/ZstContainer.h>
