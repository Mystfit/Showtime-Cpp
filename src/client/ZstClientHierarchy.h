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
#include "../core/adaptors/ZstStageDispatchAdaptor.hpp"
#include "../core/ZstStageMessage.h"
#include "../core/ZstHierarchy.h"

class ZstClientHierarchy : 
	public ZstHierarchy,
	public ZstClientModule,
	public ZstStageDispatchAdaptor
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
	
	void on_receive_from_stage(ZstStageMessage * msg) override;


	// ------------------------------
	// Entity activation/deactivation
	// ------------------------------
	
	void activate_entity(ZstEntityBase* entity, bool async) override;
	void destroy_entity(ZstEntityBase * entity, bool async) override;


	// ------------------------------
	// Performers
	// ------------------------------

	void add_performer(ZstPerformer & performer) override;
	

	// ------------------------------
	// Hierarchy queries
	// ------------------------------
	
	ZstEntityBase * find_entity(const ZstURI & path) override;
	bool path_is_local(const ZstURI & path);
	void add_proxy_entity(ZstEntityBase & entity) override;
	ZstPerformer * get_local_performer() const;


	// ------------------------------
	// Event dispatchers
	// ------------------------------

	ZstEventDispatcher<ZstStageDispatchAdaptor*> & stage_events();


private:
	ZstPerformer * m_root;

	// ----------------
	// Event completion
	// ----------------
	
	void activate_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity);
	void destroy_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity);
	void destroy_plug_complete(ZstMessageReceipt response, ZstPlug * plug);


	// -----------------
	// Event dispatchers
	// -----------------

	ZstEventDispatcher<ZstStageDispatchAdaptor*> m_stage_events;
};
