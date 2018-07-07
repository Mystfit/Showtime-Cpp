#include "../core/adaptors/ZstStageDispatchAdaptor.hpp"
#include "../core/ZstSession.h"
#include "ZstStageHierarchy.h"
#include "ZstPerformerStageProxy.h"

class ZstStageSession : 
	public ZstSession,
	public ZstStageDispatchAdaptor
{
public:
	ZstStageSession();
	~ZstStageSession();
	void init() override;
	void destroy() override;

	void on_receive_msg(ZstStageMessage * msg) override;

	ZstStageMessage * create_cable_handler(ZstStageMessage * msg, ZstPerformer * performer);
	ZstStageMessage * create_cable_complete_handler(ZstCable * cable);
	ZstStageMessage * destroy_cable_handler(ZstStageMessage * msg);

	// -------
	// Clients
	// -------

	ZstPerformerStageProxy * get_client_from_socket_id(const std::string & socket_id);
	std::string get_socket_ID(const ZstPerformer * performer);

	void connect_clients(const std::string & response_id, ZstPerformerStageProxy * output_client, ZstPerformerStageProxy * input_client);
	ZstStageMessage * complete_client_connection_handler(ZstStageMessage * msg, ZstPerformerStageProxy * input_client);

	ZstHierarchy * hierarchy() override;

private:
	ZstClientSocketMap m_client_socket_index;
	ZstStageHierarchy * m_hierarchy;
};
