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
	public ZstStageModule,
	public ZstStageTransportAdaptor
{
public:
	ZstStageSession();
	~ZstStageSession();
	virtual void process_events() override;
	virtual void set_wake_condition(std::weak_ptr<ZstSemaphore> condition) override;

	void on_receive_msg(std::shared_ptr<ZstStageMessage> msg) override;


	// ----------------
	// Message handlers
	// ----------------
	Signal signal_handler(const SignalMessage* request, ZstPerformerStageProxy* sender);
	Signal synchronise_client_graph_handler(ZstPerformerStageProxy* sender);
	Signal create_cable_handler(const StageMessage* request, ZstPerformerStageProxy* sender);
	Signal destroy_cable_handler(const CableDestroyRequest* request);
	Signal observe_entity_handler(const StageMessage* request, ZstPerformerStageProxy* sender);
	Signal aquire_entity_ownership_handler(const EntityTakeOwnershipRequest* request, ZstPerformerStageProxy* sender);

	// Adapter overrides
	void on_performer_leaving(ZstPerformer* performer) override;
	void on_entity_leaving(ZstEntityBase* entity) override;

	// Cables
	void disconnect_cables(ZstEntityBase* entity);
	void destroy_cable(ZstCable* cable) override;


	// -------
	// Clients
	// -------

	void connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client);
	void connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client, const ZstMessageReceivedAction& on_msg_received);
	Signal complete_client_connection(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client);


	// -------
	// Modules
	// -------

	std::shared_ptr<ZstHierarchy> hierarchy() override;
	std::shared_ptr<ZstStageHierarchy> stage_hierarchy();

private:
	std::shared_ptr<ZstStageHierarchy> m_hierarchy;
};

}