#include "Showtime.h"
#include <iostream>

class Sink : public ZstComponent {
private:
    ZstInputPlug * m_input;

public:

	bool received_hit = false;

	Sink(const char * name) : ZstComponent("SINK", name) {
		init();
	}

	virtual void init() override {
		m_input = create_input_plug("in", ZstValueType::ZST_INT);
	}

	virtual void compute(ZstInputPlug * plug) override {
		std::cout << "Sink received plug hit." << std::endl;
		received_hit = true;
	}

	const ZstURI & input_URI() {
		return m_input->URI();
	}
};


class TestCableArrivingEventCallback : public ZstCableEvent {
public:
	void run(ZstCable * cable) override {
		std::cout << "SINK - cable arriving " << cable->get_output()->URI().path() << " to " << cable->get_input()->URI().path() << std::endl;
	}
};


int main(int argc,char **argv){

	if(argc < 2){
		std::cout << "Skipping test" << std::endl;
		return 0;
	}

	Showtime::init("sink");
    Showtime::join("127.0.0.1");
	TestCableArrivingEventCallback * cable_arrive = new TestCableArrivingEventCallback();
    Showtime::attach(cable_arrive, ZstCallbackAction::ARRIVING);

	std::cout << "Starting sink" << std::endl;

	Sink * sink = new Sink("sink_ent");
	Showtime::activate(sink);
	
	while (!sink->received_hit){
		Showtime::poll_once();
	}

	std::cout << "Removing sink entity" << std::endl;
	delete sink;

	Showtime::destroy();

	std::cout << "Exiting sink process" << std::endl;

	return 0;
}
