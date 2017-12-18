#pragma once

#include "ZstExports.h"
#include "ZstComponent.h"

#define CONTAINER_TYPE "container"

class ZstContainer : public ZstComponent
{
public:
	friend class ZstClient;
	ZST_EXPORT ZstContainer();
	ZST_EXPORT ZstContainer(const char * path);
	ZST_EXPORT ZstContainer(const char * component_type, const char * path);

	ZST_EXPORT ~ZstContainer();

	ZST_EXPORT virtual void init() override {};

	//Register graph sender for output plugs and children
	ZST_EXPORT virtual void register_graph_sender(ZstGraphSender * sender);

	//Find a child in this entity by a URI
	ZST_EXPORT virtual ZstEntityBase * find_child_by_URI(const ZstURI & path);

	//Get a child entity at a specific URI
	ZST_EXPORT virtual ZstEntityBase * get_child_by_URI(const ZstURI & path);
	
	//Get a child by index
	ZST_EXPORT virtual ZstEntityBase * get_child_at(int index) const;

	//Number of children owned by this entity
	ZST_EXPORT virtual const size_t num_children() const;

	//Manipulate the hierarchy of this entity
	ZST_EXPORT void add_child(ZstEntityBase * child);
	ZST_EXPORT void remove_child(ZstEntityBase * child);

	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

private:
	std::unordered_map<ZstURI, ZstEntityBase*> m_children;
};