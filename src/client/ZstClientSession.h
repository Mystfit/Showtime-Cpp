#pragma once

/*
	ZstClientSession
*/

//Core API
#include <ZstCore.h>
#include "../core/ZstMessage.h"
#include "../core/ZstSession.h"

//Client modules
#include "ZstClientModule.h"
#include "ZstClientHierarchy.h"
#include "ZstReaper.h"

//Liasons
#include "../core/liasons/ZstCableLiason.hpp"
#include "../core/liasons/ZstPlugLiason.hpp"
#include "../core/liasons/ZstSynchronisableLiason.hpp"

//Adaptors
#include <adaptors/ZstSessionAdaptor.hpp>
#include <adaptors/ZstSynchronisableAdaptor.hpp>

class ZstClientSession : 
	public ZstSession,
	public ZstClientModule
{
public:
	ZstClientSession();
	~ZstClientSession();


	// ------------------------------
	// Delegator overrides
	// ------------------------------

	void init(std::string name);
	void init() override {};
	void destroy() override;
	void process_events();
	void flush();


	// ---------------------------
	// Connection events
	// ---------------------------

	void dispatch_connected_to_stage();
	void dispatch_disconnected_from_stage();
	void plug_received_value(ZstInputPlug * plug);


	// ---------------------------
	// Adaptor plug send/receive
	// ---------------------------

	void on_receive_from_performance(ZstMessage * msg) override;
	void on_plug_fire(ZstOutputPlug * plug) override;
	void on_receive_from_stage(size_t payload_index, ZstMessage * msg) override;


	// -------------------------------
	// Adaptor syncronisable methods
	// -------------------------------

	void on_synchronisable_destroyed(ZstSynchronisable * synchronisable) override;
	void synchronisable_has_event(ZstSynchronisable * synchronisable) override;


	// ------------------
	// Cable creation
	// ------------------

	ZstCable * connect_cable(ZstPlug * input, ZstPlug * output, bool async = false) override;
	void destroy_cable(ZstCable * cable, bool async = false) override;


	// -----------------
	// Event dispatchers
	// -----------------
	
	ZstEventDispatcher<ZstStageDispatchAdaptor*> & stage_events();
	ZstEventDispatcher<ZstPerformanceDispatchAdaptor*> & performance_events();

	
	// -----------------
	// Modules
	// -----------------

	ZstClientHierarchy * hierarchy() override;


private:
	// ----------------
	// Event completion
	// ----------------

	void connect_cable_complete(ZstMessageReceipt response, ZstCable * cable);
	void destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable);

	ZstEventDispatcher<ZstStageDispatchAdaptor*> m_stage_events;
	ZstEventDispatcher<ZstPerformanceDispatchAdaptor*> m_performance_events;
	ZstEventDispatcher<ZstSynchronisableAdaptor*> m_synchronisable_events;

	ZstReaper * m_reaper;
	ZstClientHierarchy * m_hierarchy;
};
