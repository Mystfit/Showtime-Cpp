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

ZstInputPlug::ZstInputPlug(ZstURI * uri, ZstValueType t) : ZstPlug(uri, t)
{
	m_input_fired_manager = new ZstCallbackQueue<ZstInputPlugEventCallback, ZstInputPlug*>();
}

//ZstInputPlug
//------------
ZstInputPlug::~ZstInputPlug() {
	delete m_input_fired_manager;
}

void ZstInputPlug::recv(ZstValue * val) {
	m_value = new ZstValue(*val);
	Showtime::endpoint().enqueue_event(ZstEvent(*get_URI(), ZstEvent::EventType::PLUG_HIT));
}

ZstCallbackQueue<ZstInputPlugEventCallback, ZstInputPlug*> * ZstInputPlug::input_events()
{
	return m_input_fired_manager;
}


//ZstOutputPlug
//-------------

void ZstOutputPlug::fire()
{
	Showtime::endpoint().send_to_graph(ZstMessages::build_graph_message(*(this->get_URI()), ZstValueWire(*m_value)));
}
