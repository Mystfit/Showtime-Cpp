#include <exception>
#include "entities/ZstEntityFactory.h"
#include "../ZstEventDispatcher.hpp"
#include "adaptors/ZstSynchronisableAdaptor.hpp"
#include "ZstLogging.h"

using namespace flatbuffers;

//Template instatiations
//template class ZstEventDispatcher<ZstFactoryAdaptor*>;
namespace showtime {

ZstEntityFactory::ZstEntityFactory() : 
	ZstEntityBase(),
	m_factory_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<ZstFactoryAdaptor> > >("factory events"))
{
	set_entity_type(EntityTypes_Factory);
}

ZstEntityFactory::ZstEntityFactory(const char * name) : 
	ZstEntityBase(name),
	m_factory_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<ZstFactoryAdaptor> > >("factory events"))
{
	set_entity_type(EntityTypes_Factory);
}
    
ZstEntityFactory::ZstEntityFactory(const Factory* buffer) : 
	ZstEntityBase(),
	m_factory_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<ZstFactoryAdaptor> > >("factory events"))
{
	set_entity_type(EntityTypes_Factory);
    ZstEntityFactory::deserialize_partial(buffer->factory());
	ZstEntityBase::deserialize_partial(buffer->entity());
}

ZstEntityFactory::ZstEntityFactory(const ZstEntityFactory & other) : 
	ZstEntityBase(other),
	m_factory_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<ZstFactoryAdaptor> > >("factory events"))
{
	m_creatables = other.m_creatables;
}

ZstEntityFactory::~ZstEntityFactory()
{
}

void ZstEntityFactory::add_creatable(const ZstURI & creatable_path)
{
	m_creatables.insert(creatable_path);
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
			adaptor->on_publish_entity_update(this);
		});
	} 

	factory_events()->defer([this](std::shared_ptr<ZstFactoryAdaptor> adaptor) {
		adaptor->on_creatables_updated(this);
	});
	synchronisable_events()->invoke([this](std::shared_ptr<ZstSynchronisableAdaptor>  adaptor) {
		adaptor->on_synchronisable_has_event(this);
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
		bundle.add(c);
	}
	return bundle;
}

const ZstURI & ZstEntityFactory::get_creatable_at(size_t index)
{
	if (index >= m_creatables.size())
		throw(std::out_of_range("Creatable index out of range"));
	return *std::next(m_creatables.begin(), index);
}

void ZstEntityFactory::clear_creatables()
{
	m_creatables.clear();
}

ZstEntityBase * ZstEntityFactory::create_entity(const ZstURI & creatable_path, const char * name)
{
	throw(std::runtime_error("ZstEntityFactory::create_entity not implemented"));
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

std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstFactoryAdaptor> > > & ZstEntityFactory::factory_events()
{
	return m_factory_events;
}

ZstEntityBase * ZstEntityFactory::activate_entity(ZstEntityBase * entity)
{
	if (!entity)
	{
		ZstLog::net(LogLevel::error, "Factory {} can't activate a null entity", URI().path());
		return NULL;
	}

	//Activate entity and attach listeners
	entity_events()->invoke([entity](std::shared_ptr<ZstEntityAdaptor> adaptor) {
		adaptor->on_register_entity(entity);
	});
	return entity;
}

void ZstEntityFactory::update_URI()
{
	ZstEntityBase::update_URI();
	this->update_createable_URIs();
}

void ZstEntityFactory::process_events()
{
	ZstEntityBase::process_events();
	factory_events()->process_events();
}

void ZstEntityFactory::update_createable_URIs()
{
	std::set<ZstURI> orig_uris = std::move(m_creatables);
	m_creatables.clear();
	for (auto c : orig_uris) {
		//Update creatables to match the new factory URI
		bool creatable_contains_path = c.contains(this->URI());
		if (!creatable_contains_path) {
			c = this->URI() + c.last();
		}
		m_creatables.insert(c);
	}
}
    
void ZstEntityFactory::serialize_partial(flatbuffers::Offset<FactoryData> & serialized_offset, flatbuffers::FlatBufferBuilder& buffer_builder) const
{
    // Sereialize creatables
    std::vector<std::string> creatables;
    for(auto c : m_creatables){
        creatables.push_back(c.path());
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
        m_creatables.emplace(c->c_str(), c->size());
    }
}
    
void ZstEntityFactory::deserialize(const Factory* buffer)
{
    ZstEntityFactory::deserialize_partial(buffer->factory());
    ZstEntityBase::deserialize_partial(buffer->entity());
}

}
