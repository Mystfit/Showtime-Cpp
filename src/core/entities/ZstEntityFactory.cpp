#include <exception>
#include <functional>
#include <showtime/entities/ZstEntityFactory.h>
#include <showtime/adaptors/ZstSynchronisableAdaptor.hpp>
#include <showtime/ZstLogging.h>
#include "../ZstEventDispatcher.hpp"

using namespace flatbuffers;

//Template instatiations
//template class ZstEventDispatcher<ZstFactoryAdaptor*>;
namespace showtime {

ZstEntityFactory::ZstEntityFactory() : 
	ZstEntityBase(),
	m_factory_events(std::make_shared<ZstEventDispatcher<ZstFactoryAdaptor> >())
{
	set_entity_type(ZstEntityType::FACTORY);
}

ZstEntityFactory::ZstEntityFactory(const char * name) : 
	ZstEntityBase(name),
	m_factory_events(std::make_shared<ZstEventDispatcher<ZstFactoryAdaptor> >())
{
	set_entity_type(ZstEntityType::FACTORY);
}
    
ZstEntityFactory::ZstEntityFactory(const Factory* buffer) : 
	ZstEntityBase(),
	m_factory_events(std::make_shared<ZstEventDispatcher<ZstFactoryAdaptor> >())
{
	set_entity_type(ZstEntityType::FACTORY);
    ZstEntityFactory::deserialize_partial(buffer->factory());
	ZstEntityBase::deserialize_partial(buffer->entity());
}

ZstEntityFactory::ZstEntityFactory(const ZstEntityFactory & other) : 
	ZstEntityBase(other),
	m_factory_events(std::make_shared<ZstEventDispatcher<ZstFactoryAdaptor> >())
{
	m_creatables = other.m_creatables;
}

ZstEntityFactory::~ZstEntityFactory()
{
	m_owned_entities.clear();
}

size_t ZstEntityFactory::num_creatables()
{
	return m_creatables.size();
}

void ZstEntityFactory::update_creatables()
{
	//Alert stage
	if (!is_proxy()) {
		this->update_createable_URIs();
		entity_events()->invoke([this](std::shared_ptr<ZstEntityAdaptor> adaptor) {
			adaptor->publish_entity_update(this, this->URI());
		});
	} 

	factory_events()->defer([this](std::shared_ptr<ZstFactoryAdaptor>& adaptor) {
		adaptor->on_creatables_updated(this);
	});
	synchronisable_events()->invoke([this](std::shared_ptr<ZstSynchronisableAdaptor> adaptor) {
		adaptor->synchronisable_has_event(this);
	});
}

void ZstEntityFactory::remove_creatable(const ZstURI & creatable_path)
{
	try {
		m_creatables.erase(creatable_path);
		//Let client library know that we want to update our creatables list
		//Do stuff
	}
	catch (std::out_of_range) {}
}

ZstURIBundle & ZstEntityFactory::get_creatables(ZstURIBundle & bundle)
{
	for (auto c : m_creatables) {
		bundle.add(c.first);
	}
	return bundle;
}

const ZstURI & ZstEntityFactory::get_creatable_at(size_t index)
{
	if (index >= m_creatables.size())
		throw(std::out_of_range("Creatable index out of range"));
	return std::next(m_creatables.begin(), index)->first;
}

void ZstEntityFactory::clear_creatables()
{
	m_creatables.clear();
}

ZstEntityBase * ZstEntityFactory::create_entity(const ZstURI & creatable_path, const char * name)
{
	auto creatable_it = m_creatables.find(creatable_path);
	if (creatable_it != m_creatables.end()) {
		auto entity = creatable_it->second(name);
		auto entity_ptr = entity.get();
		m_owned_entities.push_back(std::move(entity));
		return entity_ptr;
	} 

	Log::net(Log::Level::warn, "Could not find creatable {} in factory {}", creatable_path.path(), this->URI().path());
	return NULL;
}

void ZstEntityFactory::add_adaptor(std::shared_ptr<ZstFactoryAdaptor> adaptor)
{
	m_factory_events->add_adaptor(adaptor);
}

void ZstEntityFactory::remove_adaptor(std::shared_ptr<ZstFactoryAdaptor> adaptor)
{
	m_factory_events->remove_adaptor(adaptor);
}

std::shared_ptr<ZstEventDispatcher<ZstFactoryAdaptor> > & ZstEntityFactory::factory_events()
{
	return m_factory_events;
}

ZstEntityBase * ZstEntityFactory::activate_entity(ZstEntityBase * entity)
{
	if (!entity)
	{
		Log::net(Log::Level::error, "Factory {} can't activate a null entity", URI().path());
		return NULL;
	}

	//Activate entity and attach listeners
	entity_events()->invoke([entity](std::shared_ptr<ZstEntityAdaptor> adaptor) {
		adaptor->on_register_entity(entity);
	});
	return entity;
}

void ZstEntityFactory::update_URI(const ZstURI& original_path)
{
	ZstEntityBase::update_URI(original_path);
	this->update_createable_URIs();
}

void ZstEntityFactory::process_events()
{
	ZstEntityBase::process_events();
	factory_events()->process_events();
}

void ZstEntityFactory::update_createable_URIs()
{
	auto orig_uris = std::move(m_creatables);
	m_creatables.clear();
	for (auto c : orig_uris) {
		//Update creatables to match the new factory URI
		bool creatable_contains_path = c.first.contains(this->URI());
		ZstURI updated_c(c.first);
		if (!creatable_contains_path) {
			updated_c = this->URI() + c.first.last();
		}
		m_creatables.insert({ updated_c, c.second });
	}
}
    
void ZstEntityFactory::serialize_partial(flatbuffers::Offset<FactoryData> & serialized_offset, flatbuffers::FlatBufferBuilder& buffer_builder) const
{
    // Sereialize creatables
    std::vector<std::string> creatables;
    for(auto c : m_creatables){
        creatables.push_back(c.first.path());
    }
    
    serialized_offset = CreateFactoryData(buffer_builder, buffer_builder.CreateVectorOfStrings(creatables));
}

 
uoffset_t ZstEntityFactory::serialize(FlatBufferBuilder & buffer_builder) const
{
    Offset<EntityData> entity_offset;
	ZstEntityBase::serialize_partial(entity_offset, buffer_builder);
    
    Offset<FactoryData> factory_offset;
    serialize_partial(factory_offset, buffer_builder);
    
	return CreateFactory(buffer_builder, entity_offset, factory_offset).o;
}
    
void ZstEntityFactory::deserialize_partial(const FactoryData* buffer)
{
	if (!buffer) return;

    for(auto c : *buffer->creatables()){
		add_creatable<ZstEntityBase>(ZstURI(c->c_str(), c->size()));
    }
}
    
void ZstEntityFactory::deserialize(const Factory* buffer)
{
    ZstEntityFactory::deserialize_partial(buffer->factory());
    ZstEntityBase::deserialize_partial(buffer->entity());
}

}
