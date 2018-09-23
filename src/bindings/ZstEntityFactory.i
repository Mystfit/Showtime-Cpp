%feature("director") ZstEntityFactory;

%inline %{
	ZstEntityFactory* cast_to_factory(ZstEntityFactory * factory){
		return dynamic_cast<ZstEntityFactory*>(factory);
	}
%}

%feature("nodirector") ZstEntityFactory::add_adaptor;
%feature("nodirector") ZstEntityFactory::remove_adaptor;
%ignore ZstEntityFactory::activate_entity;
%ignore ZstEntityFactory::update_URI;

%nodefaultctor ZstEntityFactory;
%include <entities/ZstEntityFactory.h>
