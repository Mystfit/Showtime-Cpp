#pragma once

#include <showtime/ShowtimeClient.h>

// Forwards
class UShowtimeSubsystem;

class ClientAdaptors :
	//public showtime::ZstSessionAdaptor,
	//public showtime::ZstPluginAdaptor,
	//public showtime::ZstHierarchyAdaptor,
	public showtime::ZstConnectionAdaptor,
	public showtime::ZstLogAdaptor
{
public:
	ClientAdaptors(UShowtimeSubsystem* owner);

	// Connection adaptor overrides

	void on_connected_to_server(showtime::ShowtimeClient* client, const showtime::ZstServerAddress* server) override;
	void on_disconnected_from_server(showtime::ShowtimeClient* client, const showtime::ZstServerAddress* server) override;
	void on_synchronised_graph(showtime::ShowtimeClient* client, const showtime::ZstServerAddress* server) override;
	
	// Log adaptor overrides

	void on_formatted_log_record(const char* record) override;

private:
	UShowtimeSubsystem* Owner;
};
