#include "entities/ZstContainer.h"
#include "msgpack.hpp"

ZstContainer::ZstContainer() : 
	ZstComponent(CONTAINER_TYPE)
{
}

ZstContainer::ZstContainer(const char * entity_type, const char * entity_name) :
	ZstComponent(entity_type, entity_name)
{
}

ZstContainer::ZstContainer(const char * entity_name) :
	ZstComponent(CONTAINER_TYPE, entity_name)
{
}

ZstContainer::~ZstContainer()
{
	for (auto child : m_children) {
		if (!child.second->is_activated()) {
			delete child.second;
		}
	}
	m_children.clear();
	m_parent = NULL;
}

void ZstContainer::register_graph_sender(ZstGraphSender * sender)
{
	//Register sender to out own plugs
	ZstComponent::register_graph_sender(sender);

	//Register sender for all child components in case they have plugs too
	for (auto child : m_children) {
		child.second->register_graph_sender(sender);
	}
}

ZstEntityBase * ZstContainer::find_child_by_URI(const ZstURI & path) const
{
	ZstEntityBase * result = NULL;

	auto entity_iter = m_children.find(path);
	if (entity_iter != m_children.end()) {
		result = entity_iter->second;
	}

	return result;
}

ZstEntityBase * ZstContainer::get_child_entity_at(int index) const
{
	ZstEntityBase * result;
	int i = 0;
	for (auto it : m_children) {
		if (i == index) {
			result = it.second;
			break;
		}
		i++;
	}
	return result;
}

const size_t ZstContainer::num_children() const
{
	return m_children.size();
}

void ZstContainer::add_child(ZstEntityBase * child) {
	ZstEntityBase * c = find_child_by_URI(child->URI());
	if (!c) {
		//New URI should be a combination of the parent and the local path
		if (!(child->URI().range(0, child->URI().size() - 1) == URI())) {
			child->m_uri = URI() + child->m_uri;
			child->m_parent = this;
		}
		m_children[child->URI()] = child;
		child->set_parent(this);
	}
}

void ZstContainer::remove_child(ZstEntityBase * child) {
	auto c = m_children.find(child->URI());
	if (c != m_children.end()) {
		m_children.erase(c);
	}
}

void ZstContainer::write(std::stringstream & buffer)
{
	//Pack entity
	ZstComponent::write(buffer);

	//Pack children
	msgpack::pack(buffer, num_children());
	for (auto child : m_children) {
		child.second->write(buffer);
	}
}

void ZstContainer::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack entity base first
	ZstComponent::read(buffer, length, offset);

	//Unpack children
	auto handle = msgpack::unpack(buffer, length, offset);
	auto obj = handle.get();
	int num_children = static_cast<int>(obj.via.i64);
	for (int i = 0; i < num_children; ++i) {
		//TODO: How do we know to create a component or a container?
		ZstContainer * child = new ZstContainer();
		child->read(buffer, length, offset);
		add_child(child);
	}
}

