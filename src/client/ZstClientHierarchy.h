/*
	ZstHierarchy

	Storage and utilities for navigating and manipulating the entity hierarchy of a performance
*/

#pragma once

#include <ZstCore.h>
#include "ZstClientModule.h"

#include "adaptors/ZstSessionAdaptor.hpp"
#include "../core/ZstEventDispatcher.hpp"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/ZstStageMessage.h"
#include "../core/ZstHierarchy.h"

class ZstClientHierarchy : 
	public ZstHierarchy,
	public ZstClientModule,
	public ZstTransportAdaptor
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
	
	void on_receive_msg(ZstMessage * msg) override;
	void on_publish_entity_update(ZstEntityBase * entity) override;
	void on_request_entity_activation(ZstEntityBase * entity) override;
	

	// ------------------------------
	// Entity activation/deactivation
	// ------------------------------
	
	void activate_entity(ZstEntityBase* entity, const ZstTransportSendType & sendtype, ZstMsgID request_ID);
	void activate_entity(ZstEntityBase* entity, const ZstTransportSendType & sendtype) override;
	void destroy_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype) override;
	ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name);
	ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name, const ZstTransportSendType & sendtype) override;
	void create_entity_handler(ZstMessage * msg);
	
	
	// ------------------------------
	// Performers
	// ------------------------------

	virtual void add_performer(const ZstPerformer & performer) override;
	virtual ZstEntityBundle & get_performers(ZstEntityBundle & bundle) const override;
	virtual ZstPerformer * get_performer_by_URI(const ZstURI & uri) const override;
	

	// ------------------------------
	// Hierarchy queries
	// ------------------------------
	
	virtual ZstEntityBase * find_entity(const ZstURI & path) const override;
	bool path_is_local(const ZstURI & path);
	virtual ZstMsgKind add_proxy_entity(const ZstEntityBase & entity) override;
	virtual ZstMsgKind update_proxy_entity(const ZstEntityBase & entity) override;
	ZstPerformer * get_local_performer() const override;


private:
	ZstPerformer * m_root;

	// ----------------
	// Event completion
	// ----------------
	virtual void activate_entity_complete(ZstEntityBase * entity) override;
	virtual void destroy_entity_complete(ZstEntityBase * entity) override;
};
