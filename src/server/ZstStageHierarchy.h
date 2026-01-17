#include <map>
#include <string>
#include <vector>
#include <functional>
#include <boost/uuid/uuid.hpp>
#include <boost/container_hash/hash.hpp>
#include <showtime/schemas/messaging/session_generated.h>

#include "../core/ZstSemaphore.h"
#include "../core/ZstHierarchy.h"
#include "../core/ZstStageMessage.h"
#include "../core/adaptors/ZstStageTransportAdaptor.hpp"
#include "ZstPerformerStageProxy.h"
#include "ZstStageModule.h"

namespace showtime {

// Callback type for entity reclaimed events
// Parameters: reclaimed entity, owning performer
using EntityReclaimedCallback = std::function<void(ZstEntityBase*, ZstPerformerStageProxy*)>;

typedef std::unordered_map<boost::uuids::uuid, ZstPerformerStageProxy*, boost::hash<boost::uuids::uuid> > ZstClientEndpointMap;

class ZstStageHierarchy :
	public ZstHierarchy,
	public ZstStageTransportAdaptor,
	public ZstStageModule,
	public ZstSerialisable<Hierarchy, void>
{
public:
	~ZstStageHierarchy();
	virtual void init_adaptors() override;
	virtual void set_wake_condition(std::shared_ptr<std::condition_variable>& condition) override;
	ZstPerformer* get_local_performer() const override;
	virtual void process_events() override;

	// ---------------------------
	// Hierarchy adaptor overrides
	// ---------------------------

	virtual void on_entity_arriving(ZstEntityBase* entity) override;
	virtual void on_factory_arriving(ZstEntityFactory* factory) override;
	virtual void on_performer_arriving(ZstPerformer* performer) override;
	void on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg) override;

	// ---------------------------
	// Serialisable overrides
	// ---------------------------
	void serialize_partial(flatbuffers::Offset<void>& destination_offset, flatbuffers::FlatBufferBuilder& buffer_builder) const override;
	flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder& buffer_builder) const override;
	void deserialize_partial(const void* buffer) override;
	void deserialize(const Hierarchy* buffer) override;


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
	void broadcast(showtime::Content message_content_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& builder, const ZstTransportArgs& args, const std::vector<ZstPerformer*> & excluded = std::vector<ZstPerformer*>());
	void whisper(ZstPerformerStageProxy* performer, showtime::Content message_content_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& builder, const ZstTransportArgs& args);
	void whisper(ZstPerformerStageProxy* performer, flatbuffers::DetachedBuffer&& message_buffer, const ZstTransportArgs& args);

	void client_leaving(ZstPerformer* performer, const ClientLeaveReason& reason);

	// ----------------
	// Offline entity management
	// ----------------

	void set_preserve_entities_on_disconnect(bool preserve);
	bool get_preserve_entities_on_disconnect() const;
	void mark_performer_offline(ZstPerformer* performer);
	bool has_offline_entities(const ZstURI& performer_uri) const;
	void get_offline_entities_for_performer(const ZstURI& performer_uri, ZstEntityBundle& bundle) const;
	void send_offline_entities_notification(ZstPerformerStageProxy* performer);
	void transfer_entity_to_performer(ZstEntityBase* entity, ZstPerformerStageProxy* new_owner);
	Signal entity_reclaim_handler(const std::shared_ptr<ZstStageMessage>& request, ZstPerformerStageProxy* sender);

	// Callback for when an entity is reclaimed (used by session to sync cables)
	void set_entity_reclaimed_callback(EntityReclaimedCallback callback);

	// ----------------
	// Proxies
	// ----------------

	virtual void request_entity_registration(ZstEntityBase* entity) override;

	// ---------------------
	// Socket IDs
	// ---------------------

	ZstPerformerStageProxy* get_client_from_endpoint_UUID(const uuid& origin_endpoint_UUID);

private:
	bool m_preserve_entities_on_disconnect = false;
	// Storage for offline entities by original performer URI
	std::unordered_map<ZstURI, std::vector<ZstEntityBase*>, ZstURIHash> m_offline_entities;

	// Callback invoked when an entity is reclaimed
	EntityReclaimedCallback m_entity_reclaimed_callback;

	// Entity creation source tracking (entity URI -> {source, factory_path})
	struct EntityCreationInfo {
		EntityCreationSource source;
		ZstURI factory_path;  // Only valid if source == FACTORY

		EntityCreationInfo() : source(EntityCreationSource_MANUAL), factory_path() {}
		EntityCreationInfo(EntityCreationSource s, const ZstURI& path = ZstURI())
			: source(s), factory_path(path) {}
	};
	std::unordered_map<ZstURI, EntityCreationInfo, ZstURIHash> m_entity_creation_sources;

	// Pending factory entity creations (creatable_path -> factory_path)
	std::unordered_map<ZstURI, ZstURI, ZstURIHash> m_pending_factory_entities;
};

}
