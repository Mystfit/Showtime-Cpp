#pragma once

#include <ZstCore.h>
#include "../core/ZstMessage.h"
#include "../core/ZstEventDispatcher.h"
#include "../core/ZstEventQueue.h"
#include "ZstClientModule.h"


class ZstCableNetwork : public ZstClientModule
{
	friend class ZstCableLeavingEvent;

public:
	ZstCableNetwork(ZstClient * client);
	~ZstCableNetwork();
	
	ZstCable * connect_cable(ZstPlug * input, ZstPlug * output, bool async = false);
	void destroy_cable(ZstCable * cable, bool async = false);
	void disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug);

	// -------------
	// Cable queries
	// -------------
	
	ZstCable * find_cable(const ZstURI & input_path, const ZstURI & output_path);
	ZstCable * find_cable(ZstPlug * input, ZstPlug * output);


	// -------------
	// Events
	// -------------

	ZstEventQueue & cable_arriving_events();
	ZstEventQueue & cable_leaving_events();

private:
	ZstCableNetwork();

	// -------------
	// Event hooks
	// -------------
	
	ZstCableLeavingEvent * m_cable_leaving_hook;


	// ----------------
	// Event completion
	// ----------------

	void connect_cable_complete(ZstMsgKind status, ZstCable * cable);
	void destroy_cable_complete(ZstMsgKind status, ZstCable * cable);


	// --------------
	// Cable creation
	// --------------
	 
	ZstCable * create_cable_ptr(const ZstCable & cable);
	ZstCable * create_cable_ptr(ZstPlug * output, ZstPlug * input);
	ZstCable * create_cable_ptr(const ZstURI & input_path, const ZstURI & output_path);


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


class ZstCableLeavingEvent : public ZstCableEvent {
	virtual void run(ZstCable * target) override;
};


