#include "../src/core_entities/Adder.h"
#include "TestCommon.hpp"

using namespace ZstTest;

void test_connect_plugs() {
    ZstLog::app(LogLevel::notification, "Running connect plugs test");
    
    ZstLog::app(LogLevel::debug, "Creating entities");
    OutputComponent * test_output = new OutputComponent("connect_test_out");
    InputComponent * test_input = new InputComponent("connect_test_in", 0);
    zst_activate_entity(test_output);
    zst_activate_entity(test_input);

    ZstLog::app(LogLevel::debug, "Testing sync cable connection");
    ZstCable * cable = zst_connect_cable(test_input->input(), test_output->output());
    assert(cable);
    assert(cable->is_activated());
    
    ZstLog::app(LogLevel::debug, "Verifying cable");
    assert(cable->get_output() == test_output->output());
    assert(cable->get_input() == test_input->input());
    assert(test_output->output()->is_connected_to(test_input->input()));
    assert(test_input->input()->is_connected_to(test_output->output()));
    for (auto c : ZstCableBundleScoped(test_output->output())) {
        assert(c->get_input() == test_input->input());
    }

    ZstLog::app(LogLevel::debug, "Testing cable disconnection");
    zst_destroy_cable(cable);
    cable = 0;
    assert(!test_output->output()->is_connected_to(test_input->input()));
    assert(!test_input->input()->is_connected_to(test_output->output()));
    
    ZstLog::app(LogLevel::debug, "Testing async cable connection");
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

    ZstLog::app(LogLevel::debug, "Testing cable disconnection when removing parent");
    cable = zst_connect_cable(test_input->input(), test_output->output());
    cable->add_adaptor(cable_activation);
    cable_activation->reset_num_calls();
    zst_deactivate_entity(test_output);
    wait_for_event(cable_activation, 1);
    assert(!test_input->input()->is_connected_to(test_output->output()));
    assert(!test_output->output()->is_connected_to(test_input->input()));
    
    //Cleanup
    zst_deactivate_entity(test_input);
    clear_callback_queue();
    delete test_output;
    delete test_input;
    delete cable_activation;
}

void test_add_filter() {
    ZstLog::app(LogLevel::notification, "Starting addition filter test");
    int first_cmp_val = 4;
    int second_cmp_val = 30;

    ZstLog::app(LogLevel::debug, "Creating input/output components for addition filter");
    OutputComponent * test_output_augend = new OutputComponent("add_test_augend");
    OutputComponent * test_output_addend = new OutputComponent("add_test_addend");
    InputComponent * test_input_sum = new InputComponent("add_test_sum", first_cmp_val, true);
    Adder * add_filter = new Adder("add_test");
    
    zst_activate_entity(test_output_augend);
    zst_activate_entity(test_output_addend);
    zst_activate_entity(test_input_sum);
    zst_activate_entity(add_filter);
    
    ZstLog::app(LogLevel::debug, "Connecting cables");
    zst_connect_cable(add_filter->augend(), test_output_augend->output() );
    zst_connect_cable(add_filter->addend(), test_output_addend->output());
    zst_connect_cable(test_input_sum->input(), add_filter->sum());
    
    TAKE_A_BREATH

    //Send values
    ZstLog::app(LogLevel::debug, "Sending values");
    test_output_augend->send(2);
    test_output_addend->send(2);

    TAKE_A_BREATH

    int max_wait = 10000;
    int current_wait = 0;

    //Wait for the first two input callbacks to clear before we check for the sum
    while(test_input_sum->num_hits < 2 && ++current_wait < max_wait){
        zst_poll_once();
    }
    assert(test_input_sum->last_received_val == first_cmp_val);
    test_input_sum->reset();
    test_input_sum->last_received_val = 0;
    current_wait = 0;

    //Send more values
    test_input_sum->compare_val = second_cmp_val;
    test_output_augend->send(20);
    test_output_addend->send(10);

    while (test_input_sum->num_hits < 2 && ++current_wait < max_wait){
        zst_poll_once();
    }
    assert(test_input_sum->last_received_val == second_cmp_val);
    ZstLog::app(LogLevel::debug, "Addition component succeeded at addition!");

    //Cleanup
    zst_deactivate_entity(test_output_augend);
    zst_deactivate_entity(test_output_addend);
    zst_deactivate_entity(test_input_sum);
    zst_deactivate_entity(add_filter);
    clear_callback_queue();
    delete test_output_augend;
    delete test_output_addend;
    delete test_input_sum;
    delete add_filter;
}

void test_unreliable_graph()
{
    ZstLog::app(LogLevel::notification, "Starting unreliable graph test");
    int first_cmp_val = 4;

   OutputComponent * test_output = new OutputComponent("unreliable", false);
    InputComponent * test_input = new InputComponent("reliable", first_cmp_val, true);

    zst_activate_entity(test_output);
    zst_activate_entity(test_input);
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
}


int main(int argc,char **argv)
{
	TestRunner runner("TestGraph", argv[0]);

    test_connect_plugs();
    test_add_filter();

	//Unreliable test disabled until polling performance issues are sorted
    test_unreliable_graph();
}
