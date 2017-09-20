#include "ZstPlug.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include "ZstEvent.h"
#include "ZstValue.h"
#include "ZstValueWire.h"
#include "entities/ZstComponent.h"

using namespace std;

ZstPlug::ZstPlug(ZstComponent * entity, const char * name, ZstValueType t) :
    m_owner(entity),
	m_uri((entity->URI() + ZstURI(name))),
    m_is_destroyed(false)
{
	m_value = new ZstValue(t);
}

ZstPlug::~ZstPlug() {
	delete m_value;
}

const ZstURI & ZstPlug::get_URI() const
{
	return m_uri;
}

const ZstEntityBase * ZstPlug::owner() const
{
	return m_owner;
}

bool ZstPlug::is_destroyed()
{
	return m_is_destroyed;
}

void ZstPlug::clear()
{
	m_value->clear();
}

void ZstPlug::append_int(int value)
{
	m_value->append_int(value);
}

void ZstPlug::append_float(float value)
{
	m_value->append_float(value);
}

void ZstPlug::append_char(const char * value)
{
	m_value->append_char(value);
}

const size_t ZstPlug::size() const
{
	return m_value->size();
}

const int ZstPlug::int_at(const size_t position) const
{
	return m_value->int_at(position);
}

const float ZstPlug::float_at(const size_t position) const
{
	return m_value->float_at(position);
}

void ZstPlug::char_at(char * buf, const size_t position) const
{
	return m_value->char_at(buf, position);
}

const size_t ZstPlug::size_at(const size_t position) const
{
	return m_value->size_at(position);
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

void ZstInputPlug::recv(ZstValue * val) {
    m_value->clear();
	m_value->copy(*val);
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
	Showtime::endpoint().send_to_graph(ZstMessages::build_graph_message(this->get_URI(), ZstValueWire(*m_value)));
	m_value->clear();
}
