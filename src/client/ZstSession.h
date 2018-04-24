/*
	ZstPerformance
*/


#pragma once

//Core API
#include <ZstCore.h>
#include "../core/ZstMessage.h"

//Client modules
#include "ZstClientModule.h"
#include "ZstHierarchy.h"
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

class ZstSession :
	public ZstClientModule,
	public ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>,
	public ZstEventDispatcher<ZstStageDispatchAdaptor*>,
	public ZstEventDispatcher<ZstSessionAdaptor*>,
	public ZstEventDispatcher<ZstSynchronisableAdaptor*>,
	public ZstSessionAdaptor,
	public ZstStageDispatchAdaptor,
	public ZstPerformanceDispatchAdaptor,
	public ZstOutputPlugAdaptor,
	public ZstSynchronisableAdaptor,
	public ZstPlugLiason,
	public ZstCableLiason,
	public ZstSynchronisableLiason
{

public:
	using ZstEventDispatcher<ZstSessionAdaptor*>::add_adaptor;
	using ZstEventDispatcher<ZstSessionAdaptor*>::remove_adaptor;
	using ZstEventDispatcher<ZstSessionAdaptor*>::process_events;
	using ZstEventDispatcher<ZstSessionAdaptor*>::run_event;
	using ZstEventDispatcher<ZstSessionAdaptor*>::add_event;
	using ZstEventDispatcher<ZstSessionAdaptor*>::flush;
	using ZstEventDispatcher<ZstStageDispatchAdaptor*>::add_adaptor;
	using ZstEventDispatcher<ZstStageDispatchAdaptor*>::remove_adaptor;
	using ZstEventDispatcher<ZstStageDispatchAdaptor*>::run_event;
	using ZstEventDispatcher<ZstStageDispatchAdaptor*>::flush;
	using ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>::run_event;

	ZstSession();
	~ZstSession();


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
	ZstCable * connect_cable(ZstPlug * input, ZstPlug * output, bool async = false);
	void destroy_cable(ZstCable * cable, bool async = false);
	void disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug);


	// -------------
	// Cable queries
	// -------------

	ZstCable * find_cable(const ZstURI & input_path, const ZstURI & output_path);
	ZstCable * find_cable(ZstPlug * input, ZstPlug * output);


	// -------------
	// Modules
	// -------------
	ZstHierarchy * hierarchy();

private:
	// --------------------------
	// Cable creation/destruction
	// --------------------------

	ZstCable * create_cable(const ZstCable & cable);
	ZstCable * create_cable(ZstPlug * output, ZstPlug * input);
	ZstCable * create_cable(const ZstURI & input_path, const ZstURI & output_path);

	// ----------------
	// Event completion
	// ----------------

	void connect_cable_complete(ZstMessageReceipt response, ZstCable * cable);
	void destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable);


	ZstCableList m_cables;
	ZstHierarchy * m_hierarchy;
	ZstReaper * m_reaper;
};
