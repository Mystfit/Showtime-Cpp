#include <map>
#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/container_hash/hash.hpp>

#include "../core/ZstSemaphore.h"
#include "../core/ZstHierarchy.h"
#include "../core/ZstStageMessage.h"
#include "../core/adaptors/ZstStageTransportAdaptor.hpp"
#include "ZstPerformerStageProxy.h"
#include "ZstStageModule.h"

namespace showtime {

typedef std::unordered_map<boost::uuids::uuid, ZstPerformerStageProxy*, boost::hash<boost::uuids::uuid> > ZstClientEndpointMap;

class ZstStageHierarchy :
	public ZstHierarchy,
	public ZstStageTransportAdaptor,
	public ZstStageModule
{
public:
	~ZstStageHierarchy();
	virtual void init_adaptors() override;
	virtual void set_wake_condition(std::shared_ptr<ZstSemaphore>& condition) override;
	ZstPerformer* get_local_performer() const override;
	virtual void process_events() override;

	// ---------------------------
	// Hierarchy adaptor overrides
	// ---------------------------

	virtual void on_entity_arriving(ZstEntityBase* entity) override;
	virtual void on_factory_arriving(ZstEntityFactory* factory) override;
	virtual void on_performer_arriving(ZstPerformer* performer) override;
	void on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg) override;


	// ----------------
	// Message handlers
	// ----------------

	Signal signal_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender);
	Signal create_client_handler(const std::shared_ptr<ZstStageMessage>& request);
	Signal client_leaving_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender);
	Signal create_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender);
	Signal factory_create_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender);
	Signal update_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender);
	Signal destroy_entity_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender);


	// ----------------
	// Messaging
	// ----------------
	void reply_with_signal(ZstPerformerStageProxy* performer, Signal signal, ZstMsgID request_id);
	void broadcast(Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder> & buffer_builder, const ZstTransportArgs& args, const std::vector<ZstPerformer*> & excluded = std::vector<ZstPerformer*>());
	void whisper(ZstPerformerStageProxy* performer, Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args);


	// ----------------
	// Proxies
	// ----------------

	virtual void request_entity_registration(ZstEntityBase* entity) override;


	// ---------------------
	// Socket IDs
	// ---------------------

	ZstPerformerStageProxy* get_client_from_endpoint_UUID(const uuid& origin_endpoint_UUID);
};

}