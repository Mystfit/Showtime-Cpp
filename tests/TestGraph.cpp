#define BOOST_TEST_MODULE Graph

#include "TestCommon.hpp"

using namespace ZstTest;

class LimitedConnectionInputComponent : public ZstComponent {
public:
	LimitedConnectionInputComponent(std::string name, int max_cables) : ZstComponent("LimitedConnectionInputComponent", name.c_str()) {
		input = create_input_plug("limited_input", ZstValueType::ZST_INT, max_cables);
	}
	ZstInputPlug * input;
};


struct FixtureCableCompare {
	ZstURI in = ZstURI("a/1");
	ZstURI out = ZstURI("b/1");
	ZstURI less = ZstURI("a/b");
	ZstURI more = ZstURI("b/a");
	std::unique_ptr<ZstInputPlug> plug_in_A = std::make_unique<ZstInputPlug>(in.path(), ZST_INT);
	std::unique_ptr<ZstOutputPlug> plug_out_A = std::make_unique<ZstOutputPlug>(out.path(), ZST_INT);
	std::unique_ptr<ZstInputPlug> plug_in_B = std::make_unique<ZstInputPlug>(less.path(), ZST_INT);
	std::unique_ptr<ZstOutputPlug> plug_out_B = std::make_unique<ZstOutputPlug>(more.path(), ZST_INT);
	std::unique_ptr<ZstInputPlug> plug_in_C = std::make_unique<ZstInputPlug>(more.path(), ZST_INT);
	std::unique_ptr<ZstOutputPlug> plug_out_C = std::make_unique<ZstOutputPlug>(less.path(), ZST_INT);
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
		zst_get_root()->add_child(output_component.get());
		zst_get_root()->add_child(input_component.get());
	}
	~FixturePlugs() {};
};


struct FixtureCable : public FixturePlugs {
	ZstCable* cable;
	std::shared_ptr<TestSynchronisableEvents> cable_activation_events;

	FixtureCable() : cable_activation_events(std::make_shared<TestSynchronisableEvents>())
	{
		cable = zst_connect_cable(input_component->input(), output_component->output());
		cable->add_adaptor(cable_activation_events.get());
		cable_activation_events->reset_num_calls();
	}
	~FixtureCable() {
		cable->remove_adaptor(cable_activation_events.get());
	};
};


bool found_cable(ZstCableAddress cable_address) {
	ZstCableBundle bundle;
	zst_get_root()->get_child_cables(bundle);
	for (auto c : bundle) {
		if (c->get_address() == cable_address) {
			return true;
		}
	}
	return false;
}


BOOST_FIXTURE_TEST_CASE(cable_URIs, FixtureCableCompare)
{
	BOOST_TEST(cable_a->get_address().get_input_URI() == in);
	BOOST_TEST(cable_a->get_address().get_output_URI() == out);
	BOOST_TEST(cable_a->is_attached(out));
	BOOST_TEST(cable_a->is_attached(in));
}

BOOST_FIXTURE_TEST_CASE(cable_comparisons, FixtureCableCompare)
{
	//Test cable comparisons
	BOOST_TEST(ZstCableAddressEq{}(cable_a->get_address(), cable_b->get_address()));
	BOOST_TEST(ZstCableAddressHash{}(cable_a->get_address()) == ZstCableAddressHash{}(cable_b->get_address()));
	BOOST_TEST(ZstCableCompare{}(cable_c, cable_d));
	BOOST_TEST(ZstCableCompare{}(cable_c->get_address(), cable_d));
	BOOST_TEST(ZstCableCompare{}(cable_c, cable_d->get_address()));
	BOOST_TEST(!(ZstCableCompare{}(cable_d->get_address(), cable_c)));
	BOOST_TEST(!(ZstCableCompare{}(cable_d, cable_c->get_address())));
}

BOOST_FIXTURE_TEST_CASE(cable_sets, FixtureCableCompare)
{
	//Test cable sets
	std::set<std::unique_ptr<ZstCable>, ZstCableCompare> cable_set;
	cable_set.insert(std::move(cable_c));
	cable_set.insert(std::move(cable_d));
	BOOST_TEST((cable_set.find(cable_c) != cable_set.end()));
	BOOST_TEST((cable_set.find(cable_d) != cable_set.end()));
}

BOOST_FIXTURE_TEST_CASE(sync_connect_cable, FixturePlugs) {
	auto cable = zst_connect_cable(input_component->input(), output_component->output());
	BOOST_TEST(cable);
	BOOST_TEST(cable->is_activated());
	BOOST_TEST(cable->get_output() == output_component->output());
	BOOST_TEST(cable->get_input() == input_component->input());
	BOOST_TEST(output_component->output()->is_connected_to(input_component->input()));
	BOOST_TEST(input_component->input()->is_connected_to(output_component->output()));
}

BOOST_FIXTURE_TEST_CASE(async_connect_cable_callback, FixturePlugs) {
	auto cable_activation_events = std::make_shared<TestSynchronisableEvents>();
	auto cable = zst_connect_cable_async(input_component->input(), output_component->output());
	cable->add_adaptor(cable_activation_events.get());
	
	wait_for_event(cable_activation_events.get(), 1);
	BOOST_TEST(cable->is_activated());
}

BOOST_FIXTURE_TEST_CASE(async_destroy_cable_callback, FixtureCable) {
	auto address_cmp = cable->get_address();
	zst_destroy_cable_async(cable);
	wait_for_event(cable_activation_events.get(), 1);
	BOOST_TEST(!found_cable(address_cmp));
}

BOOST_FIXTURE_TEST_CASE(get_cable_from_plug, FixtureCable) {
	ZstCableBundle bundle;
	output_component->output()->get_child_cables(bundle);
	for (auto c : bundle) {
		BOOST_TEST(c->get_address().get_input_URI() == input_component->input()->URI());
	}
}

BOOST_FIXTURE_TEST_CASE(disconnect_cable, FixtureCable) {
	zst_destroy_cable(cable);
	BOOST_TEST(!output_component->output()->is_connected_to(input_component->input()));
	BOOST_TEST(!found_cable(ZstCableAddress(input_component->input()->URI(), output_component->output()->URI())));
}

BOOST_FIXTURE_TEST_CASE(parent_disconnects_cable, FixtureCable) {
	auto address_cmp = cable->get_address();
	zst_deactivate_entity(output_component.get());
	wait_for_event(cable_activation_events.get(), 1);
	BOOST_TEST(!output_component->output()->is_connected_to(input_component->input()));
	BOOST_TEST(!found_cable(address_cmp));
}


BOOST_FIXTURE_TEST_CASE(limit_connected_cables, FixtureJoinServer) {
	auto test_limited_input = std::make_unique<LimitedConnectionInputComponent>("limited_test_in", 1);
	auto test_output = std::make_unique<OutputComponent>("connect_test_out1");
	auto second_output = std::make_unique<OutputComponent>("connect_test_out2");
    zst_get_root()->add_child(test_limited_input.get());
    zst_get_root()->add_child(test_output.get());
    zst_get_root()->add_child(second_output.get());
	zst_connect_cable(test_limited_input->input, test_output->output());
	zst_connect_cable(test_limited_input->input, second_output->output());
	BOOST_TEST(!test_limited_input->input->is_connected_to(test_output->output()));
	BOOST_TEST(test_limited_input->input->is_connected_to(second_output->output()));
	BOOST_TEST(test_limited_input->input->num_cables() == 1);
}

BOOST_FIXTURE_TEST_CASE(send_through_reliable_graph, FixtureCable) {
	int first_cmp_val = 4;
	int current_wait = 0;

	output_component->send(first_cmp_val);
	while (input_component->num_hits < 1 && ++current_wait < 10000) {
		zst_poll_once();
	}
	BOOST_TEST(input_component->last_received_val == first_cmp_val);
}

BOOST_FIXTURE_TEST_CASE(send_through_unreliable_graph, FixturePlugs) {
	int first_cmp_val = 4;

	auto unreliable_out = std::make_unique<OutputComponent>("unreliable_out", false);
#ifndef ZST_BUILD_DRAFT_API
	// If we don't have draft support enabled, check reliable fallback has been set
	BOOST_TEST(unreliable_out->output()->is_reliable());
#else
	zst_get_root()->add_child(unreliable_out);
	zst_connect_cable(input_component->input(), unreliable_out->output());
	unreliable_out->send(first_cmp_val);

	int current_wait = 0;
	while (input_component->num_hits < 1 && ++current_wait < 10000) {
		zst_poll_once();
	}
	BOOST_TEST(input_component->last_received_val == first_cmp_val);
#endif
}
