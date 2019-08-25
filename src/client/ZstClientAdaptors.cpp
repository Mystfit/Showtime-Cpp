#include "adaptors/ZstConnectionAdaptor.hpp"

void ZstConnectionAdaptor::on_connected_to_stage(ShowtimeClient* client, const ZstServerAddress & server){}
void ZstConnectionAdaptor::on_disconnected_from_stage(ShowtimeClient* client, const ZstServerAddress & server){}
void ZstConnectionAdaptor::on_server_discovered(ShowtimeClient* client, const ZstServerAddress & server){}
void ZstConnectionAdaptor::on_synchronised_with_stage(ShowtimeClient* client, const ZstServerAddress & server){}
