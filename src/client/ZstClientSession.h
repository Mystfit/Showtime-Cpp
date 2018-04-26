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
#include "adaptors/ZstStageDispatchAdaptor.hpp"
#include "adaptors/ZstPerformanceDispatchAdaptor.hpp"
#include <adaptors/ZstPlugAdaptors.hpp>
#include <adaptors/ZstSessionAdaptor.hpp>
#include <adaptors/ZstSynchronisableAdaptor.hpp>

class ZstClientSession : 
	public ZstSession,
	public ZstClientModule,
	public ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>,
	public ZstEventDispatcher<ZstStageDispatchAdaptor*>,
	public ZstEventDispatcher<ZstSynchronisableAdaptor*>,
	public ZstStageDispatchAdaptor,
	public ZstPerformanceDispatchAdaptor,
	public ZstOutputPlugAdaptor
{

public:
	using ZstEventDispatcher<ZstSessionAdaptor*>::add_event;
	using ZstEventDispatcher<ZstSessionAdaptor*>::add_adaptor;
	using ZstEventDispatcher<ZstSessionAdaptor*>::remove_adaptor;
	using ZstEventDispatcher<ZstStageDispatchAdaptor*>::add_adaptor;
	using ZstEventDispatcher<ZstStageDispatchAdaptor*>::remove_adaptor;
	using ZstEventDispatcher<ZstStageDispatchAdaptor*>::run_event;
	using ZstEventDispatcher<ZstStageDispatchAdaptor*>::flush;
	using ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>::add_adaptor;
	using ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>::add_event;
	using ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>::run_event;

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
	void on_connected_to_stage() override;
	void on_disconnected_from_stage() override;

	// ---------------------------
	// Adaptor plug send/receive
	// ---------------------------

	void on_receive_from_performance(ZstMessage * msg) override;
	void on_plug_fire(ZstOutputPlug * plug) override;
	void on_plug_received_value(ZstInputPlug * plug) override;
	
	void on_receive_from_stage(size_t payload_index, ZstMessage * msg) override;

	// -------------------------------
	// Adaptor syncronisable methods
	// -------------------------------

	void on_synchronisable_destroyed(ZstSynchronisable * synchronisable) override;


	// ------------------
	// Cable creation
	// ------------------
	ZstCable * connect_cable(ZstPlug * input, ZstPlug * output, bool async = false) override;
	void destroy_cable(ZstCable * cable, bool async = false) override;


	// -------------
	// Modules
	// -------------
	ZstClientHierarchy * hierarchy() override;

private:
	// ----------------
	// Event completion
	// ----------------

	void connect_cable_complete(ZstMessageReceipt response, ZstCable * cable);
	void destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable);

	ZstClientHierarchy * m_hierarchy;
	ZstCableList m_cables;
	ZstReaper * m_reaper;
};
