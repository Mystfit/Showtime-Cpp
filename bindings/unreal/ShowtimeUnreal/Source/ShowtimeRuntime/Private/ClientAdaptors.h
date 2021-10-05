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

private:
	UShowtimeClient* Owner;
};
