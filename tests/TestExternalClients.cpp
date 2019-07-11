#define BOOST_TEST_MODULE External clients

#include "TestCommon.hpp"

using namespace ZstTest;

struct FixtureSinkClient : public FixtureExternalClient {
	//Output entity
	std::unique_ptr<OutputComponent> output_ent;

    //Common URIs
	ZstURI sink_ent_uri;
	ZstURI sink_plug_uri;
	ZstURI sync_out_plug_uri;
    
	FixtureSinkClient() :
		FixtureExternalClient("TestHelperSink"),
		output_ent(std::make_unique<OutputComponent>("output")),
		sink_ent_uri(external_performer_URI + ZstURI("sink_ent")),
		sink_plug_uri(sink_ent_uri + ZstURI("in")),
		sync_out_plug_uri(sink_ent_uri + ZstURI("out"))
    {
	}

	~FixtureSinkClient() {
	}
};


struct FixtureWaitForClient : public FixtureSinkClient {
	std::shared_ptr<TestPerformerEvents> performerEvents;

	FixtureWaitForClient() : performerEvents(std::make_shared<TestPerformerEvents>()) {
		zst_add_hierarchy_adaptor(performerEvents.get());
		wait_for_event(performerEvents.get(), 1);
		performerEvents->reset_num_calls();
	}

	~FixtureWaitForClient() {}
};


struct FixtureExternalEntities : public FixtureWaitForClient
{
    ZstComponent * sink_ent;
    ZstInputPlug * sink_plug;

    FixtureExternalEntities() :
        sink_ent(NULL),
        sink_plug(NULL)
    {
        zst_get_root()->add_child(output_ent.get());
        clear_callback_queue();
        
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


BOOST_FIXTURE_TEST_CASE(performer_arriving, FixtureWaitForClient) {
	BOOST_TEST(performerEvents->last_arrived_performer == external_performer_URI);
}

BOOST_FIXTURE_TEST_CASE(performer_leaving, FixtureExternalConnectCable) {
	output_ent->send(-1);
	wait_for_event(performerEvents.get(), 1);
	BOOST_TEST(performerEvents->last_left_performer == external_performer_URI);
}

BOOST_FIXTURE_TEST_CASE(find_performer, FixtureWaitForClient) {
	BOOST_TEST(zst_get_performer_by_URI(external_performer_URI));
	BOOST_TEST(found_performer(external_performer_URI));
}

BOOST_FIXTURE_TEST_CASE(find_performer_entities, FixtureWaitForClient) {
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
