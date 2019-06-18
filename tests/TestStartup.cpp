#include "TestCommon.hpp"

using namespace ZstTest;


void test_stage_discovery()
{
    ZstLog::app(LogLevel::debug, "Testing discovery of existing servers");
    zst_init("TestDiscovery", true);
    WAIT_UNTIL_STAGE_TIMEOUT
    ZstServerBundle bundle;
    zst_get_discovered_servers(bundle);
    assert(bundle.size() > 0);
    assert(bundle.item_at(0).first == "TestStartup_server");
    
    clear_callback_queue();
    auto discovery_adaptor = std::make_shared<TestConnectionEvents>();
    zst_add_session_adaptor(discovery_adaptor.get());
    
    ZstLog::app(LogLevel::debug, "Testing server discovery event adaptor");
    auto detected_server_name = std::string("detected_server");
    auto detected_server = zst_create_server(detected_server_name.c_str(), STAGE_ROUTER_PORT + 10);
    
    wait_for_event(discovery_adaptor.get(), 1);
    assert(discovery_adaptor->num_calls() == 1);
    bundle.clear();
    zst_get_discovered_servers(bundle);
    assert(bundle.size() == 2);
    assert(bundle.item_at(1).first == detected_server_name);
    
    //Cleanup
    zst_destroy_server(detected_server);
    zst_remove_session_adaptor(discovery_adaptor.get());
}

void test_startup()
{
    zst_init("TestStartup", true);
    //zst_start_file_logging();

    //--------------------
    //Test destroy
    ZstLog::app(LogLevel::debug, "Testing early destruction of library");
    zst_destroy();
    ZstLog::app(LogLevel::debug, "Testing aborting join before init");
    ZstServerBundle bundle;
    zst_get_discovered_servers(bundle);
//    std::string server_address = bundle.item_at(0).second.c_str();
    std::string server_address = "127.0.0.1:40004";
    std::string server_name = "TestStartup_server";
    zst_join(server_address.c_str());
    ZstLog::app(LogLevel::debug, "Testing double library init");
    zst_init("TestClient", true);
    zst_init("TestClient", true);
    
    
    //--------------------
    ZstLog::app(LogLevel::debug, "Testing sync join");
    zst_join(server_address.c_str());
    assert(zst_is_connected());
    ZstLog::app(LogLevel::debug, "Testing sync leave");
    zst_leave();
    
    //Test sync join again to verify we cleaned up properly the first time
    ZstLog::app(LogLevel::debug, "Testing sync join again");
    zst_join(server_address.c_str());
    assert(zst_is_connected());
    zst_leave();

    // -------------------
    ZstLog::app(LogLevel::debug, "Testing joining by name");
    zst_join_by_name(server_name.c_str());
    assert(zst_is_connected());
    zst_leave();
    
    ZstLog::app(LogLevel::debug, "Testing autojoin");
    zst_auto_join();
    assert(zst_is_connected());
    zst_leave();

    //Testing join not starting if we're already connected
    ZstLog::app(LogLevel::debug, "Testing abort connection start if we're already connected");
    zst_join(server_address.c_str());
    zst_join(server_address.c_str());
    assert(!zst_is_connecting());
    zst_leave();

    //Test async join
    TestConnectionEvents * connectCallback = new TestConnectionEvents();
    zst_add_session_adaptor(connectCallback);

    ZstLog::app(LogLevel::debug, "Testing async join");
    assert(connectCallback->num_calls() == 0);
    zst_join_async(server_address.c_str());
    wait_for_event(connectCallback, 1);
    assert(connectCallback->num_calls() == 1);
    assert(zst_is_connected());
    connectCallback->reset_num_calls();
    zst_leave();
    assert(!zst_is_connected());

    //Test join timeout
    ZstLog::app(LogLevel::debug, "Testing sync join timeout");
    zst_join("255.255.255.255:1111");
    assert(!zst_is_connected());

    //Test async join timeout
    ZstLog::app(LogLevel::debug, "Testing async join timeout");
    zst_join_async("255.255.255.255:1111");
    WAIT_UNTIL_STAGE_TIMEOUT
    assert(!zst_is_connected());
    
    //Testing abort connection start if we're already connecting
    ZstLog::app(LogLevel::debug, "Testing async abort connection start if we're already connecting");
    zst_join_async("255.255.255.255:1111");
    assert(zst_is_connecting());
    zst_join("255.255.255.255:1111");
    assert(!zst_is_connected());
    WAIT_UNTIL_STAGE_TIMEOUT
    assert(!zst_is_connecting());
	
    //Cleanup
    zst_remove_session_adaptor(connectCallback);
    delete connectCallback;
}

void test_URI()
{
    //Run URI self test
    ZstURI::self_test();

    //RUn cable self test
    ZstCable::self_test();
}

void test_root_entity()
{
    ZstLog::app(LogLevel::notification, "Running performer test");
    
    ZstServerBundle bundle;
    zst_get_discovered_servers(bundle);
    std::string server_address = bundle.item_at(0).second.c_str();
	zst_join(server_address.c_str());
    
    //Test root entity is activated
    TestSynchronisableEvents * performer_activated = new TestSynchronisableEvents();
    ZstPerformer * root_entity = zst_get_root();
    assert(root_entity);
    
    //This should execute immediately since we've already connected to the stage
    root_entity->add_adaptor(performer_activated);
    assert(performer_activated->num_calls() == 1);
    performer_activated->reset_num_calls();
    assert(root_entity->is_activated());
    clear_callback_queue();
    root_entity->remove_adaptor(performer_activated);
    delete performer_activated;
    ZstLog::app(LogLevel::debug, "Root performer is activated");
}

int main(int argc,char **argv)
{
	TestRunner runner("TestStartup", argv[0], false);
    test_URI();
    test_stage_discovery();
    test_startup();
    test_root_entity();
}
