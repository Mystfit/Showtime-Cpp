#include "TestCommon.hpp"

using namespace ZstTest;

void test_create_entities(){
    ZstLog::app(LogLevel::notification, "Running entity creation test");
    
    OutputComponent * test_output_sync = new OutputComponent("entity_create_test_sync");
    
    ZstLog::app(LogLevel::debug, "Testing entity sync activation");
    zst_activate_entity(test_output_sync);
    assert(test_output_sync->is_activated());
    assert(zst_find_entity(test_output_sync->URI()));
    
    ZstLog::app(LogLevel::debug, "Testing entity sync deactivation");
    zst_deactivate_entity(test_output_sync);
    assert(!test_output_sync->is_activated());
    assert(!zst_find_entity(test_output_sync->URI()));
    delete test_output_sync;
	clear_callback_queue();
    
    //Test async entity
    OutputComponent * test_output_async = new OutputComponent("entity_create_test_async");
    TestSynchronisableEvents * entity_sync = new TestSynchronisableEvents();
    test_output_async->add_adaptor(entity_sync);
        
    ZstLog::app(LogLevel::debug, "Testing entity async activation");
    zst_activate_entity_async(test_output_async);
    wait_for_event(entity_sync, 1);
    assert(entity_sync->num_calls() == 1);
    entity_sync ->reset_num_calls();
    
    //Check local client registered plugs correctly
    assert(test_output_async->is_activated());
    ZstURI localPlug_uri = test_output_async->get_plug_by_URI(test_output_async->output()->URI())->URI();
    ZstURI localPlug_uri_via_entity = test_output_async->output()->URI();
    assert(ZstURI::equal(localPlug_uri, localPlug_uri_via_entity));

    ZstLog::app(LogLevel::debug, "Testing entity async deactivation");
    zst_deactivate_entity_async(test_output_async);
    wait_for_event(entity_sync, 1);
    assert(entity_sync->num_calls() == 1);
    entity_sync->reset_num_calls();
    assert(!test_output_async->is_activated());
    assert(!zst_find_entity(test_output_async->URI()));
    assert(!zst_find_entity(localPlug_uri));
    
    //Cleanup
    delete test_output_async;
    delete entity_sync;
	clear_callback_queue();
}


void test_hierarchy() {
    ZstLog::app(LogLevel::notification, "Running hierarchy test");

    //Test hierarchy
    ZstContainer * parent = new ZstContainer("parent");
    ZstComponent * child = new ZstComponent("child");
    parent->add_child(child);

    zst_activate_entity(parent);
    assert(zst_find_entity(parent->URI()));
    assert(zst_find_entity(child->URI()));
    
    //Test child removal from parent
    ZstLog::app(LogLevel::debug, "Testing child removal from parent");
    ZstURI child_URI = ZstURI(child->URI());
    zst_deactivate_entity(child);
    assert(!parent->walk_child_by_URI(child_URI));
    assert(!zst_find_entity(child_URI));
    
    //Test child activation and deactivation callbacks
    ZstLog::app(LogLevel::debug, "Test child activation callback");
    TestSynchronisableEvents * child_activation = new TestSynchronisableEvents();
    child->add_adaptor(child_activation);
    parent->add_child(child);
    
    zst_activate_entity(child);
    assert(child_activation->num_calls() == 1);
    child_activation->reset_num_calls();

    ZstLog::app(LogLevel::debug, "Test child deactivation callback");
    zst_deactivate_entity(child);
    assert(child_activation->num_calls() == 1);
    child_activation->reset_num_calls();
    child->remove_adaptor(child_activation);
    delete child_activation;
    
    //Test removing parent removes child
    parent->add_child(child);
    zst_activate_entity(child);
    
    ZstURI parent_URI = ZstURI(parent->URI());
    zst_deactivate_entity(parent);
    assert(!zst_find_entity(parent->URI()));
    assert(!zst_find_entity(child->URI()));

	//Test deleting entity removes it from the library
	zst_activate_entity(parent);
	delete parent;
	assert(!zst_find_entity(parent_URI));
	assert(!zst_get_root()->num_children() );

	parent = 0;
	child = 0;

	//Clear callback queue and make sure delete entities don't still have queued events
    clear_callback_queue();
}

int main(int argc,char **argv)
{
    TestRunner runner("TestEntities", argv[0]);
    test_create_entities();
    test_hierarchy();

    return 0;
}
