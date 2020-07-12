#pragma once

#include <showtime/ShowtimeClient.h>

// Forwards
class UShowtimeClient;

class ClientAdaptors :
	public showtime::ZstSessionAdaptor,
	public showtime::ZstPluginAdaptor,
	public showtime::ZstHierarchyAdaptor,
	public showtime::ZstConnectionAdaptor,
	public showtime::ZstLogAdaptor
{
public:
	ClientAdaptors(UShowtimeClient* owner);

	// Connection adaptor overrides

	void on_connected_to_server(showtime::ShowtimeClient* client, const showtime::ZstServerAddress* server) override;
	void on_disconnected_from_server(showtime::ShowtimeClient* client, const showtime::ZstServerAddress* server) override;
	void on_server_discovered(showtime::ShowtimeClient* client, const showtime::ZstServerAddress* server) override;
	void on_server_lost(showtime::ShowtimeClient* client, const showtime::ZstServerAddress* server) override;
	void on_synchronised_graph(showtime::ShowtimeClient* client, const showtime::ZstServerAddress* server) override;
	
	// Log adaptor overrides

	void on_formatted_log_record(const char* record) override;

	// Hierarchy adaptor overrides

	void on_performer_arriving(showtime::ZstPerformer* performer) override;
	void on_performer_leaving(const showtime::ZstURI& performer_path) override;
	void on_entity_arriving(showtime::ZstEntityBase* entity) override;
	void on_entity_leaving(const showtime::ZstURI& entity_path) override;
	void on_entity_updated(showtime::ZstEntityBase* entity) override;
	void on_factory_arriving(showtime::ZstEntityFactory* factory) override;
	void on_factory_leaving(const showtime::ZstURI& factory_path) override;

	// Session adaptor overrides

	void on_cable_created(showtime::ZstCable* cable) override;
	void on_cable_destroyed(const showtime::ZstCableAddress& cable_address) override;

	// Plugin adaptor overrides

	//void on_plugin_loaded(std::shared_ptr<ZstPlugin> plugin) override;
	//void on_plugin_unloaded(std::shared_ptr<ZstPlugin> plugin) override;


private:
	UShowtimeClient* Owner;
};
