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
#include "../core/ZstStageMessage.h"
#include "../core/ZstPerformanceMessage.h"

//Client modules
#include "ZstClientModule.h"
#include "ZstClientHierarchy.h"

//Liasons
#include "../core/liasons/ZstCableLiason.hpp"
#include "../core/liasons/ZstPlugLiason.hpp"
#include "../core/liasons/ZstSynchronisableLiason.hpp"

//Adaptors
#include "../core/adaptors/ZstGraphTransportAdaptor.hpp"
#include "../core/adaptors/ZstStageTransportAdaptor.hpp"

#include "adaptors/ZstSessionAdaptor.hpp"
#include "adaptors/ZstSynchronisableAdaptor.hpp"

namespace showtime::client {

class ZstClientSession : 
	public ZstSession,
	public ZstClientModule,
    public virtual ZstStageTransportAdaptor,
	public virtual ZstGraphTransportAdaptor
{
	friend class ZstClient;
public:
	ZstClientSession();

	// ------------------------------
	// Delegator overrides
	// ------------------------------

	void init(std::string name);
	void process_events() override;
	void flush_events() override;


	// ---------------------------
	// Connection events
	// ---------------------------

	void dispatch_connected_to_stage();
	void dispatch_disconnected_from_stage();
	void plug_received_value(ZstInputPlug * plug);


	// ---------------------------
	// Adaptor plug send/receive
	// ---------------------------

	virtual void on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg) override;
    virtual void on_receive_msg(const std::shared_ptr<ZstPerformanceMessage>& msg) override;
    
    
    // ----------------
    // Message handlers
    // ----------------
    
    void cable_create_handler(const CableCreateRequest* request);
    void cable_destroy_handler(const CableDestroyRequest* request);
    void aquire_entity_ownership_handler(const EntityTakeOwnershipRequest* request);

    
	// ---------------------------
	// Hierarchy adaptor overrides
	// ---------------------------
	void on_performer_leaving(const ZstURI& performer_path) override;
    

	// ------------------
	// Cable creation
	// ------------------

	ZstCable * connect_cable(ZstInputPlug * input, ZstOutputPlug * output, const ZstTransportRequestBehaviour & sendtype) override;
	void destroy_cable(ZstCable * cable, const ZstTransportRequestBehaviour & sendtype) override;
	bool observe_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype) override;
    
    
    // ---------------------------
    // Ownership
    // ---------------------------
    
    virtual void aquire_entity_ownership(ZstEntityBase* entity) override;
    virtual void release_entity_ownership(ZstEntityBase* entity) override;


	// -----------------
	// Submodules access
	// -----------------

	virtual std::shared_ptr<ZstHierarchy> hierarchy() override;


private:
	// ----------------
	// Event completion
	// ----------------

	void connect_cable_complete(ZstMessageResponse response, ZstCable * cable);
	void destroy_cable_complete(ZstMessageResponse response, ZstCable * cable);
	void observe_entity_complete(ZstMessageResponse response, ZstEntityBase * entity);
	
    std::shared_ptr<ZstClientHierarchy> m_hierarchy;
};

}
