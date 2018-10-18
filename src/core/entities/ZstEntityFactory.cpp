#include <exception>
#include <nlohmann/json.hpp>
#include <msgpack.hpp>
#include <entities/ZstEntityFactory.h>
#include "../ZstEventDispatcher.hpp"
#include <adaptors/ZstSynchronisableAdaptor.hpp>

//Template instatiations
template class ZstEventDispatcher<ZstFactoryAdaptor*>;


ZstEntityFactory::ZstEntityFactory() : ZstEntityBase("")
{
	m_factory_events = new ZstEventDispatcher<ZstFactoryAdaptor*>();
	set_entity_type(FACTORY_TYPE);
}

ZstEntityFactory::ZstEntityFactory(const char * name) : ZstEntityBase(name)
{
	m_factory_events = new ZstEventDispatcher<ZstFactoryAdaptor*>();
	set_entity_type(FACTORY_TYPE);
}

ZstEntityFactory::ZstEntityFactory(const ZstEntityFactory & other) : ZstEntityBase(other)
{
	m_factory_events = new ZstEventDispatcher<ZstFactoryAdaptor*>();
	m_creatables = other.m_creatables;
}

ZstEntityFactory::~ZstEntityFactory()
{
	m_factory_events->flush();
	m_factory_events->remove_all_adaptors();
	delete m_factory_events;
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
		entity_events()->invoke([this](ZstEntityAdaptor* adp) {adp->on_publish_entity_update(this); });
	} 

	factory_events()->defer([this](ZstFactoryAdaptor * dlg) { dlg->on_creatables_updated(this); });
	synchronisable_events()->invoke([this](ZstSynchronisableAdaptor * adp) {adp->on_synchronisable_has_event(this); });
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

void ZstEntityFactory::add_adaptor(ZstFactoryAdaptor * adaptor)
{
	m_factory_events->add_adaptor(adaptor);
}

void ZstEntityFactory::remove_adaptor(ZstFactoryAdaptor * adaptor)
{
	m_factory_events->remove_adaptor(adaptor);
}

ZstEventDispatcher<ZstFactoryAdaptor*>* ZstEntityFactory::factory_events()
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
	entity_events()->invoke([entity](ZstEntityAdaptor * adp) { adp->on_register_entity(entity); });
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

void ZstEntityFactory::write(std::stringstream & buffer) const
{	
	//Pack creatables
	ZstEntityBase::write(buffer);
	msgpack::pack(buffer, m_creatables.size());
	for (auto creatable : m_creatables) {
		msgpack::pack(buffer, creatable.path());
	}
}

void ZstEntityFactory::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack entity
	ZstEntityBase::read(buffer, length, offset);

	//Unpack creatable paths
	auto handle = msgpack::unpack(buffer, length, offset);
	int64_t num_creatables = handle.get().via.i64;
	if (num_creatables > 0) {
		for (int i = 0; i < num_creatables; ++i) {
			handle = msgpack::unpack(buffer, length, offset);
			m_creatables.emplace(handle.get().via.str.ptr, handle.get().via.str.size);
		}
	}
}

void ZstEntityFactory::write_json(json & buffer) const
{
	//Pack creatables
	ZstEntityBase::write_json(buffer);
	buffer["creatables"] = json::array();
	for (auto creatable : m_creatables) {
		buffer["creatables"].push_back(creatable.path());
	}
}

void ZstEntityFactory::read_json(const json & buffer)
{
	ZstEntityBase::read_json(buffer);
	for (auto creatable : buffer["creatables"]) {
		m_creatables.emplace(creatable.get<std::string>().c_str(), creatable.get<std::string>().size());
	}
	
}
