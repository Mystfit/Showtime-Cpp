#include <map>
#include <string>

#include "../core/ZstHierarchy.h"
#include "../core/ZstStageMessage.h"
#include "ZstPerformerStageProxy.h"

typedef std::unordered_map<std::string, ZstPerformerStageProxy*> ZstClientSocketMap;

class ZstStageHierarchy : 
	public ZstHierarchy,
	public ZstTransportAdaptor
{
public:
	~ZstStageHierarchy();
	void destroy() override;
	
	void on_receive_msg(ZstMessage * msg) override;

	void destroy_client(ZstPerformer * performer);

	//Message handlers
	ZstMsgKind create_client_handler(std::string sender_identity, ZstMessage * msg);
	ZstMsgKind destroy_client_handler(ZstPerformer * performer);

	ZstMsgKind add_proxy_entity(ZstEntityBase & entity) override;
	void remove_proxy_entity(ZstEntityBase * entity) override;

	ZstMsgKind create_entity_template_handler(ZstMessage * msg);
	ZstMsgKind create_entity_from_template_handler(ZstMessage * msg);

	ZstPerformerStageProxy * get_client_from_socket_id(const std::string & socket_id);
	std::string get_socket_ID(const ZstPerformer * performer);

private:
	ZstClientSocketMap m_client_socket_index;

	ZstEventDispatcher<ZstTransportAdaptor*> m_router_events;
	ZstEventDispatcher<ZstTransportAdaptor*> m_publisher_events;
};