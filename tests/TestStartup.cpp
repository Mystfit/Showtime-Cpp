#include "TestCommon.hpp"

using namespace ZstTest;

void test_startup()
{
    zst_init("TestStartup", true);
    //zst_start_file_logging();

    //--------------------
    //Test destroy
    ZstLog::app(LogLevel::debug, "Testing early destruction of library");
    zst_destroy();
    ZstLog::app(LogLevel::debug, "Testing aborting join before init");
    zst_join("127.0.0.1");
    ZstLog::app(LogLevel::debug, "Testing double library init");
    zst_init("TestClient", true);
    zst_init("TestClient", true);
    
    //--------------------
    ZstLog::app(LogLevel::debug, "Testing sync join");
    zst_join("127.0.0.1");
    assert(zst_is_connected());
    ZstLog::app(LogLevel::debug, "Testing sync leave");
    zst_leave();

    //TODO: Pause to let zmq finish disconnecting - a bit ugly
    TAKE_A_BREATH
    assert(!zst_is_connected());

    //Test sync join again to verify we cleaned up properly the first time
    ZstLog::app(LogLevel::debug, "Testing sync join again");
    zst_join("127.0.0.1");
    assert(zst_is_connected());

    //Testing join not starting if we're already connected
    ZstLog::app(LogLevel::debug, "Testing abort connection start if we're already connected");
    zst_join("127.0.0.1");
    assert(!zst_is_connecting());
    zst_leave();

    //TODO: Pause to let zmq finish disconnecting - a bit ugly
    TAKE_A_BREATH
    assert(!zst_is_connected());

    //Test async join
    TestConnectionEvents * connectCallback = new TestConnectionEvents();
    zst_add_session_adaptor(connectCallback);

    ZstLog::app(LogLevel::debug, "Testing async join");
    assert(connectCallback->num_calls() == 0);
    zst_join_async("127.0.0.1");
    wait_for_event(connectCallback, 1);
    assert(connectCallback->num_calls() == 1);
    assert(zst_is_connected());
    connectCallback->reset_num_calls();
    zst_leave();
    assert(!zst_is_connected());

    //Test join timeout
    ZstLog::app(LogLevel::debug, "Testing sync join timeout");
    zst_join("255.255.255.255");
    assert(!zst_is_connected());

    //Test async join timeout
    ZstLog::app(LogLevel::debug, "Testing async join timeout");
    zst_join_async("255.255.255.255");
    WAIT_UNTIL_STAGE_TIMEOUT
    assert(!zst_is_connected());
    
    //Testing abort connection start if we're already connecting
    ZstLog::app(LogLevel::debug, "Testing async abort connection start if we're already connecting");
    zst_join_async("255.255.255.255");
    assert(zst_is_connecting());
    zst_join("255.255.255.255");
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

	zst_join("127.0.0.1");
    
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
    test_startup();
    test_root_entity();
}
