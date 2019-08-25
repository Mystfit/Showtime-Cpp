#pragma once

#include "ZstCore.h"
#include <memory>

//Forwards
namespace Showtime {
    namespace detail {
        class ZstClient;
    }
}

class ZST_CLASS_EXPORTED ShowtimeClient : std::enable_shared_from_this<ShowtimeClient>
{
public:
    ZST_CLIENT_EXPORT ShowtimeClient();
    
    //Disable copying
    ZST_CLIENT_EXPORT ShowtimeClient(const ShowtimeClient & other) = delete;
    
	//Init the library
	ZST_CLIENT_EXPORT void init(const char * performer, bool debug);
	ZST_CLIENT_EXPORT void start_file_logging(const char * log_file_path = "");
    
    //Connect to servers
	ZST_CLIENT_EXPORT void join(const char * stage_address);
    ZST_CLIENT_EXPORT void join_async(const char * stage_address);
    ZST_CLIENT_EXPORT void join_by_name(const char * stage_name);
    ZST_CLIENT_EXPORT void join_by_name_async(const char * stage_name);
    ZST_CLIENT_EXPORT void auto_join_by_name(const char * name);
    ZST_CLIENT_EXPORT void auto_join_by_name_async(const char * name);
    ZST_CLIENT_EXPORT void get_discovered_servers(ZstServerAddressBundle & servers);

	//Cleanup
	ZST_CLIENT_EXPORT void destroy();
	ZST_CLIENT_EXPORT void leave();

	//Poll the event queue - for runtimes that have process events from the main thread
	ZST_CLIENT_EXPORT void poll_once();

	//Adaptors
    ZST_CLIENT_EXPORT void add_connection_adaptor(ZstConnectionAdaptor * adaptor);
	ZST_CLIENT_EXPORT void add_session_adaptor(ZstSessionAdaptor * adaptor);
	ZST_CLIENT_EXPORT void add_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor);
    ZST_CLIENT_EXPORT void remove_connection_adaptor(ZstConnectionAdaptor * adaptor);
	ZST_CLIENT_EXPORT void remove_session_adaptor(ZstSessionAdaptor * adaptor);
	ZST_CLIENT_EXPORT void remove_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor);
	
	//Entity activation/deactivation
	ZST_CLIENT_EXPORT void deactivate_entity(ZstEntityBase * entity);
    ZST_CLIENT_EXPORT void deactivate_entity_async(ZstEntityBase * entity);
	ZST_CLIENT_EXPORT void observe_entity(ZstEntityBase * entity);
	ZST_CLIENT_EXPORT void observe_entity_async(ZstEntityBase * entity);

	//Factories
	ZST_CLIENT_EXPORT ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name);
	ZST_CLIENT_EXPORT void create_entity_async(const ZstURI & creatable_path, const char * name);
	ZST_CLIENT_EXPORT void register_factory(ZstEntityFactory * factory);
	ZST_CLIENT_EXPORT void register_factory_async(ZstEntityFactory * factory);

	//Hierarchy
	ZST_CLIENT_EXPORT ZstPerformer* get_root();
	ZST_CLIENT_EXPORT ZstPerformer* get_performer_by_URI(const ZstURI & path);
	ZST_CLIENT_EXPORT ZstEntityBase* find_entity(const ZstURI & path);
	ZST_CLIENT_EXPORT void get_performers(ZstEntityBundle & bundle);

	//Stage methods
	ZST_CLIENT_EXPORT bool is_connected();
	ZST_CLIENT_EXPORT bool is_connecting();
    ZST_CLIENT_EXPORT bool is_init_completed();
	ZST_CLIENT_EXPORT int ping();

	//Cable management
	ZST_CLIENT_EXPORT ZstCable * connect_cable(ZstInputPlug * input, ZstOutputPlug * output);
    ZST_CLIENT_EXPORT ZstCable * connect_cable_async(ZstInputPlug * input, ZstOutputPlug * output);
	ZST_CLIENT_EXPORT void destroy_cable(ZstCable * cable);
    ZST_CLIENT_EXPORT void destroy_cable_async(ZstCable * cable);
    
private:
    bool library_init_guard();
    bool library_connected_guard();
    std::shared_ptr<Showtime::detail::ZstClient> m_client;
};
