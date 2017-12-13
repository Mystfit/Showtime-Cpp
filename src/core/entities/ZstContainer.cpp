#include "entities/ZstContainer.h"
#include "msgpack.hpp"

ZstContainer::ZstContainer() :
	ZstComponent(CONTAINER_TYPE)
{
}

ZstContainer::ZstContainer(const char * entity_type) :
	ZstComponent(entity_type)
{
}

ZstContainer::ZstContainer(const char * entity_type, const char * entity_name) :
	ZstComponent(entity_type, entity_name)
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

	//If our own path is >= than our target path then we won't find the child here
	if (this->URI().size() >= path.size() || !path.contains(URI())) {
		return result;
	}

	ZstURI next;
	int distance = distance = path.size() - URI().size();
	while(distance > 0) {
		next = path.range(0, path.size() - distance);
		result = get_child_by_URI(next);

		//Could not find child entity at the last level, check the plugs
		if (distance == 1 && !result) {
			result = get_plug_by_URI(next);
		}

		if(result)
			distance = path.size() - result->URI().size();
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

void ZstContainer::add_child(ZstEntityBase * child) {
	ZstEntityBase * c = find_child_by_URI(child->URI());
	if (!c) {
		//If child is a plug, needs to be added to the plug list
		child->set_parent(this);
		if (child->parent()->URI() == URI()) {
			m_children[child->URI()] = child;
		}
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

