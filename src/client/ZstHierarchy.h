/*
	ZstHierarchy

	Hierarchy and utilities for navigating and manipulating a performance
*/

#pragma once
#include <ZstCore.h>
#include "../core/ZstMessage.h"
#include "../core/ZstEventQueue.h"
#include "../core/ZstEventDispatcher.h"
#include "../core/ZstINetworkInteractor.h"
#include "ZstReaper.h"
#include "ZstClientModule.h"


class ZstHierarchy : public ZstClientModule {
	friend class ZstEntityLeavingEvent;
	friend class ZstPlugLeavingEvent;

public:
	ZstHierarchy(ZstClient * client);
	~ZstHierarchy();
	void destroy() override;
	void init(std::string name);


	// ------------------------------
	// Entity activation/deactivation
	// ------------------------------
	void synchronise_graph(bool async = false);
	void synchronise_graph_complete(ZstMsgKind status);
	void activate_entity(ZstEntityBase* entity, bool async = false);
	void destroy_entity(ZstEntityBase * entity, bool async = false);
	void destroy_plug(ZstPlug * plug, bool async);

	// ------------------------------
	// Hierarchy queries
	// ------------------------------
	
	ZstEntityBase * find_entity(const ZstURI & path);
	ZstPlug * find_plug(const ZstURI & path);
	bool entity_is_local(ZstEntityBase & entity);
	bool path_is_local(const ZstURI & path);
	void add_proxy_entity(ZstEntityBase & entity);
	ZstPerformer * get_performer_by_URI(const ZstURI & uri) const;
	ZstPerformer * get_local_performer() const;

	
	// -------------
	// Events
	// -------------
	
	ZstEventQueue & performer_arriving_events();
	ZstEventQueue & performer_leaving_events();
	ZstEventQueue & component_arriving_events();
	ZstEventQueue & component_leaving_events();
	ZstEventQueue & component_type_arriving_events();
	ZstEventQueue & component_type_leaving_events();
	ZstEventQueue & plug_arriving_events();
	ZstEventQueue & plug_leaving_events();

private:
	ZstHierarchy();
	ZstPerformer * m_root;
	ZstPerformerMap m_clients;
	ZstINetworkInteractor * m_client;
	void add_performer(ZstPerformer & performer);


	// ----------------
	// Event completion
	// ----------------
	
	void activate_entity_complete(ZstMsgKind status, ZstEntityBase * entity);
	void destroy_entity_complete(ZstMsgKind status, ZstEntityBase * entity);
	void destroy_plug_complete(ZstMsgKind status, ZstPlug * plug);


	// --------------
	// Hooks
	// --------------

	ZstEntityLeavingEvent * m_performer_leaving_hook;
	ZstEntityLeavingEvent * m_component_leaving_hook;
	ZstPlugLeavingEvent * m_plug_leaving_hook;

	// --------------
	// Event managers
	// --------------
	
	ZstEventQueue * m_performer_arriving_event_manager;
	ZstEventQueue * m_performer_leaving_event_manager;
	ZstEventQueue * m_component_arriving_event_manager;
	ZstEventQueue * m_component_leaving_event_manager;
	ZstEventQueue * m_component_type_arriving_event_manager;
	ZstEventQueue * m_component_type_leaving_event_manager;
	ZstEventQueue * m_plug_arriving_event_manager;
	ZstEventQueue * m_plug_leaving_event_manager;
		
	int m_num_graph_recv_messages;
	int m_num_graph_send_messages;
};


class ZstEntityLeavingEvent : public ZstEntityEvent {
	virtual void run(ZstEntityBase * target) override;
};


class ZstPlugLeavingEvent : public ZstPlugEvent {
	virtual void run(ZstPlug * target) override;
};