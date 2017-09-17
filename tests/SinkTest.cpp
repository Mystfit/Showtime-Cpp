#include "Showtime.h"
#include "entities/ZstComponent.h"


class Sink : public ZstComponent {
private:
    ZstInputPlug * m_input;

public:

	bool received_hit = false;

	Sink(const char * name, ZstEntityBase * parent) : ZstComponent("SINK", name, parent) {
		init();
	}

	virtual void init() override {
		activate();
		m_input = create_input_plug("in", ZstValueType::ZST_INT);
	}

	virtual void compute(ZstInputPlug * plug) override {
		std::cout << "Sink received plug hit." << std::endl;
		received_hit = true;
	}

	const ZstURI & input_URI() {
		return m_input->get_URI();
	}
};


class TestCableArrivingEventCallback : public ZstCableEventCallback {
public:
	void run(ZstCable cable) override {
		std::cout << "SINK - cable arriving " << cable.get_output().path() << " to " << cable.get_input().path() << std::endl;
	}
};


int main(int argc,char **argv){

	if(argc < 2){
		std::cout << "Skipping test" << std::endl;
		return 0;
	}

	Showtime::init();
    Showtime::join("127.0.0.1");
	TestCableArrivingEventCallback * cable_arrive = new TestCableArrivingEventCallback();
	Showtime::attach_cable_arriving_callback(cable_arrive);

	std::cout << "Starting sink" << std::endl;

    ZstComponent * root = new ZstComponent("ROOT", "sink_root");
	root->activate();
	Sink * sink = new Sink("sink", root);
	
	while (!sink->received_hit){
		Showtime::poll_once();
	}

	std::cout << "Removing sink root" << std::endl;
    delete root;

	std::cout << "Removing sink" << std::endl;
	delete sink;

	Showtime::remove_cable_arriving_callback(cable_arrive);
	delete cable_arrive;

	Showtime::destroy();

	std::cout << "Exiting sink process" << std::endl;

	return 0;
}
