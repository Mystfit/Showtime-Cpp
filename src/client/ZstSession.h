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
#include <adaptors/ZstPlugAdaptors.hpp>

class ZstSession :
	public ZstClientModule,
	public ZstEventDispatcher<ZstSessionAdaptor*>,
	public ZstEventDispatcher<ZstStageAdaptor*>,
	private ZstSessionAdaptor,
	private ZstStageAdaptor,
	private ZstOutputPlugAdaptor,
	private ZstSynchronisableAdaptor,
	private ZstPlugLiason,
	private ZstCableLiason,
	private ZstSynchronisableLiason
{
	using ZstEventDispatcher<ZstSessionAdaptor*>::process_events;
	using ZstEventDispatcher<ZstSessionAdaptor*>::run_event;
	using ZstEventDispatcher<ZstSessionAdaptor*>::add_event;
	using ZstEventDispatcher<ZstStageAdaptor*>::run_event;
	using ZstEventDispatcher<ZstSessionAdaptor*>::flush;
	using ZstEventDispatcher<ZstStageAdaptor*>::flush;

public:
	ZstSession(ZstClient * client);
	~ZstSession();


	// ------------------------------
	// Delegator overrides
	// ------------------------------

	void init() override;
	void destroy() override;
	void process_events();
	void flush();


	// ---------------------------
	// Adaptor plug send/receive
	// ---------------------------

	void on_receive_from_performance(ZstPerformanceMessage * msg) override;
	void on_plug_fire(ZstOutputPlug * plug) override;
	void on_plug_received_value(ZstInputPlug * plug) override;
	
	void on_receive_from_stage(int payload_index, ZstStageMessage * msg) override;

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


private:
	ZstSession();

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
