/*
	ZstHierarchy

	Storage and utilities for navigating and manipulating the entity hierarchy of a performance
*/

#pragma once

#include <ZstCore.h>
#include "ZstClientModule.h"

#include "adaptors/ZstSessionAdaptor.hpp"
#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstStageTransportAdaptor.hpp"
#include "../core/ZstStageMessage.h"
#include "../core/ZstHierarchy.h"

namespace showtime::client {
    
class ZstClientHierarchy : 
	public ZstHierarchy,
	public ZstClientModule,
	public ZstStageTransportAdaptor
{
	friend class ZstClient;
public:
	ZstClientHierarchy();
	virtual ~ZstClientHierarchy();
	
	void init(std::string name);
	void init() override {};
	void destroy() override;
	

	// --------------------------
	// Event dispatcher overrides
	// --------------------------

	void process_events() override;
	void flush_events() override;
	

	// --------------------
	// Adaptor behaviours
	// --------------------
	
	virtual void on_receive_msg(std::shared_ptr<ZstStageMessage> stage_msg) override;
	void on_publish_entity_update(ZstEntityBase * entity) override;
	void on_request_entity_activation(ZstEntityBase * entity) override;
	

	// ------------------------------
	// Entity activation/deactivation
	// ------------------------------
	
	void activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour & sendtype, ZstMessageReceivedAction callback);
	void activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour & sendtype) override;
	void destroy_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype) override;
	ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name);
	ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name, const ZstTransportRequestBehaviour & sendtype) override;
    
    void create_proxy_entity_handler(const EntityCreateRequest * request);
	void factory_create_entity_handler(const FactoryCreateEntityRequest * request, ZstMsgID request_id);
    void update_proxy_entity_handler(const EntityUpdateRequest * request);
    void destroy_entity_handler(const EntityDestroyRequest * request);
	
	// ------------------------------
	// Performers
	// ------------------------------

	virtual void add_performer(const Performer* performer) override;
	virtual ZstEntityBundle & get_performers(ZstEntityBundle & bundle) const override;
	virtual ZstPerformer * get_performer_by_URI(const ZstURI & uri) const override;
	

	// ------------------------------
	// Hierarchy queries
	// ------------------------------
	
	virtual ZstEntityBase * find_entity(const ZstURI & path) const override;
	bool path_is_local(const ZstURI & path);
	virtual void add_proxy_entity(const EntityTypes entity_type, const EntityData* entity_data, const void* payload) override;
	virtual void update_proxy_entity(const EntityTypes entity_type, const EntityData* entity_data, const void* payload) override;
	ZstPerformer * get_local_performer() const override;


private:
    std::shared_ptr<ZstPerformer> m_root;

	// ----------------
	// Event completion
	// ----------------
	virtual void activate_entity_complete(ZstEntityBase * entity) override;
	virtual void destroy_entity_complete(ZstEntityBase * entity) override;
};

}
