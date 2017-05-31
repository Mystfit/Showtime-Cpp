#include "ZstPlug.h"
#include "Showtime.h"

using namespace std;

ZstPlug::ZstPlug(ZstURI uri) : m_uri(uri)
{
	m_buffer = new msgpack::sbuffer();
	m_packer = new msgpack::packer<msgpack::sbuffer>(m_buffer);
}

ZstPlug::~ZstPlug() {
	delete m_buffer;
	delete m_packer;
}

ZstURI ZstPlug::get_URI() const
{
	return m_uri;
}

void ZstPlug::attach_recv_callback(PlugCallback *callback){
    m_received_data_callbacks.push_back(callback);
}

void ZstPlug::destroy_recv_callback(PlugCallback *callback){
    m_received_data_callbacks.erase(std::remove(m_received_data_callbacks.begin(), m_received_data_callbacks.end(), callback), m_received_data_callbacks.end());
    delete callback;
}

void ZstPlug::run_recv_callbacks(){
    if(m_received_data_callbacks.size() > 0){
        for(vector<PlugCallback*>::iterator callback = m_received_data_callbacks.begin(); callback != m_received_data_callbacks.end(); ++callback){
            (*callback)->run(this);
        }
    }
}

void ZstPlug::fire()
{
	zmsg_t * msg = zmsg_new();

	//First frame is the address of the sender plug
	zmsg_addstr(msg, get_URI().to_str().c_str());

	//Second frame is the plug payload
	zframe_t * payload  = zframe_new(m_buffer->data(), m_buffer->size());
	zmsg_append(msg, &payload);

	Showtime::instance().send_to_graph(msg);

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
    run_recv_callbacks();
}

int ZstIntPlug::get_value(){
    return m_value;
}
