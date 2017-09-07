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
		m_input = create_input_plug("in", ZstValueType::ZST_INT);
	}

	virtual void compute(ZstInputPlug * plug) override {
		received_hit = true;
	}

	const ZstURI & input_URI() {
		return m_input->get_URI();
	}
};


int main(int argc,char **argv){

	if(argc < 2){
		std::cout << "Skipping test" << std::endl;
		return 0;
	}

	Showtime::init();
    Showtime::join("127.0.0.1");

	std::cout << "Starting sink" << std::endl;

    ZstComponent * root = new ZstComponent("ROOT", "sink_root");
	Sink * sink = new Sink("sink", root);
	
	while (!sink->received_hit){
		Showtime::poll_once();
	}

	delete sink;
    delete root;
	
	Showtime::destroy();
	return 0;
}
