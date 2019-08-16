#include <map>
#include <string>
#include <boost/uuid/uuid.hpp>

#include "../core/ZstSemaphore.h"
#include "../core/ZstHierarchy.h"
#include "../core/ZstStageMessage.h"
#include "ZstPerformerStageProxy.h"
#include "ZstStageModule.h"

typedef std::unordered_map<boost::uuids::uuid, ZstPerformerStageProxy*> ZstClientEndpointMap;

class ZstStageHierarchy : 
	public ZstHierarchy,
	public ZstTransportAdaptor,
	public ZstStageModule
{
public:
	~ZstStageHierarchy();
	void destroy() override;
    
    virtual void set_wake_condition(std::weak_ptr<ZstSemaphore> condition) override;
    ZstPerformer * get_local_performer() const override;


	// ---------------------------
	// Hierarchy adaptor overrides
	// ---------------------------

	void on_receive_msg(ZstMessage * msg) override;
	

	// ----------------
	// Clients
	// ----------------

	ZstMsgKind create_client_handler(ZstStageMessage * msg);
	ZstMsgKind destroy_client_handler(ZstPerformer * performer);
	void broadcast_message(const ZstMsgKind & msg_kind, const ZstMsgArgs & args, const ZstMsgArgs & payload = json());

    
	// ----------------
	// Proxies
	// ----------------
    
	virtual ZstMsgKind add_proxy_entity(const ZstEntityBase & entity, ZstMsgID request_ID, ZstPerformer * sender);
	ZstMsgKind update_proxy_entity(const ZstEntityBase & entity, ZstMsgID request_ID);
	ZstMsgKind remove_proxy_entity(ZstEntityBase * entity) override;


	// ------------------------
	// Factories and creatables
	// ------------------------

	ZstMsgKind create_entity_from_factory_handler(ZstStageMessage * msg, ZstPerformerStageProxy * sender);


	// ---------------------
	// Socket IDs
	// ---------------------

	ZstPerformerStageProxy * get_client_from_endpoint_UUID(const uuid & endpoint_UUID);
	boost::uuids::uuid get_endpoint_UUID_from_client(const ZstPerformer * performer);

private:
	ZstClientEndpointMap m_client_endpoint_UUIDS;
};
