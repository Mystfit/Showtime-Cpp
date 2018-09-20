#include <exception>
#include <msgpack.hpp>
#include <entities/ZstEntityFactory.h>
#include "../ZstEventDispatcher.hpp"

ZstEntityFactory::ZstEntityFactory() : ZstEntityBase("")
{
	set_entity_type(FACTORY_TYPE);
}

ZstEntityFactory::ZstEntityFactory(const char * name) : ZstEntityBase(name)
{
	set_entity_type(FACTORY_TYPE);
}

ZstEntityFactory::ZstEntityFactory(const ZstEntityFactory & other) : ZstEntityBase(other)
{
	m_creatables = other.m_creatables;
}

void ZstEntityFactory::add_creatable(const ZstURI & creatable_path)
{
	m_creatables.insert(creatable_path);
}

void ZstEntityFactory::remove_creatable(const ZstURI & creatable_path)
{
	try {
		m_creatables.erase(creatable_path);
	}
	catch (std::out_of_range) {}
}

void ZstEntityFactory::get_creatables(ZstURIBundle & bundle)
{
	for (auto c : m_creatables) {
		bundle.add(c);
	}
}

ZstEntityBase * ZstEntityFactory::create_entity(const ZstURI & creatable_path, const char * name)
{
	throw(std::runtime_error("ZstEntityFactory::create_entity not implemented"));
	return NULL;
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
	std::unordered_set<ZstURI, ZstURIHash> orig_uris = std::move(m_creatables);
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
	int num_creatables = handle.get().via.i64;
	if (num_creatables > 0) {
		for (int i = 0; i < num_creatables; ++i) {
			handle = msgpack::unpack(buffer, length, offset);
			m_creatables.emplace(handle.get().via.str.ptr, handle.get().via.str.size);
		}
	}
}
