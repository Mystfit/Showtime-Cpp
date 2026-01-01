#include "../core/ZstSemaphore.h"
#include "../core/adaptors/ZstTransportAdaptor.hpp"
#include "../core/ZstSession.h"
#include "ZstStageModule.h"
#include "ZstStageHierarchy.h"
#include "ZstPerformerStageProxy.h"
#include <showtime/ZstFilesystemUtils.h>
#include <showtime/schemas/messaging/session_generated.h>

namespace showtime {

class ZstStageSession :
	public ZstSession,
	public ZstStageModule,
	public ZstStageTransportAdaptor,
	public ZstSerialisable<Session, void>
{
public:
	ZstStageSession();
	~ZstStageSession();
	virtual void process_events() override;
	virtual void set_wake_condition(std::shared_ptr<std::condition_variable>& condition) override;

	void on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg) override;


	// ----------------
	// Message handlers
	// ----------------
	Signal signal_handler(const std::shared_ptr<ZstStageMessage>& msg, ZstPerformerStageProxy* sender);
	Signal synchronise_client_graph_handler(ZstPerformerStageProxy* sender);
	Signal create_cable_handler(const std::shared_ptr<ZstStageMessage>& msg, ZstPerformerStageProxy* sender);
	Signal destroy_cable_handler(const std::shared_ptr<ZstStageMessage>& msg);
	Signal observe_entity_handler(const std::shared_ptr<ZstStageMessage>& msg, ZstPerformerStageProxy* sender);
	Signal aquire_entity_ownership_handler(const std::shared_ptr<ZstStageMessage>& msg, ZstPerformerStageProxy* sender);
	Signal save_session_handler(const std::shared_ptr<ZstStageMessage>& msg, ZstPerformerStageProxy* sender);
	Signal load_session_handler(const std::shared_ptr<ZstStageMessage>& msg, ZstPerformerStageProxy* sender);

	// Adapter overrides
	void on_performer_leaving(ZstPerformer* performer) override;
	void on_entity_leaving(ZstEntityBase* entity_path) override;

	// Cables
	void disconnect_cables(ZstEntityBase* entity);
	void destroy_cable(ZstCable* cable) override;


	// -------
	// Clients
	// -------

	void connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client, ConnectionType connection_type);
	void connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client, ConnectionType connection_type, const ZstMessageReceivedAction& on_msg_received);
	Signal complete_client_connection(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client, ConnectionType connection_type);


	// -------
	// Modules
	// -------

	std::shared_ptr<ZstHierarchy> hierarchy() override;
	std::shared_ptr<ZstStageHierarchy> stage_hierarchy();

	// Session serialization
	bool save_session_to_file(const std::string& filepath);
	bool load_session_from_file(const std::string& filepath);

	// ZstSerialisable implementation
	void serialize_partial(flatbuffers::Offset<void>& destination_offset, flatbuffers::FlatBufferBuilder& buffer_builder) const override;
	flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder& buffer_builder) const override;
	void deserialize_partial(const void* buffer) override;
	void deserialize(const Session* buffer) override;

private:
	std::shared_ptr<ZstStageHierarchy> m_hierarchy;

	fs::path m_session_save_path;
};

}
