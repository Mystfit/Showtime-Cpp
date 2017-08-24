#include "ZstPlug.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include "ZstEvent.h"
#include "ZstValueWire.h"
#include "entities/ZstComponent.h"

using namespace std;

ZstPlug::ZstPlug(ZstComponent * entity, const char * name, ZstValueType t) :
	m_value(t),
    m_owner(entity),
	m_uri(ZstURI::join(entity->URI(), ZstURI(name))),
    m_is_destroyed(false)
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

bool ZstPlug::is_destroyed()
{
	return m_is_destroyed;
}

ZstInputPlug::ZstInputPlug(ZstComponent * entity, const char * name, ZstValueType t) : ZstPlug(entity, name, t)
{
	m_input_fired_manager = new ZstCallbackQueue<ZstPlugDataEventCallback, ZstInputPlug*>();
}

//ZstInputPlug
//------------
ZstInputPlug::~ZstInputPlug() {
	delete m_input_fired_manager;
}

void ZstInputPlug::recv(ZstValue & val) {
    m_value.clear();
    for(int i = 0; i < val.size(); ++i){
        m_value.append_variant(val.variant_at(i));
    }
}

void ZstInputPlug::attach_receive_callback(ZstPlugDataEventCallback * callback)
{
	m_input_fired_manager->attach_event_callback(callback);
}

void ZstInputPlug::remove_receive_callback(ZstPlugDataEventCallback * callback)
{
	m_input_fired_manager->remove_event_callback(callback);
}


//ZstOutputPlug
//-------------
void ZstOutputPlug::fire()
{
	Showtime::endpoint().send_to_graph(ZstMessages::build_graph_message(this->get_URI(), ZstValueWire(m_value)));
	m_value.clear();
}


//ZstPlugEvent
//-------------
ZstPlugEvent::ZstPlugEvent(ZstURI uri, ZstValue value) : ZstEvent(uri, ZstEvent::EventType::PLUG_HIT)
{
    m_value.clear();
    for(int i = 0; i < value.size(); ++i){
        m_value.append_variant(value.variant_at(i));
    }
}

ZstPlugEvent::~ZstPlugEvent()
{
}

ZstValue & ZstPlugEvent::value()
{
	return m_value;
}
