#include <string>
#include <iostream>
#include "Showtime.h"
#include "ZstPlug.h"
#include "ZstPerformer.h"
#include "ZstStage.h"

ZstStage *stage;
Showtime *performer_a;
Showtime *performer_b;

void test_performer_init() {
	Showtime::instance().self_test();
}

//Test stage creation and performer
void test_stage_registration(){
    //Test stage connection
    assert(Showtime::instance().ping_stage().count() >= 0);
    
    assert(stage->get_performer_ref_by_name("test_performer_1") != NULL);
    assert(stage->get_performer_ref_by_name("non_existing_performer") == NULL);
}

void test_create_plugs(){
    //Create new plugs
	ZstIntPlug *outputPlug = Showtime::create_plug<ZstIntPlug>("test_performer_1", "test_output_plug", "test_instrument", PlugDir::OUT_JACK);
	ZstIntPlug *inputPlug = Showtime::create_plug<ZstIntPlug>("test_performer_1", "test_input_plug", "test_instrument", PlugDir::IN_JACK);
    
    //Check address equality
    assert(outputPlug->get_address() == outputPlug->get_address());
    assert(!(outputPlug->get_address() == inputPlug->get_address()));

    //Check stage registered plugs successfully
    ZstPerformerRef *stagePerformerRef = stage->get_performer_ref_by_name("test_performer_1");
    assert(stagePerformerRef->get_plug_by_name(outputPlug->get_name()) != NULL);
    assert(stagePerformerRef->get_plug_by_name(inputPlug->get_name()) != NULL);
    
    //Check local client registered plugs correctly
	ZstPlug *localPlug = Showtime::get_performer_by_name("test_performer_1")->get_instrument_plugs("test_instrument")[0];
    assert(localPlug->get_name() == stagePerformerRef->get_plug_by_name(outputPlug->get_name())->get_address().name);
    
    std::vector<ZstPlug*> localplugs = Showtime::get_performer_by_name("test_performer_1")->get_plugs();
    assert(localplugs.size() > 1);
    assert(stagePerformerRef->get_plug_by_name(localplugs[0]->get_name()) != NULL);
    assert(stagePerformerRef->get_plug_by_name(localplugs[1]->get_name()) != NULL);
    
    //Query stage for remote plugs
    std::vector<PlugAddress> plugs = Showtime::instance().get_all_plug_addresses();
    assert(plugs.size() > 0);
    plugs = Showtime::instance().get_all_plug_addresses("test_performer_1");
    assert(plugs.size() > 0);
    plugs = Showtime::instance().get_all_plug_addresses("non_existing_performer");
    assert(plugs.size() == 0);
    plugs = Showtime::instance().get_all_plug_addresses("test_performer_1", "test_instrument");
    assert(plugs.size() > 0);

	//Check plug destruction
	std::string outputName = outputPlug->get_name();
	std::string inputName = inputPlug->get_name();

	Showtime::instance().destroy_plug(outputPlug);
	assert(stage->get_performer_ref_by_name("test_performer_1")->get_plug_by_name(outputName) == NULL);
	assert(Showtime::get_performer_by_name("test_performer_1")->get_instrument_plugs("test_instrument").size() == 1);
	
	Showtime::instance().destroy_plug(inputPlug);
	assert(stage->get_performer_ref_by_name("test_performer_1")->get_plug_by_name(inputName) == NULL);
	assert(Showtime::get_performer_by_name("test_performer_1")->get_instrument_plugs("test_instrument").empty());
}


class TestIntCallback : public PlugCallback{
public:
    //~TestCallback();
    void run(ZstPlug* plug) override {
        assert(((ZstIntPlug*)plug)->get_value() == 27);
    };
};

class TestFloatCallback : public PlugCallback{
public:
    //~TestCallback();
    void run(ZstPlug* plug) override {
        assert(((ZstFloatPlug*)plug)->get_value() == 27.5f);
    };
};

class TestIntListCallback : public PlugCallback{
public:
    //~TestCallback();
    void run(ZstPlug* plug) override {
        assert(((ZstIntListPlug*)plug)->get_value()[0] == 27);
    };
};

class TestFloatListCallback : public PlugCallback{
public:
    //~TestCallback();
    void run(ZstPlug* plug) override {
        assert(((ZstFloatListPlug*)plug)->get_value()[0] == 27.5f);
    };
};

class TestStringCallback : public PlugCallback{
public:
    //~TestCallback();
    void run(ZstPlug* plug) override {
        assert(((ZstStringPlug*)plug)->get_value() == "Twenty seven");
    };
};


void test_connect_plugs() {
	//Test plugs connected between performers
	ZstIntPlug *output_int_plug = Showtime::create_plug<ZstIntPlug>("test_performer_1", "output_int_plug", "test_instrument", PlugDir::OUT_JACK);
	ZstIntPlug *input_int_plug = Showtime::create_plug<ZstIntPlug>("test_performer_2", "input_int_plug", "test_instrument", PlugDir::IN_JACK);
    input_int_plug->attach_recv_callback(new TestIntCallback());
	Showtime::connect_plugs(output_int_plug->get_address(), input_int_plug->get_address());

    //TODO: First connection, so need to wait for endpoint->stage->endpoint handshake to complete. Futures could help with this?
#ifdef WIN32
    Sleep(500);
#else
    sleep(1);
#endif
	output_int_plug->fire(27);
    
    ZstFloatPlug *output_float_plug = Showtime::create_plug<ZstFloatPlug>("test_performer_1", "output_float_plug", "test_instrument", PlugDir::OUT_JACK);
    ZstFloatPlug *input_float_plug = Showtime::create_plug<ZstFloatPlug>("test_performer_2", "input_float_plug", "test_instrument", PlugDir::IN_JACK);
    input_float_plug->attach_recv_callback(new TestFloatCallback());
    Showtime::connect_plugs(output_float_plug->get_address(), input_float_plug->get_address());
    output_float_plug->fire(27.5f);
    
    ZstIntListPlug *output_intlist_plug = Showtime::create_plug<ZstIntListPlug>("test_performer_1", "output_intlist_plug", "test_instrument", PlugDir::OUT_JACK);
    ZstIntListPlug *input_intlist_plug = Showtime::create_plug<ZstIntListPlug>("test_performer_2", "input_intlist_plug", "test_instrument", PlugDir::IN_JACK);
    input_intlist_plug->attach_recv_callback(new TestIntListCallback());
    Showtime::connect_plugs(output_intlist_plug->get_address(), input_intlist_plug->get_address());
    output_intlist_plug->fire(std::vector<int>{27});
    
    ZstFloatListPlug *output_floatlist_plug = Showtime::create_plug<ZstFloatListPlug>("test_performer_1", "output_floatlist_plug", "test_instrument", PlugDir::OUT_JACK);
    ZstFloatListPlug *input_floatlist_plug = Showtime::create_plug<ZstFloatListPlug>("test_performer_2", "input_floatlist_plug", "test_instrument", PlugDir::IN_JACK);
    input_floatlist_plug->attach_recv_callback(new TestFloatListCallback());
    Showtime::connect_plugs(output_floatlist_plug->get_address(), input_floatlist_plug->get_address());
    output_floatlist_plug->fire(std::vector<float>{27.5f});
    
    ZstStringPlug *output_string_plug = Showtime::create_plug<ZstStringPlug>("test_performer_1", "output_string_plug", "test_instrument", PlugDir::OUT_JACK);
    ZstStringPlug *input_string_plug = Showtime::create_plug<ZstStringPlug>("test_performer_2", "input_string_plug", "test_instrument", PlugDir::IN_JACK);
    input_string_plug->attach_recv_callback(new TestStringCallback());
    Showtime::connect_plugs(output_string_plug->get_address(), input_string_plug->get_address());
    output_string_plug->fire(std::string("Twenty seven"));
}


void test_cleanup() {
	//Test object destruction
	delete stage;
	Showtime::instance().destroy();
}

int main(int argc,char **argv){
    stage = ZstStage::create_stage();
    
    Showtime::join( "127.0.0.1");
	ZstPerformer * performer_a = Showtime::create_performer("test_performer_1");
	ZstPerformer * performer_b = Showtime::create_performer("test_performer_2");
    
	test_performer_init();
    test_stage_registration();
    test_create_plugs();
	test_connect_plugs();
	test_cleanup();

	std::cout << "Shutting down..." << std::endl;

	std::cout << "Performer test completed" << std::endl;

#ifdef WIN32
	system("pause");
#endif

	return 0;
}




