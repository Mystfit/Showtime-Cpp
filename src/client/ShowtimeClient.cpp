#include <showtime/ShowtimeClient.h>
#include <showtime/adaptors/ZstSessionAdaptor.hpp>

#include "ZstClient.h"
#include "ZstClientSession.h"
#include "../core/ZstEventDispatcher.hpp"

using namespace std;
using namespace showtime::client;

namespace showtime {

inline bool ShowtimeClient::library_init_guard() {
	if (!m_client->is_init_complete()) {
		Log::net(Log::Level::error, "Showtime library has not been initialised."); 
		return false;
	} 
	return true;
}

inline bool ShowtimeClient::library_connected_guard() {
	if (!library_init_guard()) {
		return false;
	}

	if (!m_client->is_connected_to_stage()) {
		Log::net(Log::Level::warn, "Not connected to a Showtime stage.");
		return false;
	}
	return true;
}


ShowtimeClient::ShowtimeClient() : m_client(std::make_shared<showtime::client::ZstClient>(this))
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
	if (library_init_guard()) m_client->join_stage(ZstServerAddress{ "", stage_address }, ZstTransportRequestBehaviour::SYNC_REPLY);
}

void ShowtimeClient::join_async(const char * stage_address){
	if (library_init_guard()) m_client->join_stage(ZstServerAddress{ "", stage_address }, ZstTransportRequestBehaviour::ASYNC_REPLY);
}

void ShowtimeClient::join_by_name(const char * stage_name)
{
	if (!library_init_guard()) return;
	auto discovered_server = m_client->get_discovered_server(stage_name);

	if (strcmp(discovered_server.name.c_str(), stage_name) != 0) {
		Log::net(Log::Level::error, "Could not find server {}", stage_name);
		return;
	}
	join(discovered_server.address.c_str());
	return;
}

void ShowtimeClient::join_by_name_async(const char * stage_name)
{
    if (!library_init_guard()) return;
	auto discovered_server = m_client->get_discovered_server(stage_name);

	if (strcmp(discovered_server.name.c_str(), stage_name) != 0) {
		Log::net(Log::Level::error, "Could not find server {}", stage_name);
		return;
	}
	join_async(discovered_server.address.c_str());
	return;
}

void ShowtimeClient::auto_join_by_name(const char * name)
{
    if(library_init_guard()) m_client->auto_join_stage(name, ZstTransportRequestBehaviour::SYNC_REPLY);
}

void ShowtimeClient::auto_join_by_name_async(const char * name)
{
    if(library_init_guard()) m_client->auto_join_stage(name, ZstTransportRequestBehaviour::ASYNC_REPLY);
}

void ShowtimeClient::get_discovered_servers(ZstServerAddressBundle* servers)
{
    if (!library_init_guard()) return;
    auto servers_list = m_client->get_discovered_servers();
    for(auto s : servers_list){
        servers->add(s);
    }
}

ZstServerAddress ShowtimeClient::get_discovered_server(const char* server_name)
{
	return m_client->get_discovered_server(server_name);
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

void ShowtimeClient::add_connection_adaptor(std::shared_ptr < ZstConnectionAdaptor > adaptor)
{
    m_client->ZstEventDispatcher<ZstConnectionAdaptor>::add_adaptor(adaptor);
}

void ShowtimeClient::add_session_adaptor(std::shared_ptr < ZstSessionAdaptor > adaptor)
{
	m_client->session()->session_events()->add_adaptor(adaptor);
}

void ShowtimeClient::add_hierarchy_adaptor(std::shared_ptr < ZstHierarchyAdaptor > adaptor)
{
	m_client->session()->hierarchy()->hierarchy_events()->add_adaptor(adaptor);
}

void ShowtimeClient::add_plugin_adaptor(std::shared_ptr<ZstPluginAdaptor> adaptor)
{
	m_client->plugins()->plugin_events()->add_adaptor(adaptor);
}

void ShowtimeClient::add_log_adaptor(std::shared_ptr<ZstLogAdaptor> adaptor)
{
	m_client->ZstEventDispatcher<ZstLogAdaptor>::add_adaptor(adaptor);
}

void ShowtimeClient::remove_connection_adaptor(std::shared_ptr < ZstConnectionAdaptor > adaptor)
{
    m_client->ZstEventDispatcher<ZstConnectionAdaptor>::remove_adaptor(adaptor);
}

void ShowtimeClient::remove_session_adaptor(std::shared_ptr < ZstSessionAdaptor > adaptor)
{
	m_client->session()->session_events()->remove_adaptor(adaptor);
}

void ShowtimeClient::remove_hierarchy_adaptor(std::shared_ptr < ZstHierarchyAdaptor > adaptor)
{
	m_client->session()->hierarchy()->hierarchy_events()->remove_adaptor(adaptor);
}

void ShowtimeClient::remove_plugin_adaptor(std::shared_ptr<ZstPluginAdaptor> adaptor)
{
	m_client->plugins()->plugin_events()->remove_adaptor(adaptor);
}

void ShowtimeClient::remove_log_adaptor(std::shared_ptr<ZstLogAdaptor> adaptor)
{
	m_client->ZstEventDispatcher<ZstLogAdaptor>::remove_adaptor(adaptor);
}

ZstConnectionAdaptor* ShowtimeClient::connection_events()
{
	return m_client->ZstEventDispatcher<ZstConnectionAdaptor>::get_default_adaptor().get();
}

ZstSessionAdaptor* ShowtimeClient::session_events()
{
	return m_client->session()->session_events()->get_default_adaptor().get();
}

ZstHierarchyAdaptor* ShowtimeClient::hierarchy_events()
{
	return m_client->session()->hierarchy()->hierarchy_events()->get_default_adaptor().get();
}

ZstPluginAdaptor* ShowtimeClient::plugin_events()
{
	return m_client->plugins()->plugin_events()->get_default_adaptor().get();
}

ZstLogAdaptor* ShowtimeClient::log_events()
{
	return m_client->ZstEventDispatcher<ZstLogAdaptor>::get_default_adaptor().get();
}


// ------------------------------
// Entity activation/deactivation
// ------------------------------

//void activate_entity(ZstEntityBase * entity)
//{
//    if (library_connected_guard()) m_client->session()->hierarchy()->activate_entity(entity, ZstTransportRequestBehaviour::SYNC_REPLY);
//}

//void ShowtimeClient::activate_entity_async(ZstEntityBase * entity)
//{
//    if (library_connected_guard()) m_client->session()->hierarchy()->activate_entity(entity, ZstTransportRequestBehaviour::ASYNC_REPLY);
//}

void ShowtimeClient::register_entity(ZstEntityBase* entity)
{
	if (library_init_guard()) {
		if (!entity->is_registered()) {
			m_client->session()->register_entity(entity);
			m_client->session()->hierarchy()->register_entity(entity);
		}
	}
}

void ShowtimeClient::deactivate_entity(ZstEntityBase * entity)
{
	if (library_connected_guard()) m_client->session()->hierarchy()->deactivate_entity(entity, ZstTransportRequestBehaviour::SYNC_REPLY);
}

void ShowtimeClient::deactivate_entity_async(ZstEntityBase * entity)
{
	if (library_connected_guard()) m_client->session()->hierarchy()->deactivate_entity(entity, ZstTransportRequestBehaviour::ASYNC_REPLY);
}

void ShowtimeClient::observe_entity(ZstEntityBase * entity)
{
	if (library_connected_guard()) m_client->session()->observe_entity(entity, ZstTransportRequestBehaviour::SYNC_REPLY);
}

void ShowtimeClient::observe_entity_async(ZstEntityBase * entity)
{
	if (library_connected_guard()) m_client->session()->observe_entity(entity, ZstTransportRequestBehaviour::ASYNC_REPLY);
}


// ---------
// Factories
// ---------

ZstEntityBase * ShowtimeClient::create_entity(const ZstURI & creatable_path, const char * name)
{
	return m_client->session()->hierarchy()->create_entity(creatable_path, name, ZstTransportRequestBehaviour::SYNC_REPLY);
}

void ShowtimeClient::create_entity_async(const ZstURI & creatable_path, const char * name)
{
	m_client->session()->hierarchy()->create_entity(creatable_path, name, ZstTransportRequestBehaviour::ASYNC_REPLY);
}

void ShowtimeClient::register_factory(ZstEntityFactory * factory)
{
	if (!library_init_guard()) return;

	//Add the factory to the root performer first to allow for offline factory registration
	get_root()->add_child(factory);

	if (library_connected_guard()) m_client->session()->hierarchy()->activate_entity(factory, ZstTransportRequestBehaviour::SYNC_REPLY);
}

void ShowtimeClient::register_factory_async(ZstEntityFactory * factory)
{
	if (!library_init_guard()) return;

	//Add the factory to the root performer first to allow for offline factory registration
	get_root()->add_child(factory);

	if (library_connected_guard()) m_client->session()->hierarchy()->activate_entity(factory, ZstTransportRequestBehaviour::ASYNC_REPLY);
}

// -------------
// Hierarchy
// -------------

ZstPerformer * ShowtimeClient::get_root()
{
	return m_client->session()->hierarchy()->get_local_performer();
}

ZstEntityBase* ShowtimeClient::find_entity(const ZstURI & path)
{
	if (!library_init_guard()) return NULL;
	return m_client->session()->hierarchy()->find_entity(path);
}

void ShowtimeClient::get_performers(ZstEntityBundle* bundle)
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
	return m_client->session()->connect_cable(input, output, ZstTransportRequestBehaviour::SYNC_REPLY);
}

ZstCable * ShowtimeClient::connect_cable_async(ZstInputPlug * input, ZstOutputPlug * output)
{
	if (!library_connected_guard()) return NULL;
	return m_client->session()->connect_cable(input, output, ZstTransportRequestBehaviour::ASYNC_REPLY);
}

void ShowtimeClient::destroy_cable(ZstCable * cable)
{
	if (library_connected_guard()) m_client->session()->destroy_cable(cable, ZstTransportRequestBehaviour::SYNC_REPLY);
}

void ShowtimeClient::destroy_cable_async(ZstCable * cable)
{
	if (library_connected_guard()) m_client->session()->destroy_cable(cable, ZstTransportRequestBehaviour::ASYNC_REPLY);
}

ZstCable* ShowtimeClient::find_cable(const ZstCableAddress& address)
{
	if (!library_init_guard()) return m_client->session()->find_cable(address);
	return NULL;
}

ZstCable* ShowtimeClient::find_cable(const ZstURI& input, const ZstURI& output)
{
	if (!library_init_guard()) return m_client->session()->find_cable(input, output);
	return NULL;
}

void ShowtimeClient::reload_plugins()
{
	if (!library_init_guard()) return;
}

std::vector< std::shared_ptr<ZstPlugin> > ShowtimeClient::plugins() {
	if (!library_init_guard()) return std::vector< std::shared_ptr<ZstPlugin> >();
	return m_client->plugins()->get_plugins();
}

void ShowtimeClient::set_plugin_path(const char* path) {
	m_client->plugins()->set_plugin_path(path);
}

const char* ShowtimeClient::get_plugin_path() {
	return m_client->plugins()->get_plugin_path();
}

void ShowtimeClient::set_plugin_data_path(const char* path) {
	m_client->plugins()->set_plugin_data_path(path);
}

const char* ShowtimeClient::get_plugin_data_path() {
	return m_client->plugins()->get_plugin_data_path();
}

}
