#include <exception>
#include <msgpack.hpp>
#include <entities/ZstPerformer.h>

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
}

ZstPerformer::~ZstPerformer()
{
	for (auto child : m_creatables) {
		if (!child.second->is_activated()) {
			//TODO:Possible memory leak here
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

size_t ZstPerformer::num_creatables() const
{
	return m_creatables.size();
}

void ZstPerformer::write(std::stringstream & buffer) const
{
	ZstContainer::write(buffer);
	
	//Pack number of children
	msgpack::pack(buffer, num_creatables());

	//Pack children
	for (auto creatable : m_creatables) {
		msgpack::pack(buffer, creatable.second->entity_type());
		creatable.second->write(buffer);
	}
}

void ZstPerformer::read(const char * buffer, size_t length, size_t & offset)
{
	ZstContainer::read(buffer, length, offset);

	//Unpack creatables
	auto handle = msgpack::unpack(buffer, length, offset);
	int num_creatables = static_cast<int>(handle.get().via.i64);
	for (int i = 0; i < num_creatables; ++i) {

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
