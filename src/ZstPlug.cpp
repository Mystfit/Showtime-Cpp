#include "ZstPlug.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include "ZstEvent.h"

using namespace std;

ZstPlug::ZstPlug(ZstURI * uri) : m_uri(uri)
{
	m_buffer = new msgpack::sbuffer();
	m_packer = new msgpack::packer<msgpack::sbuffer>(m_buffer);
}

ZstPlug::~ZstPlug() {
	delete m_buffer;
	delete m_packer;
	delete m_uri;
	for (vector<ZstEventCallback*>::iterator callback = m_received_data_callbacks.begin(); callback != m_received_data_callbacks.end(); ++callback) {
		delete *(callback);
	}
	m_received_data_callbacks.clear();
}

ZstURI * ZstPlug::get_URI() const
{
	return m_uri;
}

void ZstPlug::attach_recv_callback(ZstEventCallback *callback){
    m_received_data_callbacks.push_back(callback);
}

void ZstPlug::destroy_recv_callback(ZstEventCallback *callback){
    m_received_data_callbacks.erase(std::remove(m_received_data_callbacks.begin(), m_received_data_callbacks.end(), callback), m_received_data_callbacks.end());
    delete callback;
}

void ZstPlug::run_recv_callbacks(){
    if(m_received_data_callbacks.size() > 0){
        for (vector<ZstEventCallback*>::iterator callback = m_received_data_callbacks.begin(); callback != m_received_data_callbacks.end(); ++callback) {
			cout << "ZST: Running plug callback" << endl;
			(*callback)->run(ZstEvent(*(this->get_URI()), ZstEvent::EventType::PLUG_HIT));
        }
    }
}


void ZstPlug::fire()
{
	zmsg_t * msg = zmsg_new();

	//First frame is the address of the sender plug
	zmsg_addstr(msg, get_URI()->to_char());

	//Second frame is the plug payload
	zframe_t * payload  = zframe_new(m_buffer->data(), m_buffer->size());
	zmsg_append(msg, &payload);

	Showtime::endpoint().send_to_graph(msg);

	//Clear our buffer when finished
	m_buffer->clear();
}

// -----

void ZstIntPlug::fire(int value)
{
	m_packer->pack_int(value);
	ZstPlug::fire();
}

void ZstIntPlug::recv(msgpack::object object){
    int out;
    object.convert<int>(out);
    m_value = out;
	Showtime::endpoint().enqueue_plug_event(ZstEvent(*get_URI(), ZstEvent::EventType::PLUG_HIT));
}

int ZstIntPlug::get_value(){
    return m_value;
}
