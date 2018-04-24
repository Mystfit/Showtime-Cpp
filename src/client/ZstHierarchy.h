/*
	ZstHierarchy

	Storage and utilities for navigating and manipulating the entity hierarchy of a performance
*/

#pragma once
#include <concurrentqueue.h>
#include <ZstCore.h>
#include <ZstEventDispatcher.hpp>
#include "ZstClientModule.h"
#include "ZstClient.h"
#include "adaptors/ZstPerformanceAdaptor.hpp"
#include "../core/liasons/ZstSynchronisableLiason.hpp"


class ZstHierarchy : 
	public ZstClientModule,
	public ZstEventDispatcher<ZstSessionAdaptor*>,
	public ZstEventDispatcher<ZstSynchronisableAdaptor*>,
	private ZstSynchronisableAdaptor,
	private ZstStageAdaptor,
	private ZstSynchronisableLiason
{
	using ZstEventDispatcher<ZstSynchronisableAdaptor*>::run_event;
	using ZstEventDispatcher<ZstSessionAdaptor*>::add_event;

public:
	ZstHierarchy(ZstClient * client);
	~ZstHierarchy();
	void destroy() override;
	void init(std::string name);


	// --------------------------
	// Event dispatcher overrides
	// --------------------------

	void process_events();


	// --------------------
	// Adaptor behaviours
	// --------------------
	
	void on_receive_from_stage(int payload_index, ZstStageMessage * msg) override;
	void notify_event_ready(ZstSynchronisable * synchronisable) override;
	

	// ------------------------------
	// Entity activation/deactivation
	// ------------------------------
	
	void synchronise_graph(bool async = false);
	void synchronise_graph_complete(ZstMessageReceipt response);
	void activate_entity(ZstEntityBase* entity, bool async = false);
	void destroy_entity(ZstEntityBase * entity, bool async = false);
	void destroy_plug(ZstPlug * plug, bool async);


	// ------------------------------
	// Performers
	// ------------------------------

	void add_performer(ZstPerformer & performer);
	

	// ------------------------------
	// Hierarchy queries
	// ------------------------------
	
	ZstEntityBase * find_entity(const ZstURI & path);
	ZstPlug * find_plug(const ZstURI & path);
	bool path_is_local(const ZstURI & path);
	void add_proxy_entity(ZstEntityBase & entity);
	ZstPerformer * get_performer_by_URI(const ZstURI & uri) const;
	ZstPerformer * get_local_performer() const;

private:
	ZstHierarchy();
	ZstPerformer * m_root;
	ZstPerformerMap m_clients;


	// ----------------
	// Event completion
	// ----------------
	
	void activate_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity);
	void destroy_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity);
	void destroy_plug_complete(ZstMessageReceipt response, ZstPlug * plug);
};
