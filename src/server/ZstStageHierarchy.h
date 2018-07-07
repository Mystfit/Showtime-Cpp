#include <map>
#include <string>

#include "../core/adaptors/ZstStageDispatchAdaptor.hpp"
#include "../core/ZstHierarchy.h"
#include "../core/ZstStageMessage.h"
#include "ZstPerformerStageProxy.h"

typedef std::unordered_map<std::string, ZstPerformerStageProxy*> ZstClientSocketMap;

class ZstStageHierarchy : 
	public ZstHierarchy,
	public ZstStageDispatchAdaptor
{
public:
	~ZstStageHierarchy();
	void destroy() override;
	
	void on_receive_msg(ZstStageMessage * msg) override;

	void destroy_client(ZstPerformer * performer);

	//Message handlers
	ZstStageMessage * create_client_handler(std::string sender_identity, ZstStageMessage * msg);
	ZstStageMessage * destroy_client_handler(ZstPerformer * performer);

	template <typename T>
	ZstStageMessage * create_entity_handler(ZstStageMessage * msg, ZstPerformer * performer);
	ZstStageMessage * destroy_entity_handler(ZstStageMessage * msg);

	ZstStageMessage * create_entity_template_handler(ZstStageMessage * msg);
	ZstStageMessage * create_entity_from_template_handler(ZstStageMessage * msg);
};