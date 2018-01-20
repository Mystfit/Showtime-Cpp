#pragma once

#include <unordered_map>
#include "../ZstExports.h"
#include "../ZstURI.h"
#include "../ZstStreamable.h"

//Forwards
class ZstEntityEvent;
class ZstGraphSender;
class ZstEntityBase;

class ZstEntityBase : public ZstStreamable {
public:
	friend class ZstClient;
	friend class ZstContainer;

	//Base entity
	ZST_EXPORT ZstEntityBase(const char * entity_name);
	ZST_EXPORT ZstEntityBase(const ZstEntityBase & other);
	ZST_EXPORT virtual ~ZstEntityBase();
    
    //Overridable init - must be called by overriden classes
	ZST_EXPORT virtual void init() = 0;

	//Register graph sender so this entity can comunicate with the graph
	ZST_EXPORT virtual void register_graph_sender(ZstGraphSender * sender) {};

	//Query if entity is active on the stage
	ZST_EXPORT bool is_activated();

	//The parent of this entity
	ZST_EXPORT ZstEntityBase * parent() const;

	ZST_EXPORT virtual void update_URI();

    //Entity type
	ZST_EXPORT const char * entity_type() const;
    
    //URI for this entity
	ZST_EXPORT const ZstURI & URI();
    
    //Entity flags
	ZST_EXPORT bool is_destroyed();
    
    
	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

protected:
	//Set entity status
	ZST_EXPORT void set_entity_type(const char * entity_type);
	ZST_EXPORT virtual void set_parent(ZstEntityBase* entity);
	ZST_EXPORT void set_destroyed();
	ZST_EXPORT virtual void set_activated();

private:
	bool m_is_activated;
	bool m_is_destroyed;
	ZstEntityBase * m_parent;
	char * m_entity_type;
	ZstURI m_uri;
};
