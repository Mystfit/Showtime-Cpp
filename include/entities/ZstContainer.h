#pragma once

#include <ZstExports.h>
#include <entities/ZstComponent.h>

#define CONTAINER_TYPE "con"

class ZstContainer : public ZstComponent
{
public:
	friend class ZstClient;
	ZST_EXPORT ZstContainer();
	ZST_EXPORT ZstContainer(const char * path);
	ZST_EXPORT ZstContainer(const char * component_type, const char * path);
	ZST_EXPORT ZstContainer(const ZstContainer & other);
	ZST_EXPORT ~ZstContainer();

	//Find a child in this entity by a URI
	ZST_EXPORT ZstEntityBase * find_child_by_URI(const ZstURI & path);

	//Get a child entity at a specific URI
	ZST_EXPORT ZstEntityBase * get_child_by_URI(const ZstURI & path);
	
	//Get a child by index
	ZST_EXPORT ZstEntityBase * get_child_at(int index) const;

	//Number of children owned by this entity
	ZST_EXPORT const size_t num_children() const;

	//Unplug all cables in container
	ZST_EXPORT virtual void disconnect_cables() override;
	
	//Manipulate the hierarchy of this entity
	ZST_EXPORT void add_child(ZstEntityBase * child);
	ZST_EXPORT void remove_child(ZstEntityBase * child);

	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) const override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

	//Adaptor registration
	ZST_EXPORT virtual void add_adaptor_to_children(ZstSynchronisableAdaptor * adaptor) override;
	ZST_EXPORT virtual void remove_adaptor_from_children(ZstSynchronisableAdaptor * adaptor) override;

	ZST_EXPORT virtual void set_proxy();


protected:
	ZST_EXPORT ZstCableBundle * get_child_cables(ZstCableBundle * bundle) override;
    
    //Set parent for all children
    ZST_EXPORT void set_parent(ZstEntityBase * entity) override;
    
    //Enqueue all children as activated
    ZST_EXPORT virtual void enqueue_activation() override;
    
    //Enqueue all children as deactivated
    ZST_EXPORT virtual void enqueue_deactivation() override;
    
    //Set all children as activated
    ZST_EXPORT virtual void set_activation_status(ZstSyncStatus status) override;
    

private:
	ZstEntityMap m_children;
};
