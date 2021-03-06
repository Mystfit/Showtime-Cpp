/*
	ZstHierarchy

	Storage and utilities for navigating and manipulating the entity hierarchy of a performance
*/

#pragma once

#include <showtime/ZstCore.h>
#include <showtime/adaptors/ZstSessionAdaptor.hpp>
#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstStageTransportAdaptor.hpp"
#include "../core/ZstStageMessage.h"
#include "../core/ZstHierarchy.h"
#include "ZstClientModule.h"

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
	

	// --------------------------
	// Event dispatcher overrides
	// --------------------------

	void process_events() override;
	void flush_events() override;
	

	// --------------------
	// Adaptor behaviours
	// --------------------
	
	virtual void on_receive_msg(const std::shared_ptr<ZstStageMessage>& stage_msg) override;
	void publish_entity_update(ZstEntityBase * entity, const ZstURI& original_path) override;
	void request_entity_activation(ZstEntityBase * entity) override;
	void request_entity_registration(ZstEntityBase * entity) override;
	

	// ------------------------------
	// Entity activation/deactivation
	// ------------------------------
	
	void activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour & sendtype, ZstMessageReceivedAction callback);
	void activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour & sendtype) override;
	void deactivate_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype) override;
	ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name);
	ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name, const ZstTransportRequestBehaviour & sendtype) override;
    
	void client_leaving_handler(const ClientLeaveRequest* request);
    void create_proxy_entity_handler(const EntityCreateRequest * request);
	void factory_create_entity_handler(const FactoryCreateEntityRequest * request, ZstMsgID request_id);
    void update_proxy_entity_handler(const EntityUpdateRequest * request);
    void destroy_entity_handler(const EntityDestroyRequest * request);
	

	// ------------------------------
	// Hierarchy queries
	// ------------------------------
	
	virtual void update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path) override;
	virtual ZstEntityBase * find_entity(const ZstURI & path) const override;
	bool path_is_local(const ZstURI & path);
	virtual std::unique_ptr<ZstEntityBase> create_proxy_entity(const EntityTypes entity_type, const EntityData* entity_data, const void* payload) override;
	virtual void update_proxy_entity(ZstEntityBase * original, const EntityTypes entity_type, const EntityData* entity_data, const void* payload) override;
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
