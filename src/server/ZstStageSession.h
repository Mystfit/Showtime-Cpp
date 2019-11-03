#include <cf/cfuture.h>

#include "../core/ZstSemaphore.h"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/ZstSession.h"
#include "ZstStageModule.h"
#include "ZstStageHierarchy.h"
#include "ZstPerformerStageProxy.h"

namespace showtime {

class ZstStageSession :
	public ZstSession,
	public ZstStageModule
{
public:
	ZstStageSession();
	~ZstStageSession();
	void destroy() override;
	virtual void process_events() override;
	virtual void set_wake_condition(std::weak_ptr<ZstSemaphore> condition) override;

	void on_receive_msg(ZstMessage* msg) override;


	// ----------------
	// Message handlers
	// ----------------

	ZstMsgKind synchronise_client_graph_handler(ZstStageMessage* msg, ZstPerformer* sender);
	ZstMsgKind create_cable_handler(ZstStageMessage* msg, ZstPerformerStageProxy* sender);
	ZstMsgKind create_cable_complete_handler(ZstCable* cable);
	ZstMsgKind destroy_cable_handler(ZstStageMessage* msg);
	ZstMsgKind observe_entity_handler(ZstStageMessage* msg, ZstPerformerStageProxy* sender);
	ZstMsgKind aquire_entity_ownership_handler(ZstStageMessage* msg, ZstPerformerStageProxy* sender);
	ZstMsgKind release_entity_ownership_handler(ZstStageMessage* msg, ZstPerformerStageProxy* sender);

	//Adapter overrides
	void on_performer_leaving(ZstPerformer* performer) override;
	void on_entity_leaving(ZstEntityBase* entity) override;
	void on_plug_leaving(ZstPlug* plug) override;

	void disconnect_cables(ZstEntityBase* entity);
	void destroy_cable(ZstCable* cable) override;


	// -------
	// Clients
	// -------

	void connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client);
	void connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client, const ZstMessageReceivedAction& on_msg_received);
	ZstMsgKind complete_client_connection(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client);


	// -------
	// Modules
	// -------

	std::shared_ptr<ZstHierarchy> hierarchy() override;
	std::shared_ptr<ZstStageHierarchy> stage_hierarchy();

private:
	std::shared_ptr<ZstStageHierarchy> m_hierarchy;
};

}