#pragma once

#include <ZstCore.h>
#include "ZstClient.h"
#include "../core/liasons/ZstSynchronisableLiason.hpp"
#include "../core/liasons/ZstCableLiason.hpp"
#include "../core/liasons/ZstPlugLiason.hpp"
#include "ZstClientModule.h"

class ZstCableLeavingEvent : public ZstCableEvent {
	virtual void run(ZstCable * target) override;
};


class ZstCableNetwork : 
	public ZstClientModule, 
	private ZstPlugLiason, 
	private ZstCableLiason, 
	private ZstSynchronisableLiason
{
public:
	ZstCableNetwork(ZstClient * client);
	~ZstCableNetwork();
	
	ZstCable * connect_cable(ZstPlug * input, ZstPlug * output, bool async = false);
	void destroy_cable(ZstCable * cable, bool async = false);
	void disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug);


	// --------------
	// Cable creation
	// --------------

	ZstCable * create_cable(const ZstCable & cable);
	ZstCable * create_cable(ZstPlug * output, ZstPlug * input);
	ZstCable * create_cable(const ZstURI & input_path, const ZstURI & output_path);


	// -------------
	// Cable queries
	// -------------
	
	ZstCable * find_cable(const ZstURI & input_path, const ZstURI & output_path);
	ZstCable * find_cable(ZstPlug * input, ZstPlug * output);


	// -------------
	// Events
	// -------------

	ZstEventQueue * cable_arriving_events();
	ZstEventQueue * cable_leaving_events();

private:
	ZstCableNetwork();

	// -------------
	// Event hooks
	// -------------
	
	ZstCableLeavingEvent * m_cable_leaving_hook;


	// ----------------
	// Event completion
	// ----------------

	void connect_cable_complete(ZstMessageReceipt response, ZstCable * cable);
	void destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable);
	

	// --------------------
	// Cable event managers
	// --------------------

	ZstEventQueue * m_cable_arriving_event_manager;
	ZstEventQueue * m_cable_leaving_event_manager;
	
	
	// -----------
	// Cable lists
	// -----------

	ZstCableList m_cables;
	ZstINetworkInteractor * m_client;
};


