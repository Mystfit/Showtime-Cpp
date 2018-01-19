#include "entities/ZstContainer.h"
#include "msgpack.hpp"

ZstContainer::ZstContainer() :
	ZstComponent(CONTAINER_TYPE, "", "")
{
}

ZstContainer::ZstContainer(const char * path) :
	ZstComponent(CONTAINER_TYPE, "", path)
{
}

ZstContainer::ZstContainer(const char * component_type, const char * path) :
	ZstComponent(CONTAINER_TYPE, component_type, path)
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

ZstEntityBase * ZstContainer::find_child_by_URI(const ZstURI & path)
{
	ZstEntityBase * result = NULL;
	ZstEntityBase * previous = NULL;

	if (this->URI().size() >= path.size() || !path.contains(URI())) {
		return result;
	}

	ZstURI next;
	int distance = distance = path.size() - URI().size();
	while(distance > 0) {
		next = path.range(0, path.size() - distance);

		if (!previous)
			previous = this;

		result = dynamic_cast<ZstContainer*>(previous)->get_child_by_URI(next);

		//Could not find child entity at the last level, check the plugs
		if (distance == 1 && !result) {
			result = get_plug_by_URI(next);
		}

		if (result) {
			distance = path.size() - result->URI().size();
			previous = result;
		} else {
			break;
		}
	}

	return result;
}

ZstEntityBase * ZstContainer::get_child_by_URI(const ZstURI & path)
{
	ZstEntityBase * result = NULL;
	std::unordered_map<ZstURI, ZstEntityBase*>::iterator it = m_children.find(path);
	
	if (it != m_children.end())
		result = it->second;

	return result;
}

ZstEntityBase * ZstContainer::get_child_at(int index) const
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

void ZstContainer::set_activated()
{
	ZstComponent::set_activated();
	
	//Recursively activate all children
	for (auto c : m_children) {
		c.second->set_activated();
	}
}

void ZstContainer::set_parent(ZstEntityBase * entity)
{
	ZstEntityBase::set_parent(entity);

	auto temp_children = m_children;

	for (auto child : temp_children) {
		remove_child(child.second);
		add_child(child.second);
	}
}

void ZstContainer::add_child(ZstEntityBase * child) {
	ZstEntityBase * c = find_child_by_URI(child->URI());
	if (!c) {
		//If child is a plug, needs to be added to the plug list
		child->set_parent(this);
		m_children[child->URI()] = child;
	}
}

void ZstContainer::remove_child(ZstEntityBase * child) {
	
	//Check if we're removing a plug or not
	if (strcmp(child->entity_type(), PLUG_TYPE) == 0) {
		remove_plug(dynamic_cast<ZstPlug*>(child));
	}
	else {
		auto c = m_children.find(child->URI());
		if (c != m_children.end()) {
			m_children.erase(c);
		}
	}
	update_URI();
}

void ZstContainer::write(std::stringstream & buffer)
{
	//Pack entity
	ZstComponent::write(buffer);

	//Pack children
	msgpack::pack(buffer, num_children());
	for (auto child : m_children) {
		msgpack::pack(buffer, child.second->entity_type());
		child.second->write(buffer);
	}
}

void ZstContainer::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack entity base first
	ZstComponent::read(buffer, length, offset);

	//Unpack children
	auto handle = msgpack::unpack(buffer, length, offset);
	int num_children = static_cast<int>(handle.get().via.i64);
	for (int i = 0; i < num_children; ++i) {
		ZstEntityBase * child = NULL;

		handle = msgpack::unpack(buffer, length, offset);
		char * entity_type = (char*)malloc(handle.get().via.str.size + 1);
		memcpy(entity_type, handle.get().via.str.ptr, handle.get().via.str.size);
		entity_type[handle.get().via.str.size] = '\0';

		if (strcmp(entity_type, CONTAINER_TYPE) == 0) {
			child = new ZstContainer();
		}
		else if (strcmp(entity_type, COMPONENT_TYPE) == 0) {
			child = new ZstComponent();
		}
		free(entity_type);

		child->read(buffer, length, offset);
		add_child(child);
	}
}

