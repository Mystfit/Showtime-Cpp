#include "TestCommon.hpp"

using namespace ZstTest;


void test_external_entities(std::string external_test_path, bool launch_sink_process = true) {
    ZstLog::app(LogLevel::notification, "Starting external entities test");

    //Create callbacks
    TestEntityEvents * entityEvents = new TestEntityEvents();
    TestPerformerEvents * performerEvents = new TestPerformerEvents();
    
    zst_add_hierarchy_adaptor(entityEvents);
    zst_add_hierarchy_adaptor(performerEvents);

    //Create emitter
    OutputComponent * output_ent = new OutputComponent("proxy_test_output");
    zst_get_root()->add_child(output_ent);
    assert(output_ent->is_activated());
    
    ZstURI sink_perf_uri = ZstURI("sink");
    ZstURI sink_ent_uri = sink_perf_uri + ZstURI("sink_ent");
    ZstURI sink_B_uri = sink_ent_uri + ZstURI("sinkB");
    ZstURI sink_plug_uri = sink_ent_uri + ZstURI("in");
	ZstURI sync_out_plug_uri = sink_ent_uri + ZstURI("out");


    //Run the sink program
    std::string prog = system_complete(external_test_path).parent_path().generic_string() + "/TestHelperSink";
#ifdef WIN32
    prog += ".exe";
#endif
    boost::process::child sink_process;
#ifdef PAUSE_SINK
    char pause_flag = 'd';
#else
    char pause_flag = 'a';
#endif
	boost::process::ipstream sink_out;
    if (launch_sink_process) {
		//Run sink in external process so we don't share the same Showtime singleton
		ZstLog::app(LogLevel::notification, "Starting sink process");

        try {
            sink_process = boost::process::child(prog, &pause_flag, boost::process::std_out > sink_out); //d flag pauses the sink process to give us time to attach a debugger
#ifdef PAUSE_SINK
#ifdef WIN32
            system("pause");
#endif
            system("read -n 1 -s -p \"Press any key to continue...\n\"");
#endif
        }
        catch (boost::process::process_error e) {
            ZstLog::app(LogLevel::error, "Sink process failed to start. Code:{} Message:{}", e.code().value(), e.what());
        }
        assert(sink_process.valid());
	}

	auto sink_log_thread = ZstTest::log_external_pipe(sink_out);


	wait_for_event(performerEvents, 1);
    ZstPerformer * sink_performer = zst_get_performer_by_URI(sink_perf_uri);
    assert(sink_performer);
    performerEvents->reset_num_calls();

	ZstEntityBundle performer_bundle;
	zst_get_performers(performer_bundle);
	assert(performer_bundle.size() == 2);
    
    //Test entity exists
    wait_for_event(entityEvents, 2);
    ZstComponent * sink_ent = dynamic_cast<ZstComponent*>(zst_find_entity(sink_ent_uri));
    assert(sink_ent);
    entityEvents->reset_num_calls();
    
    ZstInputPlug * sink_plug = dynamic_cast<ZstInputPlug*>(sink_ent->get_child_by_URI(sink_plug_uri));
    assert(sink_plug);
	assert(zst_find_entity(sink_plug->URI()));
    assert(sink_plug->is_activated());

	ZstOutputPlug * sync_out_plug = dynamic_cast<ZstOutputPlug*>(sink_ent->get_child_by_URI(sync_out_plug_uri));
	assert(sync_out_plug);
	assert(sync_out_plug->is_activated());

	//Create plug sync adaptors
	TestPlugSync * plug_sync_adp = new TestPlugSync();
	sync_out_plug->add_adaptor(plug_sync_adp);
	zst_observe_entity(sync_out_plug);

    //Connect cable to sink
    ZstCable * cable = zst_connect_cable(sink_plug, output_ent->output());
    assert(cable);
    assert(cable->is_activated());
    
    //Send message to sink to test entity creation
    ZstLog::app(LogLevel::notification, "Asking sink to create an entity");
    output_ent->send(1);
	    
    //Test entity arriving
	ZstLog::app(LogLevel::notification, "Checking that the remote plug synced with our proxy plug");
    wait_for_event(entityEvents, 1);
    assert(zst_find_entity(sink_B_uri));
    entityEvents->reset_num_calls();

	//Check that the output plug published a sync message in response
	wait_for_event(plug_sync_adp, 1);
	assert(plug_sync_adp->num_calls() == 1);
	assert(sync_out_plug->int_at(0) == 1);
	plug_sync_adp->reset_num_calls();

	//Test adding plugs to a remote proxy
	ZstLog::app(LogLevel::notification, "Creating a plug on the remote performer");
	ZstComponent * sink_b = dynamic_cast<ZstComponent*>(zst_find_entity(sink_B_uri));
	ZstOutputPlug * remote_plug = sink_b->create_output_plug("remote_plug", ZstValueType::ZST_INT);
    assert(entityEvents->num_calls() == 1);
	assert(remote_plug);
	assert(remote_plug->is_activated());
	assert(zst_find_entity(remote_plug->URI()));
	ZstURI remote_plug_uri = remote_plug->URI();
    entityEvents->reset_num_calls();

	//Test remote plug can send values
	ZstLog::app(LogLevel::notification, "Testing sending values using remote plug");
	remote_plug->append_int(27);
	remote_plug->fire();

    //Send another value to remove the child
    //Test entity leaving
    ZstLog::app(LogLevel::notification, "Asking sink to remove an entity");
    output_ent->send(2);
    wait_for_event(entityEvents, 1);
    assert(!zst_find_entity(sink_B_uri));
	assert(!zst_find_entity(remote_plug_uri));
	assert(!remote_plug->is_activated());
	entityEvents->reset_num_calls();

    //Send message to sink
    ZstLog::app(LogLevel::notification, "Asking sink to throw an error");
    output_ent->send(3);
    //Not sure how to test for the error...

    ZstLog::app(LogLevel::debug, "Asking sink to leave");
	ZstURI out_plug_path = sync_out_plug->URI();
    output_ent->send(0);
    sink_process.wait();
    //int result = sink_process.exit_code();
    //assert(result == 0 || result == 259);	//Exit code 259 is 'No more data is available.' on Windows. Something to do with stdin?

    //Check that we received performer destruction request
	ZstLog::app(LogLevel::debug, "Checking if sink has left the graph");
    wait_for_event(performerEvents, 1);
    assert(!zst_get_performer_by_URI(sink_perf_uri));
    performerEvents->reset_num_calls();

	//Check that our local adaptor is now inactive
	zst_poll_once();
	ZstLog::app(LogLevel::debug, "Checking if plug sync adaptor is now inactive");
	assert(!zst_find_entity(out_plug_path));
	assert(!sink_performer->walk_child_by_URI(out_plug_path));
	assert(!plug_sync_adp->is_target_dispatcher_active());

    //Clean up output
    zst_deactivate_entity(output_ent);

    //Cleanup
    zst_remove_hierarchy_adaptor(entityEvents);
    zst_remove_hierarchy_adaptor(performerEvents);
    delete entityEvents;
    delete performerEvents;
    delete output_ent;
	sink_out.pipe().close();
	sink_log_thread.join();
    clear_callback_queue();
}


int main(int argc,char **argv)
{
	TestRunner runner("TestExternalClients", argv[0]);
	zst_start_file_logging("TestExternalClients.log");
    test_external_entities(argv[0], true);

    return 0;
}
