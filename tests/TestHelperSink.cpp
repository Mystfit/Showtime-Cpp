#include <iostream>
#include <chrono>
#include <thread>

#include "TestCommon.hpp"

std::shared_ptr<ShowtimeClient> test_client;


class Sink : public ZstComponent {
private:
    std::unique_ptr<ZstInputPlug> m_input;
	std::unique_ptr<ZstOutputPlug> m_output;

public:
	int last_received_code;
	Sink * m_child_sink;

	Sink(const char * name) : 
		ZstComponent("SINK", name),
        m_input(std::make_unique<ZstInputPlug>("in", ValueList_IntList)),
		m_output(std::make_unique<ZstOutputPlug>("out", ValueList_FloatList)),
        last_received_code(0),
		m_child_sink(NULL)
	{
	}

	~Sink() {
		//Plug is automatically deleted by owning component
		m_input = NULL;
		m_child_sink = NULL;
	}

	virtual void on_registered() override {
		add_child(m_input.get());
		add_child(m_output.get());
	}
	
	virtual void compute(ZstInputPlug * plug) override {
		ZstLog::entity(LogLevel::debug, "In sink compute");
		int request_code = plug->int_at(0);
		ZstLog::entity(LogLevel::notification, "Sink received code {}. Echoing over output", request_code);
		m_output->append_int(request_code);
		m_output->fire();

		switch (request_code)
		{
        case 0:
            //No-op
            break;
		case 1:
			m_child_sink = new Sink("sinkB");
			this->add_child(m_child_sink);
			ZstLog::entity(LogLevel::debug, "Sink about to sync activate child entity", m_child_sink->URI().path());
			if (!m_child_sink->is_activated())
				throw std::runtime_error("Child entity failed to activate");
			ZstLog::entity(LogLevel::debug, "Finished sync activate");
			break;
		case 2:
			ZstLog::entity(LogLevel::debug, "Sink about to sync deactivate child entity", m_child_sink->URI().path());
			if (!m_child_sink)
				throw std::runtime_error("Child entity is null");

			if (!m_child_sink->is_activated())
				throw std::runtime_error("Child entity is not activated");
			
			test_client->deactivate_entity(m_child_sink);
			ZstLog::entity(LogLevel::debug, "Finished sync deactivate");
			delete m_child_sink;
			m_child_sink = NULL;
			break;
		case 3:
			throw std::runtime_error("Testing compute failure.");
        case 4:
            //No-op
            break;
		default:
			break;
		}

		last_received_code = request_code;
	}
};

int main(int argc,char **argv){

	ZstLog::app(LogLevel::notification, "In sink process");

	bool force_launch = false;
	if (!force_launch) {
		bool skip = true;
		for (int i = 0; i < argc; ++i) {
			if (strcmp(argv[i], "test") == 0) {
				skip = false;
			}
		}
		if (skip) {
			ZstLog::app(LogLevel::warn, "Skipping, 'test' command line flag not set");
			return 0;
		}
	}

	test_client = std::make_shared<ShowtimeClient>();
	test_client->init("TestHelperSink", true);
    test_client->auto_join_by_name(TEST_SERVER_NAME);

	Sink * sink = new Sink("sink_ent");
	test_client->get_root()->add_child(sink);
	
	while (sink->last_received_code >= 0){
		test_client->poll_once();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
    
	ZstLog::app(LogLevel::notification, "Sink is leaving");
	test_client->destroy();
	return 0;
}
