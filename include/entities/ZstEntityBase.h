#pragma once

#include <unordered_map>
#include <vector>
#include "../ZstExports.h"
#include "../ZstURI.h"
#include "../ZstStreamable.h"

//Forwards
class ZstEntityEvent;
class ZstINetworkInteractor;
class ZstEntityBase;
class ZstCableBundle;

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
	ZST_EXPORT virtual void register_network_interactor(ZstINetworkInteractor * sender) {};

	//Query if entity is active on the stage
	ZST_EXPORT bool is_activated();

	//Attach entity activation callback
	ZST_EXPORT void attach_activation_callback(ZstEntityEvent * callback);
	ZST_EXPORT void detach_activation_callback(ZstEntityEvent * callback);
	ZST_EXPORT void process_events();

	//The parent of this entity
	ZST_EXPORT ZstEntityBase * parent() const;

	ZST_EXPORT virtual void update_URI();

    //Entity type
	ZST_EXPORT const char * entity_type() const;
    
    //URI for this entity
	ZST_EXPORT const ZstURI & URI();
    
    //Entity flags
	ZST_EXPORT bool is_destroyed();

	//Cable management
	ZST_EXPORT virtual ZstCableBundle * acquire_cable_bundle();
	ZST_EXPORT void release_cable_bundle(ZstCableBundle * cables);
	ZST_EXPORT virtual void disconnect_cables() {};
	    
	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

protected:
	//Set entity status
	ZST_EXPORT void set_entity_type(const char * entity_type);
	ZST_EXPORT virtual void set_parent(ZstEntityBase* entity);
	ZST_EXPORT void set_destroyed();
	ZST_EXPORT virtual void set_activated();
	ZST_EXPORT virtual void set_deactivated();
	ZST_EXPORT virtual ZstCableBundle * get_child_cables(ZstCableBundle * bundle);

private:
	bool m_is_activated;
	bool m_is_destroyed;
	ZstEntityBase * m_parent;
	char * m_entity_type;
	ZstURI m_uri;

	std::vector<ZstEntityEvent*> m_activation_callbacks;
	bool m_activation_queued = false;
};
