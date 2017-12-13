#include <exception>
#include "entities/ZstPerformer.h"
#include <czmq.h>

using namespace std;

ZstPerformer::ZstPerformer() : 
	ZstContainer(PERFORMER_TYPE),
	m_heartbeat_active(false),
	m_missed_heartbeats(0),
	m_address(""),
	m_uuid("")
{
}

ZstPerformer::ZstPerformer(const char * name, const char * client_ip) :
	ZstContainer(PERFORMER_TYPE, name),
	m_heartbeat_active(false),
	m_missed_heartbeats(0),
	m_address(client_ip),
	m_uuid("")
{
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

void ZstPerformer::set_uuid(const char * uuid)
{
	m_uuid = std::string(uuid);
}

const char * ZstPerformer::uuid()
{
	return m_uuid.c_str();
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

	msgpack::pack(buffer, num_creatables());
	for (auto child : m_creatables) {
		child.second->write(buffer);
	}
}

void ZstPerformer::read(const char * buffer, size_t length, size_t & offset)
{
	ZstContainer::read(buffer, length, offset);

	auto handle = msgpack::unpack(buffer, length, offset);
	m_uuid = std::string(handle.get().via.str.ptr);

	//Unpack creatables
	handle = msgpack::unpack(buffer, length, offset);
	int num_children = static_cast<int>(handle.get().via.i64);
	for (int i = 0; i < num_children; ++i) {
		//TODO: How do we know to create a component or a container? Does it matter?
		ZstContainer * child = new ZstContainer();
		child->read(buffer, length, offset);
		m_creatables[child->URI()] = child;
	}
}
