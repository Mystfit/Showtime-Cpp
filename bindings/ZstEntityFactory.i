namespace showtime{
	class Factory;

	%feature("director") ZstEntityFactory;
	%feature("nodirector") ZstEntityFactory::add_adaptor;
	%feature("nodirector") ZstEntityFactory::remove_adaptor;
	%feature("nodirector") ZstEntityFactory::activate_entity;
	%feature("nodirector") ZstEntityFactory::update_URI;
	%feature("nodirector") ZstEntityFactory::factory_events;
	%ignore ZstEntityFactory::factory_events;
	%feature("nodirector") ZstEntityFactory::create_entity;
	%feature("nodirector") ZstEntityFactory::serialize_partial;
	%feature("nodirector") ZstEntityFactory::serialize;
	%feature("nodirector") ZstEntityFactory::deserialize;
	%feature("nodirector") ZstEntityFactory::deserialize_partial;

	%nodefaultctor ZstEntityFactory;
	%template(ZstSerialisableFactory) ZstSerialisable<Factory, FactoryData>;

	%ignore ZstEntityFactory::ZstEntityFactory(Factory);
	%ignore ZstEntityFactory::serialize;
	%ignore ZstEntityFactory::deserialize;
	%ignore ZstEntityFactory::serialize_partial;
	%ignore ZstEntityFactory::deserialize_partial;
}


%inline %{
	showtime::ZstEntityFactory* cast_to_factory(showtime::ZstSynchronisable * synchronisable){
		return dynamic_cast<showtime::ZstEntityFactory*>(synchronisable);
	}
%}
%include <showtime/entities/ZstEntityFactory.h>
