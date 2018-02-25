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
		ZstLog::entity(LogLevel::notification, "Sink received code {}", request_code);

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


int main(int argc,char **argv){

	zst_init("sink", true);

	if(argc < 2){
		ZstLog::app(LogLevel::warn, "Skipping sink test, command line flag not set");
		return 0;
	}

	ZstLog::app(LogLevel::notification, "In sink process");

	if(argv[1][0] == 'd')
#ifdef WIN32
		system("pause");
#else
        system("read -n 1 -s -p \"Press any key to continue...\n\"");
#endif

	
    zst_join("127.0.0.1");

	Sink * sink = new Sink("sink_ent");
	zst_activate_entity(sink);
	
	while (sink->last_received_code > 0){
		zst_poll_once();
	}
	
	ZstLog::app(LogLevel::notification, "Sink is leaving");
	zst_destroy();
	return 0;
}
