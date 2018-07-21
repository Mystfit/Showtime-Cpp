#include <cf/cfuture.h>

#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/ZstSession.h"
#include "ZstStageModule.h"
#include "ZstStageHierarchy.h"
#include "ZstPerformerStageProxy.h"

class ZstStageSession : 
	public ZstSession,
	public ZstHierarchyAdaptor,
	public ZstStageModule
{
public:
	ZstStageSession();
	~ZstStageSession();
	void init() override;
	void destroy() override;

	void on_receive_msg(ZstMessage * msg) override;
	ZstMsgKind synchronise_client_graph(ZstPerformer * client);
	
	ZstMsgKind create_cable_handler(ZstMessage * msg, ZstPerformer * performer);
	ZstMsgKind create_cable_complete_handler(ZstCable * cable);
	ZstMsgKind destroy_cable_handler(ZstMessage * msg);

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
	ZstMsgKind complete_client_connection_handler(ZstMessage * msg, ZstPerformerStageProxy * input_client);


	// -------
	// Modules
	// -------

	ZstStageHierarchy * hierarchy() override;

private:
	std::unordered_map<ZstMsgID, MessagePromise> m_deferred_cables;

	ZstStageHierarchy * m_hierarchy;
};
