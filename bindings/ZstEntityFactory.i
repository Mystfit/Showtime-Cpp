namespace showtime{
	class Factory;

	%feature("director") ZstEntityFactory;
	%feature("nodirector") ZstEntityFactory::add_adaptor;
	%feature("nodirector") ZstEntityFactory::remove_adaptor;
	%feature("nodirector") ZstEntityFactory::activate_entity;
	%feature("nodirector") ZstEntityFactory::update_URI;
	%feature("nodirector") ZstEntityFactory::factory_events;
	%feature("nodirector") ZstEntityFactory::create_entity;
	%feature("nodirector") ZstEntityFactory::serialize_partial const;
	%feature("nodirector") ZstEntityFactory::serialize const;
	%feature("nodirector") ZstEntityFactory::deserialize;
	%feature("nodirector") ZstEntityFactory::deserialize_partial;

	%nodefaultctor ZstEntityFactory;
	%ignore Factory;

	%template(ZstSerialisableFactory) ZstSerialisable<Factory, FactoryData>;
}

%inline %{
	showtime::ZstEntityFactory* cast_to_factory(showtime::ZstSynchronisable * synchronisable){
		return dynamic_cast<showtime::ZstEntityFactory*>(synchronisable);
	}
%}
%include <entities/ZstEntityFactory.h>
