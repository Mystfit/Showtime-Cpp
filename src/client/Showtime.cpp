#include "Showtime.h"
#include "ZstClient.h"
#include "ZstClientSession.h"
#include "adaptors/ZstSessionAdaptor.hpp"

using namespace std;

inline bool ShowtimeClient::library_init_guard() {
	if (!m_client->is_init_complete()) {
		ZstLog::net(LogLevel::error, "Showtime library has not been initialised."); 
		return false;
	} 
	return true;
}

inline bool ShowtimeClient::library_connected_guard() {
	if (!library_init_guard()) {
		return false;
	}

	if (!m_client->is_connected_to_stage()) {
		ZstLog::net(LogLevel::warn, "Not connected to a Showtime stage.");
		return false;
	}
	return true;
}


ShowtimeClient::ShowtimeClient() : m_client(std::make_shared<Showtime::detail::ZstClient>(this))
{
}

// -----------------
// Initialisation
// -----------------

void ShowtimeClient::init(const char * performer_name, bool debug)
{
	m_client->init_client(performer_name, debug);
}

void ShowtimeClient::start_file_logging(const char * log_file_path)
{
	m_client->init_file_logging(log_file_path);
}

void ShowtimeClient::join(const char * stage_address){
	if(library_init_guard()) m_client->join_stage(stage_address, ZstTransportSendType::SYNC_REPLY);
}

void ShowtimeClient::join_async(const char * stage_address){
	if (library_init_guard()) m_client->join_stage(stage_address, ZstTransportSendType::ASYNC_REPLY);
}

void ShowtimeClient::join_by_name(const char * stage_name)
{
    if (!library_init_guard()) return;
    auto servers_list = m_client->get_discovered_servers();
    for(auto server : servers_list){
        if(strcmp(stage_name, server.name.c_str()) == 0){
            join(server.address.c_str());
            return;
        }
    }
    ZstLog::net(LogLevel::error, "Could not find server {}", stage_name);
}

void ShowtimeClient::join_by_name_async(const char * stage_name)
{
    if (!library_init_guard()) return;
    auto servers_list = m_client->get_discovered_servers();
    for(auto server : servers_list){
        if(strcmp(stage_name, server.name.c_str()) == 0){
            join_async(server.address.c_str());
            return;
        }
    }
    ZstLog::net(LogLevel::error, "Could not find server {}", stage_name);
}

void ShowtimeClient::auto_join_by_name(const char * name)
{
    if(library_init_guard()) m_client->auto_join_stage(name, ZstTransportSendType::SYNC_REPLY);
}

void ShowtimeClient::auto_join_by_name_async(const char * name)
{
    if(library_init_guard()) m_client->auto_join_stage(name, ZstTransportSendType::ASYNC_REPLY);
}

void ShowtimeClient::get_discovered_servers(ZstServerAddressBundle & servers)
{
    if (!library_init_guard()) return;
    auto servers_list = m_client->get_discovered_servers();
    for(auto s : servers_list){
        servers.add(s);
    }
}


// -----------------
// Cleanup
// -----------------

void ShowtimeClient::destroy()
{
	if (library_init_guard()) m_client->destroy();
}

void ShowtimeClient::leave()
{
	if (library_init_guard()) m_client->leave_stage();
}


// -----------------
// Event polling
// -----------------

void ShowtimeClient::poll_once()
{
	if (library_init_guard()) m_client->process_events();
}

void ShowtimeClient::add_connection_adaptor(ZstConnectionAdaptor * adaptor)
{
    if (library_init_guard()) m_client->ZstEventDispatcher<ZstConnectionAdaptor*>::add_adaptor(adaptor);
}

void ShowtimeClient::add_session_adaptor(ZstSessionAdaptor * adaptor)
{
	if (library_init_guard()) m_client->session()->session_events().add_adaptor(adaptor);
}

void ShowtimeClient::add_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor)
{
	if (library_init_guard()) m_client->session()->hierarchy()->hierarchy_events().add_adaptor(adaptor);
}

void ShowtimeClient::remove_connection_adaptor(ZstConnectionAdaptor * adaptor)
{
    if (library_init_guard()) m_client->ZstEventDispatcher<ZstConnectionAdaptor*>::remove_adaptor(adaptor);
}

void ShowtimeClient::remove_session_adaptor(ZstSessionAdaptor * adaptor)
{
	if (library_init_guard()) m_client->session()->session_events().remove_adaptor(adaptor);
}

void ShowtimeClient::remove_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor)
{
	if (library_init_guard()) m_client->session()->hierarchy()->hierarchy_events().remove_adaptor(adaptor);
}



// ------------------------------
// Entity activation/deactivation
// ------------------------------

//void activate_entity(ZstEntityBase * entity)
//{
//    if (library_connected_guard()) m_client->session()->hierarchy()->activate_entity(entity, ZstTransportSendType::SYNC_REPLY);
//}

//void ShowtimeClient::activate_entity_async(ZstEntityBase * entity)
//{
//    if (library_connected_guard()) m_client->session()->hierarchy()->activate_entity(entity, ZstTransportSendType::ASYNC_REPLY);
//}

void ShowtimeClient::deactivate_entity(ZstEntityBase * entity)
{
	if (library_connected_guard()) m_client->session()->hierarchy()->destroy_entity(entity, ZstTransportSendType::SYNC_REPLY);
}

void ShowtimeClient::deactivate_entity_async(ZstEntityBase * entity)
{
	if (library_connected_guard()) m_client->session()->hierarchy()->destroy_entity(entity, ZstTransportSendType::ASYNC_REPLY);
}

void ShowtimeClient::observe_entity(ZstEntityBase * entity)
{
	if (library_connected_guard()) m_client->session()->observe_entity(entity, ZstTransportSendType::SYNC_REPLY);
}

void ShowtimeClient::observe_entity_async(ZstEntityBase * entity)
{
	if (library_connected_guard()) m_client->session()->observe_entity(entity, ZstTransportSendType::ASYNC_REPLY);
}


// ---------
// Factories
// ---------

ZstEntityBase * ShowtimeClient::create_entity(const ZstURI & creatable_path, const char * name)
{
	return m_client->session()->hierarchy()->create_entity(creatable_path, name, ZstTransportSendType::SYNC_REPLY);
}

void ShowtimeClient::create_entity_async(const ZstURI & creatable_path, const char * name)
{
	m_client->session()->hierarchy()->create_entity(creatable_path, name, ZstTransportSendType::ASYNC_REPLY);
}

void ShowtimeClient::register_factory(ZstEntityFactory * factory)
{
	if (!library_init_guard()) return;

	//Add the factory to the root performer first to allow for offline factory registration
	get_root()->add_child(factory);

	if (library_connected_guard()) m_client->session()->hierarchy()->activate_entity(factory, ZstTransportSendType::SYNC_REPLY);
}

void ShowtimeClient::register_factory_async(ZstEntityFactory * factory)
{
	if (!library_init_guard()) return;

	//Add the factory to the root performer first to allow for offline factory registration
	get_root()->add_child(factory);

	if (library_connected_guard()) m_client->session()->hierarchy()->activate_entity(factory, ZstTransportSendType::ASYNC_REPLY);
}

// -------------
// Hierarchy
// -------------

ZstPerformer * ShowtimeClient::get_root()
{
	return m_client->session()->hierarchy()->get_local_performer();
}

ZstPerformer * ShowtimeClient::get_performer_by_URI(const ZstURI & path)
{
	if (!library_init_guard()) return NULL;
    return m_client->session()->hierarchy()->get_performer_by_URI(path);
}

ZstEntityBase* ShowtimeClient::find_entity(const ZstURI & path)
{
	if (!library_init_guard()) return NULL;
	return m_client->session()->hierarchy()->find_entity(path);
}

void ShowtimeClient::get_performers(ZstEntityBundle & bundle)
{
	if (!library_init_guard()) return;
	m_client->session()->hierarchy()->get_performers(bundle);
}



// -------------
// Stage status
// -------------

bool ShowtimeClient::is_connected()
{
	return m_client->is_connected_to_stage();
}

bool ShowtimeClient::is_connecting()
{
	return m_client->is_connecting_to_stage();
}

bool ShowtimeClient::is_init_completed()
{
    return m_client->is_init_complete();
}

int ShowtimeClient::ping()
{
	return m_client->ping();
}


// -------------
// Cables
// -------------

ZstCable * ShowtimeClient::connect_cable(ZstInputPlug * input, ZstOutputPlug * output)
{
	if (!library_connected_guard()) return NULL;
	return m_client->session()->connect_cable(input, output, ZstTransportSendType::SYNC_REPLY);
}

ZstCable * ShowtimeClient::connect_cable_async(ZstInputPlug * input, ZstOutputPlug * output)
{
	if (!library_connected_guard()) return NULL;
	return m_client->session()->connect_cable(input, output, ZstTransportSendType::ASYNC_REPLY);
}

void ShowtimeClient::destroy_cable(ZstCable * cable)
{
	if (library_connected_guard()) m_client->session()->destroy_cable(cable, ZstTransportSendType::SYNC_REPLY);
}

void ShowtimeClient::destroy_cable_async(ZstCable * cable)
{
	if (library_connected_guard()) m_client->session()->destroy_cable(cable, ZstTransportSendType::ASYNC_REPLY);
}
