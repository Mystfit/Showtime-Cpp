#include <exception>
#include "entities/ZstPerformer.h"

using namespace std;

ZstPerformer::ZstPerformer() : 
	ZstContainer(),
	m_heartbeat_active(false),
	m_missed_heartbeats(0),
	m_address("")
{
	set_entity_type(PERFORMER_TYPE);
}

ZstPerformer::ZstPerformer(const char * name, const char * client_ip) :
	ZstContainer("", name),
	m_heartbeat_active(false),
	m_missed_heartbeats(0),
	m_address(client_ip)
{
	set_entity_type(PERFORMER_TYPE);
}

ZstPerformer::ZstPerformer(const ZstPerformer & other) : ZstContainer(other)
{
	m_heartbeat_active = other.m_heartbeat_active;
	m_missed_heartbeats = other.m_missed_heartbeats;
	m_address = other.m_address;
}

ZstPerformer::~ZstPerformer()
{
	for (auto child : m_creatables) {
		if (!child.second->is_activated()) {
			delete child.second;
		}
	}
	m_creatables.clear();
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

const char * ZstPerformer::address()
{
	return m_address.c_str();
}

int ZstPerformer::num_creatables()
{
	return m_creatables.size();
}

void ZstPerformer::write(std::stringstream & buffer)
{
	ZstContainer::write(buffer);
	
	//Pack number of children
	msgpack::pack(buffer, num_creatables());

	//Pack children
	for (auto child : m_creatables) {
		msgpack::pack(buffer, child.second->entity_type());
		child.second->write(buffer);
	}
}

void ZstPerformer::read(const char * buffer, size_t length, size_t & offset)
{
	ZstContainer::read(buffer, length, offset);

	//Unpack creatables
	auto handle = msgpack::unpack(buffer, length, offset);
	int num_children = static_cast<int>(handle.get().via.i64);
	for (int i = 0; i < num_children; ++i) {

		const char * entity_type = handle.get().via.str.ptr;

		ZstEntityBase * child = NULL;

		if (strcmp(entity_type, CONTAINER_TYPE)) {
			child = new ZstContainer();
		}
		else if (strcmp(entity_type, COMPONENT_TYPE)) {
			child = new ZstComponent();
		}

		child->read(buffer, length, offset);
		m_creatables[child->URI()] = child;
	}
}
