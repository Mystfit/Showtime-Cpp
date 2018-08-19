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
	ZST_EXPORT ZstEntityBase * walk_child_by_URI(const ZstURI & path);

	//Get a child entity at a specific URI
	ZST_EXPORT ZstEntityBase * get_child_by_URI(const ZstURI & path);
	
	//Get a child by index
	ZST_EXPORT ZstEntityBase * get_child_at(size_t index) const;

	//Number of children owned by this entity
	ZST_EXPORT const size_t num_children() const;
	
	//Manipulate the hierarchy of this entity
	ZST_EXPORT void add_child(ZstEntityBase * child);
	ZST_EXPORT void remove_child(ZstEntityBase * child);

	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) const override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

protected:
	ZST_EXPORT ZstCableBundle * get_child_cables(ZstCableBundle * bundle) override;
	ZST_EXPORT ZstEntityBundle * get_child_entities(ZstEntityBundle * bundle) override;

    //Set parent for all children
    ZST_EXPORT void set_parent(ZstEntityBase * entity) override;

private:
	ZstEntityMap m_children;
};
