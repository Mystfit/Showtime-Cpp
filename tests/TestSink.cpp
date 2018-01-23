#include "Showtime.h"
#include <iostream>

class Sink : public ZstContainer {
private:
    ZstInputPlug * m_input;

public:
	bool received_hit = false;
	Sink * m_child_sink;

	Sink(const char * name) : ZstContainer("SINK", name) {
		init();
	}

	virtual void init() override {
		m_input = create_input_plug("in", ZstValueType::ZST_INT);
	}

	virtual void compute(ZstInputPlug * plug) override {
		LOGGER->debug("Sink received plug hit.");
		received_hit = true;

		int request_code = plug->int_at(0);
		if (request_code == 0) {
			received_hit = true;
		}
		else if (request_code == 1) {
			m_child_sink = new Sink("sinkB");
			add_child(m_child_sink);
			Showtime::activate(m_child_sink);
		}
		else if (request_code == 2) {
			Showtime::deactivate(m_child_sink);
			delete m_child_sink;
		}
	}
};


class TestCableArrivingEventCallback : public ZstCableEvent {
public:
	void run(ZstCable * cable) override {
		std::cout << "SINK - cable arriving " << cable->get_output()->URI().path() << " to " << cable->get_input()->URI().path() << std::endl;
	}
};


int main(int argc,char **argv){

	ZST_init_log();
	LOGGER->set_level(spdlog::level::debug);

	if(argc < 2){
		LOGGER->warn("Skipping sink test, command line flag not set");
		return 0;
	}

	LOGGER->debug("In sink process");

	Showtime::init("sink");
    Showtime::join("127.0.0.1");
	TestCableArrivingEventCallback * cable_arrive = new TestCableArrivingEventCallback();
    Showtime::attach_event_listener(cable_arrive, ZstEventAction::ARRIVING);


	Sink * sink = new Sink("sink_ent");
	Showtime::activate(sink);
	
	while (!sink->received_hit){
		Showtime::poll_once();
	}

	LOGGER->debug("Removing sink entity");
	Showtime::remove_event_listener(cable_arrive, ZstEventAction::ARRIVING);
	Showtime::destroy();

	delete sink;

	LOGGER->debug("Exiting sink process");

	return 0;
}
