#define BOOST_TEST_MODULE External clients

#include "TestCommon.hpp"

using namespace ZstTest;

struct FixtureExternalClient : public FixtureJoinServer {
	boost::process::child sink_process;
	boost::process::ipstream sink_out;
	boost::thread sink_log_thread;
    
    //Common URIs
    ZstURI sink_perf_uri = ZstURI("sink");
    ZstURI sink_ent_uri = sink_perf_uri + ZstURI("sink_ent");
    ZstURI sink_plug_uri = sink_ent_uri + ZstURI("in");
    ZstURI sync_out_plug_uri = sink_ent_uri + ZstURI("out");
    
    std::unique_ptr<OutputComponent> output_ent;

    FixtureExternalClient() :
        output_ent(std::make_unique<OutputComponent>("output"))
    {
		std::string prog = fs::current_path().generic_string() + "/TestHelperSink";
#ifdef WIN32
		prog += ".exe";
#endif

#ifdef PAUSE_SINK
		char pause_flag = 'd';
#else
		char pause_flag = 'a';
#endif
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
			TAKE_A_BREATH
		}
		catch (boost::process::process_error e) {
			ZstLog::app(LogLevel::error, "Sink process failed to start. Code:{} Message:{}", e.code().value(), e.what());
		}

		// Create a thread to handle reading log info from the sink process' stdout pipe
		sink_log_thread = ZstTest::log_external_pipe(sink_out);
	}

	~FixtureExternalClient() {
		sink_out.pipe().close();
		sink_log_thread.join();
	}
};


struct FixtureExternalEntities : public FixtureExternalClient
{
    std::shared_ptr<TestPerformerEvents> performerEvents;
    
    ZstComponent * sink_ent;
    ZstInputPlug * sink_plug;

    FixtureExternalEntities() :
        performerEvents(std::make_shared<TestPerformerEvents>()),
        sink_ent(NULL),
        sink_plug(NULL)
    {
        zst_add_hierarchy_adaptor(performerEvents.get());
        wait_for_event(performerEvents.get(), 1);
        zst_get_root()->add_child(output_ent.get());
        clear_callback_queue();
        performerEvents->reset_num_calls();
        
        sink_ent = dynamic_cast<ZstComponent*>(zst_find_entity(sink_ent_uri));
        sink_plug = dynamic_cast<ZstInputPlug*>(sink_ent->get_child_by_URI(sink_plug_uri));
    }
    
    ~FixtureExternalEntities(){}
};


struct FixtureExternalConnectCable : public FixtureExternalEntities
{
    FixtureExternalConnectCable(){
        zst_connect_cable(sink_plug, output_ent->output());
    }
};


bool found_performer(ZstURI performer_address) {
	ZstEntityBundle bundle;
	zst_get_performers(bundle);
	for (auto p : bundle) {
		if (p->URI() == performer_address) {
			return true;
		}
	}
	return false;
}


BOOST_FIXTURE_TEST_CASE(find_performer, FixtureExternalClient) {
	BOOST_TEST(zst_get_performer_by_URI(sink_perf_uri));
	BOOST_TEST(found_performer(sink_perf_uri));
}

BOOST_FIXTURE_TEST_CASE(find_performer_entities, FixtureExternalClient) {
	auto sink_ent = dynamic_cast<ZstComponent*>(zst_find_entity(sink_ent_uri));
	BOOST_TEST(sink_ent);
	BOOST_TEST(sink_ent->is_activated());

	auto sink_plug = dynamic_cast<ZstInputPlug*>(sink_ent->get_child_by_URI(sink_plug_uri));
	BOOST_TEST(sink_plug);
	BOOST_TEST(zst_find_entity(sink_plug->URI()));
	BOOST_TEST(sink_plug->is_activated());
    
    auto sync_out_plug = dynamic_cast<ZstOutputPlug*>(sink_ent->get_child_by_URI(sync_out_plug_uri));
    BOOST_TEST(sync_out_plug);
    BOOST_TEST(sync_out_plug->is_activated());
}

BOOST_FIXTURE_TEST_CASE(connect_cable, FixtureExternalEntities) {
    auto sink_plug = dynamic_cast<ZstInputPlug*>(sink_ent->get_child_by_URI(sink_plug_uri));
    auto cable = zst_connect_cable(sink_plug, output_ent->output());
    BOOST_TEST(cable);
    BOOST_TEST(cable->is_activated());
}

BOOST_FIXTURE_TEST_CASE(plug_observation, FixtureExternalConnectCable) {
    auto sync_out_plug = dynamic_cast<ZstOutputPlug*>(sink_ent->get_child_by_URI(sync_out_plug_uri));
    auto plug_sync_adp = std::make_shared<TestPlugSync>();
    sync_out_plug->add_adaptor(plug_sync_adp.get());
    zst_observe_entity(sync_out_plug);
    
    int echo_val = 4;
    output_ent->send(echo_val);
    wait_for_event(plug_sync_adp.get(), 1);
    BOOST_TEST(zst_find_entity(sink_ent_uri + ZstURI("sinkB")));
    BOOST_TEST(sync_out_plug->int_at(0) == echo_val);
}

BOOST_FIXTURE_TEST_CASE(entity_arriving, FixtureExternalConnectCable) {
    auto entityEvents = std::make_shared<TestEntityEvents>();
    zst_add_hierarchy_adaptor(entityEvents.get());
    output_ent->send(1);
    wait_for_event(entityEvents.get(), 1);
    BOOST_TEST(zst_find_entity(sink_ent_uri + ZstURI("sinkB")));
}

BOOST_FIXTURE_TEST_CASE(entity_leaving, FixtureExternalConnectCable) {
    auto entityEvents = std::make_shared<TestEntityEvents>();
    zst_add_hierarchy_adaptor(entityEvents.get());
    output_ent->send(1);
    wait_for_event(entityEvents.get(), 1);
    entityEvents->reset_num_calls();
    
    output_ent->send(2);
    wait_for_event(entityEvents.get(), 1);
    BOOST_TEST(!zst_find_entity(sink_ent_uri + ZstURI("sinkB")));
}

BOOST_FIXTURE_TEST_CASE(external_exception, FixtureExternalConnectCable) {
    output_ent->send(3);
    //TODO: How do we test for this error?
}

BOOST_FIXTURE_TEST_CASE(performer_arriving, FixtureExternalClient) {
    auto performerEvents = std::make_shared<TestPerformerEvents>();
    zst_add_hierarchy_adaptor(performerEvents.get());
    wait_for_event(performerEvents.get(), 1);
    BOOST_TEST(performerEvents->last_arrived_performer == sink_perf_uri);
}


BOOST_FIXTURE_TEST_CASE(performer_leaving, FixtureExternalClient) {
    auto performerEvents = std::make_shared<TestPerformerEvents>();
    zst_add_hierarchy_adaptor(performerEvents.get());
    
    output_ent->send(-1);
    sink_process.wait();
    int result = sink_process.exit_code();
    BOOST_TEST((result == 0 || result == 259));    //Exit code 259 is 'No more data is available.' on Windows. Something to do with stdin?
    wait_for_event(performerEvents.get(), 1);
    BOOST_TEST(performerEvents->last_left_performer == sink_perf_uri);
}
