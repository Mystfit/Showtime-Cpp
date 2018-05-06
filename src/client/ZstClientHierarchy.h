/*
	ZstHierarchy

	Storage and utilities for navigating and manipulating the entity hierarchy of a performance
*/

#pragma once
#include <concurrentqueue.h>
#include <ZstCore.h>
#include <ZstEventDispatcher.hpp>
#include "ZstClientModule.h"
#include "adaptors/ZstSessionAdaptor.hpp"
#include "../core/adaptors/ZstStageDispatchAdaptor.hpp"
#include "../core/ZstMessage.h"
#include "../core/ZstHierarchy.h"

class ZstClientHierarchy : 
	public ZstHierarchy,
	public ZstClientModule,
	public ZstStageDispatchAdaptor
{
public:
	ZstClientHierarchy();
	~ZstClientHierarchy();
	
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
	
	void on_receive_from_stage(size_t payload_index, ZstMessage * msg) override;
	void synchronisable_has_event(ZstSynchronisable * synchronisable) override;


	// ------------------------------
	// Entity activation/deactivation
	// ------------------------------
	
	void activate_entity(ZstEntityBase* entity, bool async);
	void destroy_entity(ZstEntityBase * entity, bool async);
	void destroy_plug(ZstPlug * plug, bool async);


	// ------------------------------
	// Performers
	// ------------------------------

	void add_performer(ZstPerformer & performer);
	

	// ------------------------------
	// Hierarchy queries
	// ------------------------------
	
	ZstEntityBase * find_entity(const ZstURI & path);
	bool path_is_local(const ZstURI & path);
	void add_proxy_entity(ZstEntityBase & entity);
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
	ZstEventDispatcher<ZstSynchronisableAdaptor*> m_synchronisable_events;
};
