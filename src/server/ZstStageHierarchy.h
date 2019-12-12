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
	void destroy() override;

	virtual void set_wake_condition(std::weak_ptr<ZstSemaphore> condition) override;
	ZstPerformer* get_local_performer() const override;
	virtual void process_events() override;

	// ---------------------------
	// Hierarchy adaptor overrides
	// ---------------------------

	void on_receive_msg(std::shared_ptr<ZstStageMessage> msg) override;


	// ----------------
	// Message handlers
	// ----------------

	Signal signal_handler(const SignalMessage* request, ZstPerformerStageProxy* sender);
	Signal create_client_handler(const ClientJoinRequest * request, uuid endpoint_UUID);
	Signal client_leaving_handler(const ClientLeaveRequest* request, ZstPerformerStageProxy* sender);
	Signal create_entity_handler(const EntityCreateRequest* request, ZstPerformerStageProxy* sender);
	Signal factory_create_entity_handler(const StageMessage* request, ZstPerformerStageProxy* sender);
	Signal update_entity_handler(const EntityUpdateRequest* request);
	Signal destroy_entity_handler(const EntityDestroyRequest* request);


	// ----------------
	// Messaging
	// ----------------
	void reply_with_signal(ZstPerformerStageProxy* performer, Signal signal, ZstMsgID request_id);
	void broadcast(Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder> & buffer_builder, const ZstTransportArgs& args);
	void whisper(ZstPerformerStageProxy* performer, Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder> buffer_builder, const ZstTransportArgs& args);


	// ----------------
	// Proxies
	// ----------------

	virtual void on_request_entity_registration(ZstEntityBase* entity) override;
	Signal ZstStageHierarchy::destroy_client(ZstPerformerStageProxy* performer);


	// ---------------------
	// Socket IDs
	// ---------------------

	ZstPerformerStageProxy* get_client_from_endpoint_UUID(const uuid& endpoint_UUID);

};

}