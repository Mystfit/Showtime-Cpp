#pragma once

/*
	ZstClientSession
*/

//Core API
#include "ZstCore.h"
#include "../core/ZstPerformanceMessage.h"
#include "../core/ZstStageMessage.h"
#include "../core/ZstMessage.h"
#include "../core/ZstSession.h"

//Client modules
#include "ZstClientModule.h"
#include "ZstClientHierarchy.h"

//Liasons
#include "../core/liasons/ZstCableLiason.hpp"
#include "../core/liasons/ZstPlugLiason.hpp"
#include "../core/liasons/ZstSynchronisableLiason.hpp"

//Adaptors
#include "adaptors/ZstSessionAdaptor.hpp"
#include "adaptors/ZstSynchronisableAdaptor.hpp"

class ZstClientSession : 
	public ZstSession,
	public ZstClientModule
{
	friend class ZstClient;
public:
	ZstClientSession();
	virtual ~ZstClientSession();


	// ------------------------------
	// Delegator overrides
	// ------------------------------

	void init(std::string name);
	void destroy() override;
	void process_events() override;
	void flush_events() override;


	// ---------------------------
	// Connection events
	// ---------------------------

	void dispatch_connected_to_stage();
	void dispatch_disconnected_from_stage();
    void dispatch_server_discovered(const ZstServerAddress & server);
	void plug_received_value(ZstInputPlug * plug);


	// ---------------------------
	// Adaptor plug send/receive
	// ---------------------------

	void on_receive_msg(ZstMessage * msg) override;
	void on_receive_graph_msg(ZstPerformanceMessage * msg);
	virtual void aquire_plug_fire_control(ZstOutputPlug* plug) override;
	void aquire_plug_fire_control_handler(ZstMessage* msg);


	// ---------------------------
	// Hierarchy adaptor overrides
	// ---------------------------
	void on_performer_leaving(ZstPerformer * performer) override;


	// ------------------
	// Cable creation
	// ------------------

	ZstCable * connect_cable(ZstInputPlug * input, ZstOutputPlug * output, const ZstTransportSendType & sendtype) override;
	void destroy_cable(ZstCable * cable, const ZstTransportSendType & sendtype) override;
	bool observe_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype) override;


	// -----------------
	// Submodules
	// -----------------

	ZstClientHierarchy * hierarchy() override;


private:
	// ----------------
	// Event completion
	// ----------------

	void connect_cable_complete(ZstMessageReceipt response, ZstCable * cable);
	void destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable);
	void observe_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity);
	
	ZstClientHierarchy * m_hierarchy;
};
