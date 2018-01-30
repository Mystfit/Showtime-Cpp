#include "Showtime.h"
#include <iostream>

class Sink : public ZstContainer {
private:
    ZstInputPlug * m_input;

public:
	int last_received_code = 1;
	Sink * m_child_sink;

	~Sink() {
		//Plug is automatically deleted by owning component
		m_input = NULL;
		m_child_sink = NULL;
	}

	Sink(const char * name) : ZstContainer("SINK", name) {
		m_input = create_input_plug("in", ZstValueType::ZST_INT);
	}
	
	virtual void compute(ZstInputPlug * plug) override {
		int request_code = plug->int_at(0);
		LOGGER->debug("Sink received code {}", request_code);

		if (request_code == 1) {
			m_child_sink = new Sink("sinkB");
			add_child(m_child_sink);
			zst_activate_entity(m_child_sink);
		}
		else if (request_code == 2) {
			zst_deactivate_entity(m_child_sink);
			m_child_sink = NULL;
		}

		last_received_code = request_code;
	}
};


class TestCableArrivingEventCallback : public ZstCableEvent {
public:
	void run(ZstCable * cable) override {
		LOGGER->debug("CABLE EVENT: {}-{} arriving", cable->get_output()->URI().path(), cable->get_input()->URI().path());
	}
};


int main(int argc,char **argv){

	zst_log_init();
	LOGGER->set_level(spdlog::level::debug);

	if(argc < 2){
		LOGGER->warn("Skipping sink test, command line flag not set");
		return 0;
	}

	LOGGER->debug("In sink process");


#ifdef WIN32
	if(argv[1][0] == 'd')
		system("pause");
#endif

	zst_init("sink");
    zst_join("127.0.0.1");
	TestCableArrivingEventCallback * cable_arrive = new TestCableArrivingEventCallback();
    zst_attach_cable_event_listener(cable_arrive, ZstEventAction::ARRIVING);
	
	Sink * sink = new Sink("sink_ent");
	zst_activate_entity(sink);
	
	while (sink->last_received_code > 0){
		zst_poll_once();
	}

	
	LOGGER->debug("Sink is leaving");
	zst_destroy();


	LOGGER->debug("Exiting sink process");

	return 0;
}
