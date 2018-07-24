#include <msgpack.hpp>
#include <entities/ZstContainer.h>

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
			m_children[c.first] = new ZstContainer(*dynamic_cast<ZstContainer*>(c.second));
		}
		else if (strcmp(c.second->entity_type(), COMPONENT_TYPE) == 0) {
			m_children[c.first] = new ZstComponent(*dynamic_cast<ZstComponent*>(c.second));
		}
		m_children[c.first]->set_parent(this);		
	}
}

ZstContainer::~ZstContainer()
{
	auto children = m_children;
	for (auto child : children){
		//TODO: This will fail if the entity wasn't assigned in this DLL!
		delete child.second;
	}
	m_children.clear();
	m_parent = NULL;
}

ZstEntityBase * ZstContainer::find_child_by_URI(const ZstURI & path)
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

ZstEntityBase * ZstContainer::get_child_at(int index) const
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

void ZstContainer::enqueue_activation()
{
	ZstComponent::enqueue_activation();
	
	//Recursively activate all children
	for (auto c : m_children) {
		c.second->enqueue_activation();
	}
}

void ZstContainer::enqueue_deactivation()
{
    ZstComponent::enqueue_deactivation();
    for(auto c : m_children){
        c.second->enqueue_deactivation();
    }
}

void ZstContainer::set_activation_status(ZstSyncStatus status)
{
    ZstComponent::set_activation_status(status);
    for (auto c : m_children) {
        c.second->set_activation_status(status);
    }
}

void ZstContainer::set_parent(ZstEntityBase * entity)
{
    if(is_destroyed()) return;

    ZstComponent::set_parent(entity);
    
	auto children = m_children;
	for (auto child : children) {
        this->remove_child(child.second);
	}
    
    for(auto child : children){
        this->add_child(child.second);
    }
}

void ZstContainer::disconnect_cables()
{
	ZstComponent::disconnect_cables();
	for (auto c : m_children) {
		c.second->disconnect_cables();
	}
}

void ZstContainer::add_child(ZstEntityBase * child) {
    if(is_destroyed()) return;
    
	//If child is a plug, needs to be added to the plug list
    child->set_parent(this);
    m_children[child->URI()] = child;
}

void ZstContainer::remove_child(ZstEntityBase * child) {
    if(is_destroyed()) return;

	auto c = m_children.find(child->URI());
	if (c != m_children.end()) {
		m_children.erase(c);
	}
	
	//TODO: How do we remove all cables from child plugs?

	child->m_parent = NULL;
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

void ZstContainer::add_adaptor_to_children(ZstSynchronisableAdaptor * adaptor)
{
	ZstComponent::add_adaptor_to_children(adaptor);
	for (auto child : m_children) {
        child.second->add_adaptor_to_children(adaptor);
	}
}

void ZstContainer::remove_adaptor_from_children (ZstSynchronisableAdaptor * adaptor)
{
	ZstComponent::remove_adaptor_from_children(adaptor);
	for (auto child : m_children) {
        child.second->remove_adaptor_from_children(adaptor);
	}
}

 void ZstContainer::set_proxy()
{
	 ZstComponent::set_proxy();
	 for (auto c : m_children) {
		 c.second->set_proxy();
	 }
}

ZstCableBundle * ZstContainer::get_child_cables(ZstCableBundle * bundle)
{
	ZstComponent::get_child_cables(bundle);

	for(auto child : m_children){
		child.second->get_child_cables(bundle);
	}

	return bundle;
}

