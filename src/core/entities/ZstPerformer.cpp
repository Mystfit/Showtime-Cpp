#include <exception>
#include <msgpack.hpp>
#include <entities/ZstPerformer.h>
#include "../ZstEventDispatcher.hpp"

using namespace std;

ZstPerformer::ZstPerformer() : 
	ZstContainer(),
	m_heartbeat_active(false),
	m_missed_heartbeats(0)
{
	set_entity_type(PERFORMER_TYPE);
}

ZstPerformer::ZstPerformer(const char * name) :
	ZstContainer("", name),
	m_heartbeat_active(false),
	m_missed_heartbeats(0)
{
	set_entity_type(PERFORMER_TYPE);
}

ZstPerformer::ZstPerformer(const ZstPerformer & other) : ZstContainer(other)
{
	m_heartbeat_active = other.m_heartbeat_active;
	m_missed_heartbeats = other.m_missed_heartbeats;
	
	for (auto f : other.m_factories) {
		add_factory(new ZstEntityFactory(*(f.second)));
	}
}

ZstPerformer::~ZstPerformer()
{
	for (auto f : m_factories) {
		delete f.second;
	}
}

void ZstPerformer::set_heartbeat_active()
{
	m_heartbeat_active = true;
	m_missed_heartbeats = 0;
}

void ZstPerformer::clear_active_hearbeat() {
	m_heartbeat_active = false;
}

bool ZstPerformer::get_active_heartbeat()
{
	return m_heartbeat_active;
}

void ZstPerformer::set_heartbeat_inactive()
{
	m_missed_heartbeats++;
}

int ZstPerformer::get_missed_heartbeats()
{
	return m_missed_heartbeats;
}

void ZstPerformer::add_child(ZstEntityBase * entity)
{
	if (strcmp(entity->entity_type(), FACTORY_TYPE) == 0) {
		add_factory(static_cast<ZstEntityFactory*>(entity));
	}
	else {
		ZstContainer::add_child(entity);
	}
}

void ZstPerformer::remove_child(ZstEntityBase * entity)
{
	if (strcmp(entity->entity_type(), FACTORY_TYPE) == 0) {
		remove_factory(static_cast<ZstEntityFactory*>(entity));
	}
	else {
		ZstContainer::remove_child(entity);
	}
}

void ZstPerformer::add_factory(ZstEntityFactory * factory)
{
	if (is_destroyed()) return;

	ZstEntityBase::add_child(factory);
	m_factories[factory->URI()] = factory;
}

void ZstPerformer::remove_factory(ZstEntityFactory * factory)
{
	auto f = m_factories.find(factory->URI());
	if (f != m_factories.end()) {
		m_factories.erase(f);
	}

	ZstEntityBase::remove_child(factory);
}

ZstEntityBundle & ZstPerformer::get_factories(ZstEntityBundle & bundle)
{
	for (auto f : m_factories) {
		bundle.add(f.second);
	}
	return bundle;
}

ZstEntityFactoryBundle & ZstPerformer::get_factories(ZstEntityFactoryBundle & bundle)
{
	for (auto f : m_factories) {
		bundle.add(f.second);
	}
	return bundle;
}

void ZstPerformer::write(std::stringstream & buffer) const
{
	//Pack entity
	ZstContainer::write(buffer);
	
	//Pack children
	msgpack::pack(buffer, m_factories.size());
	for (auto factory : m_factories) {
		factory.second->write(buffer);
	}
}

void ZstPerformer::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack entity
	ZstContainer::read(buffer, length, offset);

	//Unpack factories
	auto handle = msgpack::unpack(buffer, length, offset);
	int num_factories = static_cast<int>(handle.get().via.i64);
	for (int i = 0; i < num_factories; ++i) {
		ZstEntityFactory * factory = new ZstEntityFactory();
		factory->read(buffer, length, offset);
		m_factories[factory->URI()] = factory;
	}
}
