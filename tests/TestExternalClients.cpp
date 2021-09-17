#include "TestCommon.hpp"

#define BOOST_TEST_TOOLS_DEBUGGABLE
#define BOOST_TEST_MODULE External clients

using namespace ZstTest;

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
	client->get_performers(&bundle);
	for (auto p : bundle) {
		if (p->URI() == performer_address) {
			return true;
		}
	}
	return false;
}

BOOST_FIXTURE_TEST_CASE(multiple_clients, FixtureJoinServer) {
	auto perf_event = std::make_shared<TestPerformerEvents>();
	test_client->add_hierarchy_adaptor(perf_event);

	auto remote_client = std::make_unique<ShowtimeClient>();
	remote_client->init("remote_client", true);
	remote_client->auto_join_by_name(server_name.c_str());
	BOOST_REQUIRE(remote_client->is_connected());

	ZstEntityBundle performers;
	BOOST_TEST_CHECKPOINT("Waiting for remote client performer to arrive");
	wait_for_event(test_client, perf_event, 1);
	test_client->get_performers(&performers);
	BOOST_TEST(performers.size() == 2);
	performers.clear();
	perf_event->reset_num_calls();

	remote_client->destroy();
	BOOST_TEST_CHECKPOINT("Waiting for external client performer to leave");
	wait_for_event(test_client, perf_event, 1);
	BOOST_TEST(!remote_client->is_connected());
	test_client->get_performers(&performers);
	BOOST_TEST(performers.size() == 1);
}

BOOST_FIXTURE_TEST_CASE(multiple_client_heartbeats, FixtureJoinServer) {
	auto remote_client = std::make_unique<ShowtimeClient>();
	remote_client->init("remote_client", true);
	remote_client->auto_join_by_name(server_name.c_str());
	BOOST_REQUIRE(remote_client->is_connected());
	BOOST_REQUIRE(test_client->is_connected());
	WAIT_UNTIL_STAGE_TIMEOUT
	WAIT_UNTIL_STAGE_TIMEOUT
	BOOST_TEST(remote_client->is_connected());
	BOOST_TEST(test_client->is_connected());
}


BOOST_FIXTURE_TEST_CASE(performer_arriving, FixtureWaitForSinkClient) {
	BOOST_TEST(performerEvents->last_arrived_performer == external_performer_URI);
}

BOOST_FIXTURE_TEST_CASE(performer_leaving, FixtureWaitForSinkClient) {
	remote_client->leave();
	wait_for_event(test_client, performerEvents, 1);
	BOOST_TEST(performerEvents->last_left_performer == external_performer_URI);
}

BOOST_FIXTURE_TEST_CASE(find_performer, FixtureWaitForSinkClient) {
	BOOST_TEST(test_client->find_entity(external_performer_URI));
	BOOST_TEST(found_performer(test_client, external_performer_URI));
}

BOOST_FIXTURE_TEST_CASE(find_performer_entities, FixtureWaitForSinkClient) {
	TAKE_A_BREATH
	auto sink_ent = dynamic_cast<ZstComponent*>(test_client->find_entity(sink_ent_uri));
	BOOST_TEST_REQUIRE(sink_ent);
	BOOST_TEST(sink_ent->is_activated());

	auto sink_plug = dynamic_cast<ZstInputPlug*>(sink_ent->get_child_by_URI(sink_plug_uri));
	BOOST_TEST_REQUIRE(sink_plug);
	BOOST_TEST(test_client->find_entity(sink_plug->URI()));
	BOOST_TEST(sink_plug->is_activated());
    
    auto sync_out_plug = dynamic_cast<ZstOutputPlug*>(sink_ent->get_child_by_URI(sync_out_plug_uri));
	BOOST_TEST_REQUIRE(sync_out_plug);
    BOOST_TEST(sync_out_plug->is_activated());
}

BOOST_FIXTURE_TEST_CASE(connect_cable, FixtureExternalEntities) {
    auto sink_plug = dynamic_cast<ZstInputPlug*>(sink_ent->get_child_by_URI(sink_plug_uri));
	BOOST_TEST_REQUIRE(sink_plug);
    auto cable = test_client->connect_cable(sink_plug, output_ent->output());
	BOOST_TEST_REQUIRE(cable);
    BOOST_TEST(cable->is_activated());
}

BOOST_FIXTURE_TEST_CASE(plug_observation, FixtureExternalConnectCable) {
    auto sync_out_plug = dynamic_cast<ZstOutputPlug*>(sink_ent->get_child_by_URI(sync_out_plug_uri));
	BOOST_TEST_REQUIRE(sync_out_plug);
	auto plug_sync_adp = std::make_shared<TestPlugSync>();
    sync_out_plug->add_adaptor(plug_sync_adp);
    test_client->observe_entity(sync_out_plug);
    
	int echo_val = 4;
	sink->output->append_int(echo_val);
	sink->output->fire();
    wait_for_event(test_client, plug_sync_adp, 1);
    BOOST_TEST(sync_out_plug->int_at(0) == echo_val);
}

BOOST_FIXTURE_TEST_CASE(aquire_and_release_entity_ownership, FixtureExternalEntitysWithLocalInput) {
	BOOST_TEST_REQUIRE(sync_out_plug);
	BOOST_TEST(sync_out_plug->get_owner().is_empty());
    sync_out_plug->aquire_ownership();
    TAKE_A_BREATH
    BOOST_TEST(sync_out_plug->get_owner() == test_client->get_root()->URI());
	sync_out_plug->release_ownership();
	TAKE_A_BREATH
	BOOST_TEST(sync_out_plug->get_owner().is_empty());
}

BOOST_FIXTURE_TEST_CASE(ownership_grants_plug_fire_permission, FixtureExternalEntitysWithLocalInput) {
	BOOST_TEST_REQUIRE(sync_out_plug);
	BOOST_TEST(!sync_out_plug->can_fire());
    sync_out_plug->aquire_ownership();
    TAKE_A_BREATH
    BOOST_TEST(sync_out_plug->can_fire());
	sync_out_plug->release_ownership();
	TAKE_A_BREATH
	BOOST_TEST(!sync_out_plug->can_fire());
}

BOOST_FIXTURE_TEST_CASE(ownership_plug_fire_check, FixtureExternalEntitysWithLocalInput) {
	BOOST_TEST_REQUIRE(sync_out_plug);
	sync_out_plug->aquire_ownership();
	TAKE_A_BREATH
    int cmp_val = 27;
    sync_out_plug->append_int(cmp_val);
    sync_out_plug->fire();
	TAKE_A_BREATH
	test_client->poll_once();
	BOOST_TEST(input_component->input()->size() > 0);
    BOOST_TEST(input_component->input()->int_at(0) == cmp_val);
}

BOOST_FIXTURE_TEST_CASE(entity_arriving, FixtureWaitForSinkClient) {
    auto entityEvents = std::make_shared<TestEntityEvents>();
    test_client->add_hierarchy_adaptor(entityEvents);
    
	auto remote_ent = std::make_unique<InputComponent>("remote_input");
	BOOST_TEST_REQUIRE(remote_client->get_root()->is_activated());
	remote_client->get_root()->add_child(remote_ent.get());

	BOOST_TEST_CHECKPOINT("Waiting for remote entity to arrive");
    wait_for_event(test_client, entityEvents, 1);
    BOOST_TEST_REQUIRE(test_client->find_entity(remote_ent->URI()));
}

BOOST_FIXTURE_TEST_CASE(entity_leaving, FixtureExternalConnectCable) {
    auto entityEvents = std::make_shared<TestEntityEvents>();
    test_client->add_hierarchy_adaptor(entityEvents);
	
	auto remote_ent = std::make_unique<InputComponent>("remote_input");
	remote_client->get_root()->add_child(remote_ent.get());

	BOOST_TEST_CHECKPOINT("Waiting for remote entity to leave");
    wait_for_event(test_client, entityEvents, 1);
    entityEvents->reset_num_calls();
    
	TAKE_A_BREATH
	remote_client->deactivate_entity(remote_ent.get());
    wait_for_event(test_client, entityEvents, 1);
	BOOST_TEST_REQUIRE(!test_client->find_entity(remote_ent->URI()));
}

BOOST_FIXTURE_TEST_CASE(external_exception, FixtureExternalConnectCable) {
    output_ent->send(3);
	TAKE_A_BREATH 
	remote_client->poll_once();
    //TODO: How do we test for this error?
}

BOOST_FIXTURE_TEST_CASE(set_name_updates_proxy, FixtureWaitForSinkClient) {
	auto entityEvents = std::make_shared<TestEntityEvents>();
	test_client->add_hierarchy_adaptor(entityEvents);
	entityEvents->reset_num_calls();
	auto remote_ent = std::make_unique<ZstComponent>("remote");
	remote_client->get_root()->add_child(remote_ent.get());

	BOOST_TEST_CHECKPOINT("Waiting for remote entity to arrive");
	wait_for_event(test_client, entityEvents, 1);
	auto proxy = test_client->find_entity(remote_ent->URI());
	entityEvents->reset_num_calls();
	BOOST_REQUIRE(proxy);

	BOOST_TEST_CHECKPOINT("Renaming entity");
	remote_ent->set_name("renamed");
	wait_for_event(test_client, entityEvents, 1);
	BOOST_TEST(remote_ent->URI() == proxy->URI());
	BOOST_TEST(entityEvents->last_entity_updated == remote_ent->URI());
	BOOST_TEST(test_client->find_entity(remote_ent->URI()));

	/*auto hierarchy_events = std::make_shared<TestEntityEvents>();
	test_client->add_hierarchy_adaptor(hierarchy_events);
	hierarchy_events->reset_num_calls();

	auto entity = std::make_unique<ZstComponent>("named");
	remote_client->get_root()->add_child(entity.get());
	wait_for_event(test_client, hierarchy_events, 1);
	auto proxy_entity = test_client->find_entity(entity->URI());
	hierarchy_events->reset_num_calls();

	entity->set_name("renamed");
	wait_for_event(test_client, hierarchy_events, 1);
	BOOST_TEST(entity->URI() == proxy_entity->URI());
	BOOST_TEST(hierarchy_events->last_entity_updated == entity->URI());
	BOOST_TEST(test_client->find_entity(entity->URI()));*/
}

BOOST_FIXTURE_TEST_CASE(external_cable_routes, FixtureWaitForSinkClient) {
	auto a_ent = std::make_unique<ZstComponent>("a");
	auto b_ent = std::make_unique<ZstComponent>("b");
	auto c_ent = std::make_unique<OutputComponent>("c");
	test_client->get_root()->add_child(a_ent.get());
	a_ent->add_child(b_ent.get());
	b_ent->add_child(c_ent.get());

	auto d_ent = std::make_unique<ZstComponent>("d");
	auto e_ent = std::make_unique<ZstComponent>("e");
	auto f_ent = std::make_unique<InputComponent>("f");
	remote_client->get_root()->add_child(d_ent.get());
	d_ent->add_child(e_ent.get());
	e_ent->add_child(f_ent.get());

	auto cable = c_ent->output()->connect_cable(f_ent->input());

	ZstEntityBundle bundle;
	cable->get_cable_route(bundle);

	BOOST_REQUIRE(bundle.size() == 10); // count includes both performers and plugs
	BOOST_TEST(bundle.item_at(0) == c_ent->output());
	BOOST_TEST(bundle.item_at(1) == c_ent.get());
	BOOST_TEST(bundle.item_at(2) == b_ent.get());
	BOOST_TEST(bundle.item_at(3) == a_ent.get());
	BOOST_TEST(bundle.item_at(4) == test_client->get_root());

	// URI comparisons between proxy remote entities and local versions
	BOOST_TEST(bundle.item_at(5)->URI() == remote_client->get_root()->URI());
	BOOST_TEST(bundle.item_at(6)->URI() == d_ent->URI());
	BOOST_TEST(bundle.item_at(7)->URI() == e_ent->URI());
	BOOST_TEST(bundle.item_at(8)->URI() == f_ent->URI());
	BOOST_TEST(bundle.item_at(9)->URI() == f_ent->input()->URI());
}