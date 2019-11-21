#define BOOST_TEST_TOOLS_DEBUGGABLE
#define BOOST_TEST_MODULE External clients

#include "TestCommon.hpp"

using namespace ZstTest;

struct FixtureSinkClient : public FixtureExternalClient {
    //Common URIs
	ZstURI sink_ent_uri;
	ZstURI sink_plug_uri;
	ZstURI sync_out_plug_uri;

	FixtureSinkClient() : 
		FixtureExternalClient("TestHelperSink"),
		sink_ent_uri(external_performer_URI + ZstURI("sink_ent")),
		sink_plug_uri(sink_ent_uri + ZstURI("in")),
		sync_out_plug_uri(sink_ent_uri + ZstURI("out"))
    {
	}

	~FixtureSinkClient() {
	}
};


struct FixtureWaitForSinkClient : public FixtureJoinServer, FixtureSinkClient {
	std::shared_ptr<TestPerformerEvents> performerEvents;

	FixtureWaitForSinkClient() : performerEvents(std::make_shared<TestPerformerEvents>())
	{
		test_client->add_hierarchy_adaptor(performerEvents);
		BOOST_TEST_CHECKPOINT("Waiting for external client performer to arrive");
		wait_for_event(test_client, performerEvents, 1);
		performerEvents->reset_num_calls();
	}
	~FixtureWaitForSinkClient() {}
};


struct FixtureExternalEntities : public FixtureWaitForSinkClient
{
    ZstComponent * sink_ent;
    ZstInputPlug * sink_plug;
	std::unique_ptr<OutputComponent> output_ent;

    FixtureExternalEntities() :
        sink_ent(NULL),
        sink_plug(NULL),
		output_ent(std::make_unique<OutputComponent>("output"))
    {
        test_client->get_root()->add_child(output_ent.get());
		test_client->poll_once();
        
        sink_ent = dynamic_cast<ZstComponent*>(test_client->find_entity(sink_ent_uri));
        sink_plug = dynamic_cast<ZstInputPlug*>(sink_ent->get_child_by_URI(sink_plug_uri));
    }
    
    ~FixtureExternalEntities(){}
};


struct FixtureExternalEntitysWithLocalInput : public FixtureExternalEntities{
    std::unique_ptr<InputComponent> input_component;
    ZstOutputPlug * sync_out_plug;
    
    FixtureExternalEntitysWithLocalInput() :
        input_component(std::make_unique<InputComponent>("input_cmp")),
        sync_out_plug(NULL)
    {
        test_client->get_root()->add_child(input_component.get());
        sync_out_plug = dynamic_cast<ZstOutputPlug*>(sink_ent->get_child_by_URI(sync_out_plug_uri));
        test_client->connect_cable(input_component->input(), sync_out_plug);
		test_client->poll_once();
    }
    ~FixtureExternalEntitysWithLocalInput(){}
};


struct FixtureExternalConnectCable : public FixtureExternalEntities
{
    FixtureExternalConnectCable(){
        test_client->connect_cable(sink_plug, output_ent->output());
		test_client->poll_once();
    }
};


bool found_performer(std::shared_ptr<ShowtimeClient> client, ZstURI performer_address) {
	ZstEntityBundle bundle;
	client->get_performers(bundle);
	for (auto p : bundle) {
		if (p->URI() == performer_address) {
			return true;
		}
	}
	return false;
}

BOOST_FIXTURE_TEST_CASE(performer_arriving, FixtureWaitForSinkClient) {
	BOOST_TEST(performerEvents->last_arrived_performer == external_performer_URI);
}

BOOST_FIXTURE_TEST_CASE(performer_leaving, FixtureExternalConnectCable) {
	output_ent->send(-1);
	wait_for_event(test_client, performerEvents, 1);
	BOOST_TEST(performerEvents->last_left_performer == external_performer_URI);
}

BOOST_FIXTURE_TEST_CASE(find_performer, FixtureWaitForSinkClient) {
	BOOST_TEST(test_client->get_performer_by_URI(external_performer_URI));
	BOOST_TEST(found_performer(test_client, external_performer_URI));
}

BOOST_FIXTURE_TEST_CASE(find_performer_entities, FixtureWaitForSinkClient) {
	auto sink_ent = dynamic_cast<ZstComponent*>(test_client->find_entity(sink_ent_uri));
	BOOST_TEST(sink_ent);
	BOOST_TEST(sink_ent->is_activated());

	auto sink_plug = dynamic_cast<ZstInputPlug*>(sink_ent->get_child_by_URI(sink_plug_uri));
	BOOST_TEST(sink_plug);
	BOOST_TEST(test_client->find_entity(sink_plug->URI()));
	BOOST_TEST(sink_plug->is_activated());
    
    auto sync_out_plug = dynamic_cast<ZstOutputPlug*>(sink_ent->get_child_by_URI(sync_out_plug_uri));
    BOOST_TEST(sync_out_plug);
    BOOST_TEST(sync_out_plug->is_activated());
}

BOOST_FIXTURE_TEST_CASE(connect_cable, FixtureExternalEntities) {
    auto sink_plug = dynamic_cast<ZstInputPlug*>(sink_ent->get_child_by_URI(sink_plug_uri));
    auto cable = test_client->connect_cable(sink_plug, output_ent->output());
    BOOST_TEST(cable);
    BOOST_TEST(cable->is_activated());
}

BOOST_FIXTURE_TEST_CASE(plug_observation, FixtureExternalConnectCable) {
    auto sync_out_plug = dynamic_cast<ZstOutputPlug*>(sink_ent->get_child_by_URI(sync_out_plug_uri));
    auto plug_sync_adp = std::make_shared<TestPlugSync>();
    sync_out_plug->add_adaptor(plug_sync_adp);
    test_client->observe_entity(sync_out_plug);
    
    int echo_val = 4;
    output_ent->send(echo_val);
    wait_for_event(test_client, plug_sync_adp, 1);
    BOOST_TEST(sync_out_plug->int_at(0) == echo_val);
}

BOOST_FIXTURE_TEST_CASE(aquire_and_release_entity_ownership, FixtureExternalEntitysWithLocalInput) {
    BOOST_TEST(sync_out_plug->get_owner().is_empty());
    sync_out_plug->aquire_ownership();
    TAKE_A_BREATH
    BOOST_TEST(sync_out_plug->get_owner() == test_client->get_root()->URI());
	sync_out_plug->release_ownership();
	TAKE_A_BREATH
	BOOST_TEST(sync_out_plug->get_owner().is_empty());
}

BOOST_FIXTURE_TEST_CASE(ownership_grants_plug_fire_permission, FixtureExternalEntitysWithLocalInput) {
    BOOST_TEST(!sync_out_plug->can_fire());
    sync_out_plug->aquire_ownership();
    TAKE_A_BREATH
    BOOST_TEST(sync_out_plug->can_fire());
	sync_out_plug->release_ownership();
	TAKE_A_BREATH
	BOOST_TEST(!sync_out_plug->can_fire());
}

BOOST_FIXTURE_TEST_CASE(ownership_plug_fire_check, FixtureExternalEntitysWithLocalInput) {
    sync_out_plug->aquire_ownership();
	TAKE_A_BREATH
    int cmp_val = 27;
    sync_out_plug->append_int(cmp_val);
    sync_out_plug->fire();
	TAKE_A_BREATH
	BOOST_TEST(input_component->input()->size() > 0);
    BOOST_TEST(input_component->input()->int_at(0) == cmp_val);
}

BOOST_FIXTURE_TEST_CASE(entity_arriving, FixtureExternalConnectCable) {
    auto entityEvents = std::make_shared<TestEntityEvents>();
    test_client->add_hierarchy_adaptor(entityEvents);
    output_ent->send(1);
    wait_for_event(test_client, entityEvents, 1);
    BOOST_TEST(test_client->find_entity(sink_ent_uri + ZstURI("sinkB")));
}

BOOST_FIXTURE_TEST_CASE(entity_leaving, FixtureExternalConnectCable) {
    auto entityEvents = std::make_shared<TestEntityEvents>();
    test_client->add_hierarchy_adaptor(entityEvents);
    output_ent->send(1);
    wait_for_event(test_client, entityEvents, 1);
    entityEvents->reset_num_calls();
    
    output_ent->send(2);
    wait_for_event(test_client, entityEvents, 1);
    BOOST_TEST(!test_client->find_entity(sink_ent_uri + ZstURI("sinkB")));
}

BOOST_FIXTURE_TEST_CASE(external_exception, FixtureExternalConnectCable) {
    output_ent->send(3);
    //TODO: How do we test for this error?
}
