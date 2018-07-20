/*
	ZstHierarchy

	Storage and utilities for navigating and manipulating the entity hierarchy of a performance
*/

#pragma once

#include <ZstCore.h>
#include <ZstEventDispatcher.hpp>
#include "ZstClientModule.h"

#include "adaptors/ZstSessionAdaptor.hpp"
#include "../dependencies/concurrentqueue.h"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/ZstStageMessage.h"
#include "../core/ZstHierarchy.h"

class ZstClientHierarchy : 
	public ZstHierarchy,
	public ZstClientModule,
	public ZstTransportAdaptor
{
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

	// ------------------------------
	// Entity activation/deactivation
	// ------------------------------
	
	void activate_entity(ZstEntityBase* entity, const ZstTransportSendType & sendtype) override;
	void destroy_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype) override;

	// ------------------------------
	// Performers
	// ------------------------------

	void add_performer(ZstPerformer & performer) override;
	

	// ------------------------------
	// Hierarchy queries
	// ------------------------------
	
	ZstEntityBase * find_entity(const ZstURI & path) override;
	bool path_is_local(const ZstURI & path);
	ZstMsgKind add_proxy_entity(ZstEntityBase & entity) override;
	ZstPerformer * get_local_performer() const;


	// ------------------------------
	// Event dispatchers
	// ------------------------------

	ZstEventDispatcher<ZstTransportAdaptor*> & stage_events();


private:
	ZstPerformer * m_root;

	// ----------------
	// Event completion
	// ----------------
	void activate_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity);
	void destroy_entity_complete(ZstEntityBase * entity) override;

	// -----------------
	// Event dispatchers
	// -----------------

	ZstEventDispatcher<ZstTransportAdaptor*> m_stage_events;
};
