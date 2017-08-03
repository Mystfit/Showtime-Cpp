#include "ZstPlug.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include "ZstEvent.h"
#include "ZstValueWire.h"
#include "entities/ZstFilter.h"

using namespace std;

ZstPlug::ZstPlug(ZstFilter * entity, const char * name, ZstValueType t) :
	m_owner(entity),
	m_value(t),
	m_uri(ZstURI::join(entity->URI(), ZstURI(name)))
{
}

ZstPlug::~ZstPlug() {
}


ZstURI ZstPlug::get_URI() const
{
	return m_uri;
}

ZstValue & ZstPlug::value()
{
	return m_value;
}

ZstInputPlug::ZstInputPlug(ZstFilter * entity, const char * name, ZstValueType t) : ZstPlug(entity, name, t)
{
	m_input_fired_manager = new ZstCallbackQueue<ZstPlugDataEventCallback, ZstInputPlug*>();
}

//ZstInputPlug
//------------
ZstInputPlug::~ZstInputPlug() {
	delete m_input_fired_manager;
}

void ZstInputPlug::recv(ZstValue val) {
	m_value = ZstValue(val);
	Showtime::endpoint().enqueue_event(ZstEvent(get_URI(), ZstEvent::EventType::PLUG_HIT));
}

ZstCallbackQueue<ZstPlugDataEventCallback, ZstInputPlug*> * ZstInputPlug::input_events()
{
	return m_input_fired_manager;
}


//ZstOutputPlug
//-------------
void ZstOutputPlug::fire()
{
	Showtime::endpoint().send_to_graph(ZstMessages::build_graph_message(this->get_URI(), ZstValueWire(m_value)));
	m_value.clear();
}
