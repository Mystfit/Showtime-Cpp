#include <map>
#include <string>

#include "../core/ZstHierarchy.h"
#include "../core/ZstStageMessage.h"
#include "ZstPerformerStageProxy.h"
#include "ZstStageModule.h"

typedef std::unordered_map<std::string, ZstPerformerStageProxy*> ZstClientSocketMap;

class ZstStageHierarchy : 
	public ZstHierarchy,
	public ZstTransportAdaptor,
	public ZstStageModule
{
public:
	~ZstStageHierarchy();
	void destroy() override;
	

	// ---------------------------
	// Hierarchy adaptor overrides
	// ---------------------------

	void on_receive_msg(ZstMessage * msg) override;
	

	// ----------------
	// Clients
	// ----------------

	ZstMsgKind create_client_handler(std::string sender_identity, ZstMessage * msg);
	ZstMsgKind destroy_client_handler(ZstPerformer * performer);


	// ----------------
	// Proxies
	// ----------------
	ZstMsgKind add_proxy_entity(const ZstEntityBase & entity, ZstMsgID request_ID, ZstPerformer * sender);
	ZstMsgKind update_proxy_entity(const ZstEntityBase & entity, ZstMsgID request_ID);
	ZstMsgKind remove_proxy_entity(ZstEntityBase * entity) override;


	// ------------------------
	// Factories and creatables
	// ------------------------

	ZstMsgKind create_entity_from_factory_handler(ZstMessage * msg, ZstPerformerStageProxy * sender);


	// ---------------------
	// Socket IDs
	// ---------------------

	ZstPerformerStageProxy * get_client_from_socket_id(const std::string & socket_id);
	std::string get_socket_ID(const ZstPerformer * performer);

private:
	ZstClientSocketMap m_client_socket_index;
	std::unordered_map<ZstMsgID, MessagePromise> m_deferred_creatable_promises;

};