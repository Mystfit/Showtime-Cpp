#include <msgpack.hpp>
#include <entities/ZstContainer.h>
#include "../ZstEventDispatcher.hpp"

ZstContainer::ZstContainer() :
	ZstComponent("", "")
{
	set_entity_type(CONTAINER_TYPE);
}

ZstContainer::ZstContainer(const char * path) :
	ZstComponent("", path)
{
	set_entity_type(CONTAINER_TYPE);
}

ZstContainer::ZstContainer(const char * component_type, const char * path) :
	ZstComponent(component_type, path)
{
	set_entity_type(CONTAINER_TYPE);
}

ZstContainer::ZstContainer(const ZstContainer & other) : ZstComponent(other)
{
	for (auto c : other.m_children) {

		if (strcmp(c.second->entity_type(), CONTAINER_TYPE) == 0) {
			add_child(new ZstContainer(*dynamic_cast<ZstContainer*>(c.second)));
		}
		else if (strcmp(c.second->entity_type(), COMPONENT_TYPE) == 0) {
			add_child(new ZstComponent(*dynamic_cast<ZstComponent*>(c.second)));
		}
	}
}

ZstContainer::~ZstContainer()
{
	//if (!is_proxy()) {
	//	auto children = m_children;
	//	for (auto c : children) {
	//		delete c.second;
	//	}
	//	m_children.clear();
	//}
	if (!is_proxy()) {
		for (auto child : m_children) {
			// TODO: Deleting children will crash if the host GC's them after we delete them here
			ZstLog::entity(LogLevel::debug, "FIXME: Container {} leaking entity {} to avoid host app crashing when GCing", URI().path(), child.second->URI().path());
			//delete child.second;
		}
		m_children.clear();
	}
}

ZstEntityBase * ZstContainer::walk_child_by_URI(const ZstURI & path)
{
	ZstEntityBase * result = NULL;
	ZstEntityBase * previous = NULL;

	if (this->URI().size() >= path.size() || !path.contains(URI())) {
		return result;
	}
	
	int distance = static_cast<int>(path.size()) - static_cast<int>(this->URI().size());
	if (distance < 0) assert(distance >= 0);

	while(distance > 0) {
		ZstURI next = path.range(0, path.size() - distance);
		result = NULL;

		if (!previous) {
			previous = this;
		}

		//Check if the parent is a container
		ZstContainer * prev_container = dynamic_cast<ZstContainer*>(previous);
		if (prev_container) {
			result = prev_container->get_child_by_URI(next);
		}
			
		//Could not find child entity at the last level, check the plugs
		if (distance == 1 && !result) {
			ZstComponent * prev_component = dynamic_cast<ZstComponent*>(previous);
			if (prev_component) {
				result = prev_component->get_plug_by_URI(next);
			}
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
	ZstEntityMap::iterator it = m_children.find(path);
	
	if (it != m_children.end())
		result = it->second;

	return result;
}

ZstEntityBase * ZstContainer::get_child_at(size_t index) const
{
	ZstEntityBase * result = NULL;
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

void ZstContainer::set_parent(ZstEntityBase * entity)
{
    if(is_destroyed()) return;

    ZstComponent::set_parent(entity);
    
	//Removing and re-adding children will update their URI
	auto children = m_children;
	for (auto child : children) {
        this->remove_child(child.second);
	}
    
    for(auto child : children){
        this->add_child(child.second);
    }
}

void ZstContainer::add_child(ZstEntityBase * child) {
	if (is_destroyed()) return;

	ZstEntityBase::add_child(child);
    m_children[child->URI()] = child;
}

void ZstContainer::remove_child(ZstEntityBase * child) {
    if(is_destroyed()) return;

	auto c = m_children.find(child->URI());
	if (c != m_children.end()) {
		m_children.erase(c);
	}

	ZstEntityBase::remove_child(child);
}

void ZstContainer::write(std::stringstream & buffer) const
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

ZstCableBundle & ZstContainer::get_child_cables(ZstCableBundle & bundle) const
{
	for(auto child : m_children){
		child.second->get_child_cables(bundle);
	}
	return ZstComponent::get_child_cables(bundle);
}

ZstEntityBundle & ZstContainer::get_child_entities(ZstEntityBundle & bundle, bool include_parent)
{
	for (auto child : m_children) {
		child.second->get_child_entities(bundle);
		bundle.add(child.second);
	}
	return ZstComponent::get_child_entities(bundle, include_parent);
}

