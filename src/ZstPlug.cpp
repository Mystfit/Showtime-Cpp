#include "ZstPlug.h"
#include "Showtime.h"
#include "ZstEndpoint.h"
#include "ZstValue.h"
#include "ZstValueWire.h"
#include "entities/ZstEntityBase.h"
#include "entities/ZstComponent.h"

using namespace std;

ZstPlug::ZstPlug(ZstComponent * owner, const char * name, ZstValueType t) : ZstEntityBase()
{
    m_value = new ZstValue(t);
    m_uri = ZstURI(name);
    set_parent(owner);
}

ZstPlug::~ZstPlug() {
	delete m_value;
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

ZstInputPlug::ZstInputPlug(ZstComponent * owner, const char * name, ZstValueType t) : ZstPlug(owner, name, t)
{
    m_direction = PlugDirection::IN_JACK;
}

//ZstInputPlug
//------------
ZstInputPlug::~ZstInputPlug()
{
}

void ZstInputPlug::recv(const ZstValue & val) {
    //TODO:Lock plug value when copying
    m_value->clear();
	m_value->copy(val);
}

ZstOutputPlug::ZstOutputPlug(ZstComponent * owner, const char * name, ZstValueType t) : ZstPlug(owner, name, t)
{
    m_direction = PlugDirection::OUT_JACK;
}

//ZstOutputPlug
//-------------
void ZstOutputPlug::fire()
{
	Showtime::endpoint().send_to_graph(ZstMessages::build_graph_message(this->URI(), ZstValueWire(*m_value)));
	m_value->clear();
}
