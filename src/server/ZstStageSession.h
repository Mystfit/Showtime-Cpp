#include <cf/cfuture.h>

#include "../core/ZstSemaphore.h"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/ZstSession.h"
#include "../core/ZstMessageSupervisor.hpp"
#include "ZstStageModule.h"
#include "ZstStageHierarchy.h"
#include "ZstPerformerStageProxy.h"

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

	void on_receive_msg(ZstMessage * msg) override;
	ZstMsgKind synchronise_client_graph(ZstPerformer * client);
	
	ZstMsgKind create_cable_handler(ZstMessage * msg, ZstPerformerStageProxy * sender);
	ZstMsgKind create_cable_complete_handler(ZstCable * cable);
	ZstMsgKind destroy_cable_handler(ZstMessage * msg);
	ZstMsgKind observe_entity_handler(ZstMessage * msg, ZstPerformerStageProxy * sender);

	//Adapter overrides
	void on_performer_leaving(ZstPerformer * performer) override;
	void on_entity_leaving(ZstEntityBase * entity) override;
	void on_plug_leaving(ZstPlug * plug) override;

	void disconnect_cables(ZstEntityBase * entity);
	void destroy_cable(ZstCable * cable) override;


	// -------
	// Clients
	// -------

	void connect_clients(const ZstMsgID & response_id, ZstPerformerStageProxy * output_client, ZstPerformerStageProxy * input_client);
	ZstMsgKind complete_client_connection(ZstPerformerStageProxy * output_client, ZstPerformerStageProxy * input_client);


	// -------
	// Modules
	// -------

	ZstStageHierarchy * hierarchy() override;

private:
	ZstMessageSupervisor m_connection_watcher;
	ZstStageHierarchy * m_hierarchy;
};
