#include "TestCommon.hpp"

using namespace ZstTest;

class LimitedConnectionInputComponent : public ZstComponent
{
public:
	LimitedConnectionInputComponent(std::string name, int max_cables) : ZstComponent("LimitedConnectionInputComponent", name.c_str()) {
		input = create_input_plug("limited_input", ZstValueType::ZST_INT, max_cables);
	}
	ZstInputPlug * input;
};


void test_connect_plugs() {
    ZstLog::app(LogLevel::notification, "Running connect plugs test");
    
    OutputComponent * test_output = new OutputComponent("connect_test_out");
    InputComponent * test_input = new InputComponent("connect_test_in", 0);
    zst_get_root()->add_child(test_output);
    zst_get_root()->add_child(test_input);

    ZstLog::app(LogLevel::notification, "Testing sync cable connection");
    ZstCable * cable = zst_connect_cable(test_input->input(), test_output->output());
    assert(cable);
    assert(cable->is_activated());
    
    ZstLog::app(LogLevel::debug, "Verifying cable");
    assert(cable->get_output() == test_output->output());
    assert(cable->get_input() == test_input->input());
    assert(test_output->output()->is_connected_to(test_input->input()));
    assert(test_input->input()->is_connected_to(test_output->output()));

	ZstCableBundle bundle;
    for (auto c : test_output->output()->get_child_cables(bundle)) {
        assert(c.get_input_URI() == test_input->input()->URI());
    }

    ZstLog::app(LogLevel::notification, "Testing cable disconnection");
    zst_destroy_cable(cable);
    assert(!test_output->output()->is_connected_to(test_input->input()));
    assert(!test_input->input()->is_connected_to(test_output->output()));
    
    ZstLog::app(LogLevel::notification, "Testing async cable connection");
    TestSynchronisableEvents * cable_activation = new TestSynchronisableEvents();
    cable = zst_connect_cable_async(test_input->input(), test_output->output());
    cable->add_adaptor(cable_activation);
    wait_for_event(cable_activation, 1);
    assert(cable_activation->num_calls() == 1);
    cable_activation->reset_num_calls();
        
    zst_destroy_cable_async(cable);
    cable = 0;
    wait_for_event(cable_activation, 1);
    assert(cable_activation->num_calls() == 1);
    cable_activation->reset_num_calls();

	ZstLog::app(LogLevel::notification, "Testing json serialisation of entity");
	ZstLog::app(LogLevel::debug, "{}", zst_get_root()->as_json_str());

    ZstLog::app(LogLevel::notification, "Testing cable disconnection when removing parent");
    cable = zst_connect_cable(test_input->input(), test_output->output());
    cable->add_adaptor(cable_activation);
    cable_activation->reset_num_calls();
    zst_deactivate_entity(test_output);
    wait_for_event(cable_activation, 1);
    assert(!test_input->input()->is_connected_to(test_output->output()));
    assert(!test_output->output()->is_connected_to(test_input->input()));

	ZstLog::app(LogLevel::notification, "Testing setting maximum amount of cables that can be connected to an input");
	LimitedConnectionInputComponent * test_limited_input = new LimitedConnectionInputComponent("limited_test_in", 1);
	OutputComponent * second_output = new OutputComponent("connect_test_out2");
    zst_get_root()->add_child(test_limited_input);
    zst_get_root()->add_child(test_output);
    zst_get_root()->add_child(second_output);
	zst_connect_cable(test_limited_input->input, test_output->output());
	zst_connect_cable(test_limited_input->input, second_output->output());
	assert(!test_limited_input->input->is_connected_to(test_output->output()));
	assert(test_limited_input->input->is_connected_to(second_output->output()));
	assert(test_limited_input->input->num_cables() == 1);
    
    //Cleanup
    zst_deactivate_entity(test_input);
    clear_callback_queue();
    delete test_output;
	delete second_output;
    delete test_input;
    delete cable_activation;
	delete test_limited_input;
}

//void test_add_filter() {
//    ZstLog::app(LogLevel::notification, "Starting addition filter test");
//    int first_cmp_val = 4;
//    int second_cmp_val = 30;
//
//    ZstLog::app(LogLevel::debug, "Creating input/output components for addition filter");
//    OutputComponent * test_output_augend = new OutputComponent("add_test_augend");
//    OutputComponent * test_output_addend = new OutputComponent("add_test_addend");
//    InputComponent * test_input_sum = new InputComponent("add_test_sum", first_cmp_val, true);
//    Adder * add_filter = new Adder("add_test");
//    
//    zst_activate_entity(test_output_augend);
//    zst_activate_entity(test_output_addend);
//    zst_activate_entity(test_input_sum);
//    zst_activate_entity(add_filter);
//    
//    ZstLog::app(LogLevel::debug, "Connecting cables");
//    zst_connect_cable(add_filter->augend(), test_output_augend->output() );
//    zst_connect_cable(add_filter->addend(), test_output_addend->output());
//    zst_connect_cable(test_input_sum->input(), add_filter->sum());
//    
//    TAKE_A_BREATH
//
//    //Send values
//    ZstLog::app(LogLevel::debug, "Sending values");
//    test_output_augend->send(2);
//    test_output_addend->send(2);
//
//    TAKE_A_BREATH
//
//    int max_wait = 10000;
//    int current_wait = 0;
//
//    //Wait for the first two input callbacks to clear before we check for the sum
//    while(test_input_sum->num_hits < 2 && ++current_wait < max_wait){
//        zst_poll_once();
//    }
//    assert(test_input_sum->last_received_val == first_cmp_val);
//    test_input_sum->reset();
//    test_input_sum->last_received_val = 0;
//    current_wait = 0;
//
//    //Send more values
//    test_input_sum->compare_val = second_cmp_val;
//    test_output_augend->send(20);
//    test_output_addend->send(10);
//
//    while (test_input_sum->num_hits < 2 && ++current_wait < max_wait){
//        zst_poll_once();
//    }
//    assert(test_input_sum->last_received_val == second_cmp_val);
//    ZstLog::app(LogLevel::debug, "Addition component succeeded at addition!");
//
//    //Cleanup
//    zst_deactivate_entity(test_output_augend);
//    zst_deactivate_entity(test_output_addend);
//    zst_deactivate_entity(test_input_sum);
//    zst_deactivate_entity(add_filter);
//    clear_callback_queue();
//    delete test_output_augend;
//    delete test_output_addend;
//    delete test_input_sum;
//    delete add_filter;
//}

void test_reliable_graph()
{
	ZstLog::app(LogLevel::notification, "Starting reliable graph test");
	int first_cmp_val = 4;

	OutputComponent * test_output = new OutputComponent("reliable_out");
	InputComponent * test_input = new InputComponent("reliable_in", first_cmp_val, true);
    zst_get_root()->add_child(test_output);
    zst_get_root()->add_child(test_input);
	zst_connect_cable(test_input->input(), test_output->output());
	test_output->send(first_cmp_val);

	int max_wait = 10000;
	int current_wait = 0;
	while (test_input->num_hits < 1 && ++current_wait < max_wait) {
		zst_poll_once();
	}
	assert(test_input->last_received_val == first_cmp_val);

	zst_deactivate_entity(test_output);
	zst_deactivate_entity(test_input);
	clear_callback_queue();
	delete test_output;
	delete test_input;
}


void test_unreliable_graph()
{
    ZstLog::app(LogLevel::notification, "Starting unreliable graph test");
    int first_cmp_val = 4;

    OutputComponent * test_output = new OutputComponent("unreliable_out", false);
    InputComponent * test_input = new InputComponent("reliable_in", first_cmp_val, true);
    
#ifndef ZST_BUILD_DRAFT_API
    // If we don't have draft support enabled, check reliable fallback has been set
    assert(test_output->output()->is_reliable());
#else
    zst_get_root()->add_child(test_output);
    zst_get_root()->add_child(test_input);
    zst_connect_cable(test_input->input(), test_output->output());
    test_output->send(first_cmp_val);

    int max_wait = 10000;
    int current_wait = 0;
    while (test_input->num_hits < 1 && ++current_wait < max_wait) {
        zst_poll_once();
    }
    assert(test_input->last_received_val == first_cmp_val);
#endif
	//Test if deleting plugs first triggers deactivation
	delete test_output;
	delete test_input;
	clear_callback_queue();
}


int main(int argc,char **argv)
{
	TestRunner runner("TestGraph", argv[0], true);

    test_connect_plugs();
    //test_add_filter();
	test_reliable_graph();
    test_unreliable_graph();
    return 0;
}
