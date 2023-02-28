#include "TestCommon.hpp"
#include <float.h>
#include <array>

#define BOOST_TEST_MODULE Graph

using namespace ZstTest;

class LimitedConnectionInputComponent : public ZstComponent {
public:
	std::unique_ptr<ZstInputPlug> input;

	LimitedConnectionInputComponent(std::string name, int max_cables) : 
		ZstComponent("LimitedConnectionInputComponent", name.c_str()),
		input(std::make_unique<ZstInputPlug>("limited_input", ZstValueType::IntList, max_cables))
	{
	}

	virtual void on_registered() override {
		add_child(input.get());
	}
};


struct FixtureCableCompare {
	ZstURI in = ZstURI("a/1");
	ZstURI out = ZstURI("b/1");
	ZstURI less = ZstURI("a/b");
	ZstURI more = ZstURI("b/a");
	std::unique_ptr<ZstInputPlug> plug_in_A = std::make_unique<ZstInputPlug>(in.path(), ZstValueType::IntList);
	std::unique_ptr<ZstOutputPlug> plug_out_A = std::make_unique<ZstOutputPlug>(out.path(), ZstValueType::IntList);
	std::unique_ptr<ZstInputPlug> plug_in_B = std::make_unique<ZstInputPlug>(less.path(), ZstValueType::IntList);
	std::unique_ptr<ZstOutputPlug> plug_out_B = std::make_unique<ZstOutputPlug>(more.path(), ZstValueType::IntList);
	std::unique_ptr<ZstInputPlug> plug_in_C = std::make_unique<ZstInputPlug>(more.path(), ZstValueType::IntList);
	std::unique_ptr<ZstOutputPlug> plug_out_C = std::make_unique<ZstOutputPlug>(less.path(), ZstValueType::IntList);
	std::unique_ptr<ZstCable> cable_a = std::make_unique<ZstCable>(plug_in_A.get(), plug_out_A.get());
	std::unique_ptr<ZstCable> cable_b = std::make_unique<ZstCable>(plug_in_A.get(), plug_out_A.get());
	std::unique_ptr<ZstCable> cable_c = std::make_unique<ZstCable>(plug_in_B.get(), plug_out_B.get());
	std::unique_ptr<ZstCable> cable_d = std::make_unique<ZstCable>(plug_in_C.get(), plug_out_C.get());
};


struct FixturePlugs : public FixtureJoinServer {
	std::unique_ptr<OutputComponent> output_component;
	std::unique_ptr<InputComponent> input_component;

	FixturePlugs() :
		output_component(std::make_unique<OutputComponent>("connect_test_out")),
		input_component(std::make_unique<InputComponent>("connect_test_in"))
	{
		test_client->get_root()->add_child(output_component.get());
		test_client->get_root()->add_child(input_component.get());

		ZstEntityBundle bundle;
		input_component->get_plugs(&bundle);
	}

	~FixturePlugs() {};
};


struct FixtureCable : public FixturePlugs {
	ZstCable* cable;
	std::shared_ptr<TestSynchronisableEvents> cable_activation_events;

	FixtureCable() : cable_activation_events(std::make_shared<TestSynchronisableEvents>())
	{
		cable = test_client->connect_cable(input_component->input(), output_component->output());
		cable->add_adaptor(cable_activation_events);
		cable_activation_events->reset_num_calls();
	}

	~FixtureCable() {
	};
};


struct FixtureBranchingComponents : public FixtureJoinServer
{
	// Branching structure (data flow downwards):
	// 
	//      A
	//      |
	//		B
	//	   / \
	//	  C   D	

	std::unique_ptr<ZstComponent> top_comp;
	std::unique_ptr<ZstComponent> a_comp;
	std::unique_ptr<ZstOutputPlug> a_out;

	std::unique_ptr<ZstComponent> b_comp;
	std::unique_ptr<ZstInputPlug> b_in;
	std::unique_ptr<ZstOutputPlug> b_out_first;
	std::unique_ptr<ZstOutputPlug> b_out_second;

	std::unique_ptr<ZstComponent> c_comp;
	std::unique_ptr<ZstInputPlug> c_in;
	std::unique_ptr<ZstOutputPlug> c_out;

	std::unique_ptr<ZstComponent> d_comp;
	std::unique_ptr<ZstInputPlug> d_in_first;
	std::unique_ptr<ZstInputPlug> d_in_second;


	FixtureBranchingComponents() :
		top_comp(std::make_unique<ZstComponent>("computer_node")),
		a_comp(std::make_unique<ZstComponent>("a")),
        a_out(std::make_unique<ZstOutputPlug>("out", ZstValueType::IntList)),
		b_comp(std::make_unique<ZstComponent>("b")),
        b_in(std::make_unique<ZstInputPlug>("in", ZstValueType::IntList)),
        b_out_first(std::make_unique<ZstOutputPlug>("out1", ZstValueType::IntList)),
        b_out_second(std::make_unique<ZstOutputPlug>("out2", ZstValueType::IntList)),
		c_comp(std::make_unique<ZstComponent>("c")),
        c_in(std::make_unique<ZstInputPlug>("in", ZstValueType::IntList)),
        c_out(std::make_unique<ZstOutputPlug>("out", ZstValueType::IntList)),
		d_comp(std::make_unique<ZstComponent>("d")),
		d_in_first(std::make_unique<ZstInputPlug>("in1", ZstValueType::IntList)),
		d_in_second(std::make_unique<ZstInputPlug>("in2", ZstValueType::IntList))
	{
		test_client->get_root()->add_child(top_comp.get());

		// Out-of-order adding of children to make sure topological sort works
		top_comp->add_child(d_comp.get());
		top_comp->add_child(c_comp.get());
		top_comp->add_child(a_comp.get());
		top_comp->add_child(b_comp.get());

		a_comp->add_child(a_out.get());
		b_comp->add_child(b_out_first.get());
		b_comp->add_child(b_out_second.get());
		b_comp->add_child(b_in.get());

		c_comp->add_child(c_in.get());
		c_comp->add_child(c_out.get());
		d_comp->add_child(d_in_first.get());
		d_comp->add_child(d_in_second.get());

		a_out->connect_cable(b_in.get());
		b_out_first->connect_cable(c_in.get());
		b_out_second->connect_cable(d_in_first.get());
		c_out->connect_cable(d_in_second.get());
	}

	~FixtureBranchingComponents() {};
};


bool found_cable(std::shared_ptr<ShowtimeClient> client, ZstCableAddress cable_address) {
	ZstCableBundle bundle;
	client->get_root()->get_child_cables(&bundle);
	for (auto c : bundle) {
		if (c->get_address() == cable_address) {
			return true;
		}
	}
	return false;
}


BOOST_FIXTURE_TEST_CASE(cable_URIs, FixtureCableCompare){
	BOOST_TEST(cable_a->get_address().get_input_URI() == in);
	BOOST_TEST(cable_a->get_address().get_output_URI() == out);
	BOOST_TEST(cable_a->is_attached(out));
	BOOST_TEST(cable_a->is_attached(in));
}

BOOST_FIXTURE_TEST_CASE(cable_comparisons, FixtureCableCompare){
	//Test cable comparisons
	BOOST_TEST(ZstCableAddressEq{}(cable_a->get_address(), cable_b->get_address()));
	BOOST_TEST(ZstCableAddressHash{}(cable_a->get_address()) == ZstCableAddressHash{}(cable_b->get_address()));
	BOOST_TEST(ZstCableCompare{}(cable_c, cable_d));
	BOOST_TEST(ZstCableCompare{}(cable_c->get_address(), cable_d));
	BOOST_TEST(ZstCableCompare{}(cable_c, cable_d->get_address()));
	BOOST_TEST(!(ZstCableCompare{}(cable_d->get_address(), cable_c)));
	BOOST_TEST(!(ZstCableCompare{}(cable_d, cable_c->get_address())));
}

BOOST_FIXTURE_TEST_CASE(cable_sets, FixtureCableCompare){
	//Test cable sets
	std::set<std::unique_ptr<ZstCable>, ZstCableCompare> cable_set;
	cable_set.insert(std::move(cable_c));
	cable_set.insert(std::move(cable_d));
	BOOST_TEST((cable_set.find(cable_c) != cable_set.end()));
	BOOST_TEST((cable_set.find(cable_d) != cable_set.end()));
}

BOOST_FIXTURE_TEST_CASE(output_can_fire, FixturePlugs) {
	BOOST_TEST(output_component->output()->can_fire());
}

BOOST_FIXTURE_TEST_CASE(sync_connect_cable, FixturePlugs) {
	auto cable = test_client->connect_cable(input_component->input(), output_component->output());
	BOOST_REQUIRE(cable);
	BOOST_TEST(cable->is_activated());
	BOOST_TEST(cable->get_output());
	BOOST_TEST(cable->get_input());
	BOOST_TEST(cable->get_output() == output_component->output());
	BOOST_TEST(cable->get_input() == input_component->input());
	BOOST_TEST(output_component->output()->is_connected_to(input_component->input()));
	BOOST_TEST(input_component->input()->is_connected_to(output_component->output()));
}

BOOST_FIXTURE_TEST_CASE(input_plug_connect_cables, FixturePlugs) {
	auto cable = input_component->input()->connect_cable(output_component->output());
	BOOST_REQUIRE(cable);
	BOOST_TEST(cable->is_activated());
}

BOOST_FIXTURE_TEST_CASE(output_plug_connect_cables, FixturePlugs) {
	auto cable = output_component->output()->connect_cable(input_component->input());
	BOOST_REQUIRE(cable);
	BOOST_TEST(cable->is_activated());
}

BOOST_FIXTURE_TEST_CASE(input_plug_connect_cables_async, FixturePlugs) {
	auto cable = input_component->input()->connect_cable_async(output_component->output());
	bool cable_activated = false;
	BOOST_REQUIRE(cable);
	cable->synchronisable_events()->synchronisable_activated()->add([&cable_activated](ZstSynchronisable* synchronisable) { cable_activated = synchronisable->is_activated(); });
	wait_for_condition(test_client, cable_activated);
	BOOST_TEST(cable_activated);
}

BOOST_FIXTURE_TEST_CASE(output_plug_connect_cables_async, FixturePlugs) {
	auto cable = output_component->output()->connect_cable_async(input_component->input());
	bool cable_activated = false;
	BOOST_REQUIRE(cable);
	cable->synchronisable_events()->synchronisable_activated()->add([&cable_activated](ZstSynchronisable* synchronisable) { cable_activated = synchronisable->is_activated(); });
	wait_for_condition(test_client, cable_activated);
	BOOST_TEST(cable_activated);
}

BOOST_FIXTURE_TEST_CASE(local_cable_events, FixturePlugs) {
	auto cable_events = std::make_shared<TestSessionEvents>();
	test_client->add_session_adaptor(cable_events);

	auto cable = test_client->connect_cable(input_component->input(), output_component->output());
	BOOST_REQUIRE(cable);
	auto cable_address = cable->get_address();

	BOOST_TEST((cable_events->last_cable_arrived == cable_address));
	test_client->destroy_cable(cable);
	BOOST_TEST((cable_events->last_cable_left == cable_address));
}

BOOST_FIXTURE_TEST_CASE(async_connect_cable_callback, FixturePlugs) {
	auto cable_activation_events = std::make_shared<TestSynchronisableEvents>();
	auto cable = test_client->connect_cable_async(input_component->input(), output_component->output());
	cable->add_adaptor(cable_activation_events);
	
	wait_for_event(test_client, cable_activation_events, 1);
	BOOST_TEST(cable->is_activated());
}

BOOST_FIXTURE_TEST_CASE(async_destroy_cable_callback, FixtureCable) {
	auto address_cmp = cable->get_address();
	test_client->destroy_cable_async(cable);
	wait_for_event(test_client, cable_activation_events, 1);
	BOOST_TEST(!found_cable(test_client, address_cmp));
}

BOOST_FIXTURE_TEST_CASE(get_cable_from_plug, FixtureCable) {
	ZstCableBundle bundle;
	output_component->output()->get_child_cables(&bundle);
	for (auto c : bundle) {
		BOOST_TEST(c->get_address().get_input_URI() == input_component->input()->URI());
	}
}

BOOST_FIXTURE_TEST_CASE(disconnect_cable, FixtureCable) {
	test_client->destroy_cable(cable);
	BOOST_TEST(!output_component->output()->is_connected_to(input_component->input()));
	BOOST_TEST(!found_cable(test_client, ZstCableAddress(input_component->input()->URI(), output_component->output()->URI())));
}

BOOST_FIXTURE_TEST_CASE(parent_disconnects_cable, FixtureCable) {
	auto cable_events = std::make_shared<TestSessionEvents>();
	test_client->add_session_adaptor(cable_events);
	cable_events->reset_num_calls();
	auto address_cmp = cable->get_address();

	test_client->deactivate_entity(output_component.get());
	wait_for_event(test_client, cable_events, 1);
	BOOST_TEST(!output_component->output()->is_connected_to(input_component->input()));
	BOOST_TEST(!found_cable(test_client, address_cmp));
	BOOST_TEST((cable_events->last_cable_left == address_cmp));
}

BOOST_FIXTURE_TEST_CASE(limit_connected_cables, FixtureJoinServer) {
	auto test_limited_input = std::make_unique<LimitedConnectionInputComponent>("limited_test_in", 1);
	auto test_output = std::make_unique<OutputComponent>("connect_test_out1");
	auto second_output = std::make_unique<OutputComponent>("connect_test_out2");
    test_client->get_root()->add_child(test_limited_input.get());
    test_client->get_root()->add_child(test_output.get());
    test_client->get_root()->add_child(second_output.get());
	test_client->connect_cable(test_limited_input->input.get(), test_output->output());
	test_client->connect_cable(test_limited_input->input.get(), second_output->output());
	BOOST_TEST(!test_limited_input->input->is_connected_to(test_output->output()));
	BOOST_TEST(test_limited_input->input->is_connected_to(second_output->output()));
	BOOST_TEST(test_limited_input->input->num_cables() == 1);
}

BOOST_FIXTURE_TEST_CASE(send_through_local_graph, FixtureCable) {
	int first_cmp_val = 4;

	output_component->get_downstream_compute_plug()->connect_cable(input_component->get_upstream_compute_plug());
	output_component->output()->append_int(first_cmp_val);
	input_component->execute_upstream();

	BOOST_TEST(input_component->num_hits);
	BOOST_TEST(input_component->last_received_val == first_cmp_val);
}

BOOST_FIXTURE_TEST_CASE(send_through_reliable_graph, FixtureWaitForSinkClient) {
	int first_cmp_val = 4;
	int current_wait = 0;

	auto output_component =	std::make_unique<OutputComponent>("remote_test_out", true);
	auto input_component = std::make_unique<InputComponent>("remote_test_in", first_cmp_val, false, ZstValueType::IntList, true);
	test_client->get_root()->add_child(output_component.get());
	remote_client->get_root()->add_child(input_component.get());

	// Connect cables
	test_client->connect_cable(input_component->input(), output_component->output());
	output_component->get_downstream_compute_plug()->connect_cable(input_component->get_upstream_compute_plug());

	// Clear events queues
	test_client->poll_once();
	remote_client->poll_once();

	output_component->output()->append_int(first_cmp_val);
	output_component->output()->fire();

	while (input_component->num_hits < 1 && ++current_wait < 10000) {
		remote_client->poll_once();
	}

	BOOST_TEST(input_component->num_hits);
	BOOST_TEST(input_component->last_received_val == first_cmp_val);
}


BOOST_FIXTURE_TEST_CASE(send_through_unreliable_graph, FixtureWaitForSinkClient) {
	int first_cmp_val = 4;
	int current_wait = 0;

	auto unreliable_out = std::make_unique<OutputComponent>("unreliable_out", false);
	auto input_component = std::make_unique<InputComponent>("connect_test_in", first_cmp_val, true, ZstValueType::IntList, true, false);
	test_client->get_root()->add_child(unreliable_out.get());
	remote_client->get_root()->add_child(input_component.get());

	// Connect cables
	test_client->connect_cable(input_component->input(), unreliable_out->output());
	unreliable_out->get_downstream_compute_plug()->connect_cable(input_component->get_upstream_compute_plug());
	
	// Clear events queues
	test_client->poll_once();
	remote_client->poll_once();
	
	// Send
	unreliable_out->send(first_cmp_val);

	while (input_component->num_hits < 1 && ++current_wait < 1000) {
		remote_client->poll_once();
		//TAKE_A_BREATH
	}

	BOOST_TEST(input_component->last_received_val == first_cmp_val);
}

BOOST_FIXTURE_TEST_CASE(local_cable_routes, FixtureJoinServer) {
	auto out_branch_1 = std::make_unique<ZstComponent>("a");
	auto out_branch_2 = std::make_unique<ZstComponent>("b");
	auto out_branch_3 = std::make_unique<OutputComponent>("c");
	test_client->get_root()->add_child(out_branch_1.get());
	out_branch_1->add_child(out_branch_2.get());
	out_branch_2->add_child(out_branch_3.get());

	auto in_branch_1 = std::make_unique<ZstComponent>("d");
	out_branch_1->add_child(in_branch_1.get());

	auto in_branch_2 = std::make_unique<ZstComponent>("e");
	auto in_branch_3 = std::make_unique<InputComponent>("f");
	in_branch_1->add_child(in_branch_2.get());
	in_branch_2->add_child(in_branch_3.get());

	auto cable = out_branch_3->output()->connect_cable(in_branch_3->input());

	ZstEntityBundle bundle;
	cable->get_cable_route(bundle);

	BOOST_REQUIRE(bundle.size() == 8); // count includes both performers and plugs
	BOOST_TEST(bundle.item_at(0) == out_branch_3->output());
	BOOST_TEST(bundle.item_at(1) == out_branch_3.get());
	BOOST_TEST(bundle.item_at(2) == out_branch_2.get());
	BOOST_TEST(bundle.item_at(3) == out_branch_1.get());
	BOOST_TEST(bundle.item_at(4) == in_branch_1.get());
	BOOST_TEST(bundle.item_at(5) == in_branch_2.get());
	BOOST_TEST(bundle.item_at(6) == in_branch_3.get());
	BOOST_TEST(bundle.item_at(7) == in_branch_3->input());
}

BOOST_FIXTURE_TEST_CASE(get_adjacent_components, FixtureBranchingComponents) {
	ZstEntityBundle adjacent;
	a_comp->get_adjacent_components(&adjacent, ZstPlugDirection::OUT_JACK);
	BOOST_TEST(adjacent.size() == 1);
	BOOST_TEST(adjacent[0] == b_comp.get());
	adjacent.clear();

	a_comp->get_adjacent_components(&adjacent, ZstPlugDirection::IN_JACK);
	BOOST_TEST(adjacent.size() == 0);
	adjacent.clear();

	b_comp->get_adjacent_components(&adjacent, ZstPlugDirection::IN_JACK);
	BOOST_TEST(adjacent.size() == 1);
	BOOST_TEST(adjacent[0] == a_comp.get());
	adjacent.clear();

	b_comp->get_adjacent_components(&adjacent, ZstPlugDirection::OUT_JACK);
	BOOST_TEST(adjacent.size() == 2);
	BOOST_TEST((std::find_if(adjacent.begin(), adjacent.end(), [component = c_comp.get()](ZstEntityBase* ent) {return ent->URI() == component->URI(); }) != adjacent.end()));
	BOOST_TEST((std::find_if(adjacent.begin(), adjacent.end(), [component = d_comp.get()](ZstEntityBase* ent) {return ent->URI() == component->URI(); }) != adjacent.end()));
	adjacent.clear();

	d_comp->get_adjacent_components(&adjacent, ZstPlugDirection::IN_JACK);
	BOOST_TEST(adjacent.size() == 2);
	BOOST_TEST((std::find_if(adjacent.begin(), adjacent.end(), [component = b_comp.get()](ZstEntityBase* ent) {return ent->URI() == component->URI(); }) != adjacent.end()));
	BOOST_TEST((std::find_if(adjacent.begin(), adjacent.end(), [component = c_comp.get()](ZstEntityBase* ent) {return ent->URI() == component->URI(); }) != adjacent.end()));
}

BOOST_FIXTURE_TEST_CASE(local_component_dependencies, FixtureBranchingComponents) {
	ZstEntityBundle entities;
	a_comp->dependants(&entities, false, true);
	BOOST_TEST(entities.size() == 4);
	BOOST_TEST(entities[0] == a_comp.get());
	BOOST_TEST(entities[1] == b_comp.get());
	BOOST_TEST(entities[2] == c_comp.get());
	BOOST_TEST(entities[3] == d_comp.get());
	entities.clear();
	
	d_comp->dependencies(&entities, false, true);
	BOOST_TEST(entities.size() == 4);
	BOOST_TEST(entities[0] == a_comp.get());
	BOOST_TEST(entities[1] == b_comp.get());
	BOOST_TEST(entities[2] == c_comp.get());
	BOOST_TEST(entities[3] == d_comp.get());
	entities.clear();
}

BOOST_FIXTURE_TEST_CASE(renaming_entity_updates_cables, FixtureCable) {
	auto orig_cable_address = cable->get_address();
	cable->get_input()->set_name("renamed_input");
	TAKE_A_BREATH
	BOOST_REQUIRE(cable->get_input());
	BOOST_REQUIRE(cable->get_output());
	BOOST_TEST(!(test_client->find_cable(orig_cable_address)));
	auto new_address = cable->get_address();
	BOOST_TEST(test_client->find_cable(new_address));
	BOOST_TEST(cable->get_input()->is_connected_to(cable->get_output()));
	BOOST_TEST(cable->get_output()->is_connected_to(cable->get_input()));
}

BOOST_FIXTURE_TEST_CASE(send_int, FixtureJoinServer) {
	std::unique_ptr<OutputComponent> output_component = std::make_unique<OutputComponent>("connect_test_out", true, ZstValueType::IntList);
	std::unique_ptr<InputComponent> input_component = std::make_unique<InputComponent>("connect_test_in", 0, true, ZstValueType::IntList);
	test_client->get_root()->add_child(output_component.get());
	test_client->get_root()->add_child(input_component.get());

	int first_cmp_val = 4;
	int current_wait = 0;

	test_client->connect_cable(input_component->input(), output_component->output());
	output_component->output()->append_int(first_cmp_val);
	output_component->output()->fire();
	while (input_component->num_hits < 1 && ++current_wait < 10000) {
		test_client->poll_once();
	}
	//BOOST_TEST(input_component->num_hits);
	BOOST_TEST(input_component->input()->int_at(0) == first_cmp_val); 
}

BOOST_FIXTURE_TEST_CASE(send_float, FixtureJoinServer) {
	std::unique_ptr<OutputComponent> output_component = std::make_unique<OutputComponent>("connect_test_out", true, ZstValueType::FloatList);
	std::unique_ptr<InputComponent> input_component = std::make_unique<InputComponent>("connect_test_in", 0, true, ZstValueType::FloatList);
	test_client->get_root()->add_child(output_component.get());
	test_client->get_root()->add_child(input_component.get());

	float first_cmp_val = 2.7f;
	int current_wait = 0;

	test_client->connect_cable(input_component->input(), output_component->output());
	output_component->output()->append_float(first_cmp_val);
	output_component->output()->fire();
	while (input_component->num_hits < 1 && ++current_wait < 10000) {
		test_client->poll_once();
	}
	//BOOST_TEST(input_component->num_hits);
	BOOST_TEST(fabs(input_component->input()->float_at(0) - first_cmp_val) < FLT_EPSILON);
	//BOOST_TEST(input_component->last_received_val == first_cmp_val);
}

BOOST_FIXTURE_TEST_CASE(send_string, FixtureJoinServer) {
	std::unique_ptr<OutputComponent> output_component = std::make_unique<OutputComponent>("connect_test_out", true, ZstValueType::StrList);
	std::unique_ptr<InputComponent> input_component = std::make_unique<InputComponent>("connect_test_in", 0, true, ZstValueType::StrList);
	test_client->get_root()->add_child(output_component.get());
	test_client->get_root()->add_child(input_component.get());

	std::string first_cmp_val = "pineapple";
	std::string second_cmp_val = "pen";

	int current_wait = 0;

	test_client->connect_cable(input_component->input(), output_component->output());
	output_component->output()->append_string(first_cmp_val.c_str(), first_cmp_val.size());
	output_component->output()->append_string(second_cmp_val.c_str(), second_cmp_val.size());
	output_component->output()->fire();
	
	while (input_component->num_hits < 1 && ++current_wait < 10000) {
		test_client->poll_once();
	}
	
	BOOST_REQUIRE(input_component->input()->size() == 2);
	size_t size_a;
	const char* first = input_component->input()->string_at(0, size_a);
	std::string first_str(first, size_a);

	size_t size_b;
	const char* second = input_component->input()->string_at(1, size_b);
	std::string second_str(second, size_b);

	BOOST_TEST(first_str == first_cmp_val);
	BOOST_TEST(second_str == second_cmp_val);
}


BOOST_FIXTURE_TEST_CASE(send_bytes, FixtureJoinServer) {
	std::unique_ptr<OutputComponent> output_component = std::make_unique<OutputComponent>("connect_test_out", true, ZstValueType::ByteList);
	std::unique_ptr<InputComponent> input_component = std::make_unique<InputComponent>("connect_test_in", 0, true, ZstValueType::ByteList);
	test_client->get_root()->add_child(output_component.get());
	test_client->get_root()->add_child(input_component.get());

	std::string first_cmp_val = "pineapple";
	int current_wait = 0;

	test_client->connect_cable(input_component->input(), output_component->output());

	for (size_t idx = 0; idx < first_cmp_val.size(); ++idx){
		output_component->output()->append_byte(first_cmp_val[idx]);
	}
	output_component->output()->append_byte(0x0);
	output_component->output()->fire();

	while (input_component->num_hits < 1 && ++current_wait < 10000) {
		test_client->poll_once();
	}
	char* recv_val = (char*)malloc(input_component->input()->size());
	for (size_t idx = 0; idx < input_component->input()->size(); ++idx) {
		recv_val[idx] = input_component->input()->byte_at(idx);
	}
	BOOST_TEST(strcmp(recv_val, first_cmp_val.c_str()) == 0);
}


BOOST_FIXTURE_TEST_CASE(send_byte_fixed, FixtureJoinServer) {
#define TEST_BUF_SIZE 4
	size_t buffer_size = TEST_BUF_SIZE;
	std::unique_ptr<OutputComponent> output_component = std::make_unique<OutputComponent>("connect_test_out", true, ZstValueType::ByteList, buffer_size);
	std::unique_ptr<InputComponent> input_component = std::make_unique<InputComponent>("connect_test_in", 0, true, ZstValueType::ByteList, false, true, buffer_size);
	test_client->get_root()->add_child(output_component.get());
	test_client->get_root()->add_child(input_component.get());

	std::array<uint8_t, TEST_BUF_SIZE> send_buffer = { 0xDE, 0xAD, 0xBE, 0xEF };
	output_component->output()->raw_value()->assign(send_buffer.data(), send_buffer.size());

	int current_wait = 0;

	test_client->connect_cable(input_component->input(), output_component->output());
	output_component->output()->fire();
	while (input_component->num_hits < 1 && ++current_wait < 10000) {
		test_client->poll_once();
	}
	//BOOST_TEST(input_component->num_hits);

	std::array<uint8_t, TEST_BUF_SIZE> recv_buffer;
	std::copy(input_component->input()->raw_value()->byte_buffer(), input_component->input()->raw_value()->byte_buffer() + input_component->input()->raw_value()->size(), &recv_buffer[0]);
	bool equal = send_buffer == recv_buffer;
	BOOST_TEST(equal);
#undef TEST_BUF_SIZE
}


//BOOST_FIXTURE_TEST_CASE(send_string, FixtureJoinServer) {
//	std::unique_ptr<OutputComponent> output_component = std::make_unique<OutputComponent>("connect_test_out", true, ZstValueType::FloatList);
//	std::unique_ptr<InputComponent> input_component = std::make_unique<InputComponent>("connect_test_in", true, ZstValueType::StrList);
//	test_client->get_root()->add_child(output_component.get());
//	test_client->get_root()->add_child(input_component.get());
//
//	std::string first_cmp_val = "pineapple";
//	int current_wait = 0;
//
//	auto cable = test_client->connect_cable(input_component->input(), output_component->output());
//	output_component->send(first_cmp_val);
//	while (input_component->num_hits < 1 && ++current_wait < 10000) {
//		test_client->poll_once();
//	}
//	BOOST_TEST(input_component->num_hits);
//	char* val = "";
//	BOOST_TEST(input_component->input()->string_at(val, 0) == first_cmp_val);
//}

BOOST_FIXTURE_TEST_CASE(buffer_ownership, FixtureJoinServer) {
#define TEST_BUF_SIZE 4
	size_t buffer_size = TEST_BUF_SIZE;
	std::unique_ptr<OutputComponent> output_component = std::make_unique<OutputComponent>("connect_test_out", true, ZstValueType::ByteList, buffer_size);
	std::unique_ptr<InputComponent> input_component = std::make_unique<InputComponent>("connect_test_in", 0, true, ZstValueType::ByteList, false, true, buffer_size);
	test_client->get_root()->add_child(output_component.get());
	test_client->get_root()->add_child(input_component.get());


	auto send_cmp = std::array<uint8_t, TEST_BUF_SIZE>{ 0xDE, 0xAD, 0xBE, 0xEF };
	uint8_t* send_buffer = new uint8_t[TEST_BUF_SIZE]{ 0xDE, 0xAD, 0xBE, 0xEF };
	output_component->output()->raw_value()->take(send_buffer, TEST_BUF_SIZE);

	int current_wait = 0;

	test_client->connect_cable(input_component->input(), output_component->output());
	output_component->output()->fire();
	while (input_component->num_hits < 1 && ++current_wait < 10000) {
		test_client->poll_once();
	}
	BOOST_TEST(send_buffer == input_component->input()->raw_value()->byte_buffer());
	BOOST_TEST(input_component->input()->raw_value()->release() == send_buffer);

	// Make sure that we can release the taken buffer
	input_component->input()->raw_value()->clear();
	std::array<uint8_t, TEST_BUF_SIZE> send_data;
	std::copy(send_buffer, send_buffer + TEST_BUF_SIZE, &send_data[0]);
	BOOST_TEST(send_cmp == send_data);

#undef TEST_BUF_SIZE
}
