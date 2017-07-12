#include "ZstPlug.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include "ZstEvent.h"
#include "ZstValueWire.h"

using namespace std;

ZstPlug::ZstPlug(ZstURI * uri, ZstValueType t) : m_uri(uri)
{
	m_value = new ZstValue(t);
}

ZstPlug::~ZstPlug() {
	delete m_uri;
	delete m_value;
}

ZstURI * ZstPlug::get_URI() const
{
	return m_uri;
}

ZstValue * ZstPlug::value()
{
	return m_value;
}

//ZstInputPlug
//------------
ZstInputPlug::~ZstInputPlug() {
	for (vector<ZstInputPlugEventCallback*>::iterator callback = m_received_data_callbacks.begin(); callback != m_received_data_callbacks.end(); ++callback) {
		delete *(callback);
	}
	m_received_data_callbacks.clear();
}

void ZstInputPlug::recv(ZstValue * val) {
	m_value = new ZstValue(*val);
	Showtime::endpoint().enqueue_plug_event(ZstEvent(*get_URI(), ZstEvent::EventType::PLUG_HIT));
}

void ZstInputPlug::attach_recv_callback(ZstInputPlugEventCallback *callback){
    m_received_data_callbacks.push_back(callback);
}

void ZstInputPlug::destroy_recv_callback(ZstInputPlugEventCallback *callback){
    m_received_data_callbacks.erase(std::remove(m_received_data_callbacks.begin(), m_received_data_callbacks.end(), callback), m_received_data_callbacks.end());
    delete callback;
}

void ZstInputPlug::run_recv_callbacks(){
    for (auto callback : m_received_data_callbacks) {
        callback->run(this);
    }
}


//ZstOutputPlug
//-------------

void ZstOutputPlug::fire()
{
	Showtime::endpoint().send_to_graph(ZstMessages::build_graph_message(*(this->get_URI()), ZstValueWire(*m_value)));
}

ZstInputPlugEventCallback::ZstInputPlugEventCallback()
{
}
