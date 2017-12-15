#include "entities/ZstPlug.h"
#include "entities/ZstEntityBase.h"
#include "entities/ZstComponent.h"
#include "../ZstValue.h"
#include "../ZstGraphSender.h"

using namespace std;

ZstPlug::ZstPlug() :
	ZstEntityBase(PLUG_TYPE)
{
	m_value = new ZstValue(ZST_NONE);
}

ZstPlug::ZstPlug(const char * name, ZstValueType t) : 
	ZstEntityBase(PLUG_TYPE, name)
{
    m_value = new ZstValue(t);
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

ZstValue * ZstPlug::raw_value()
{
	return m_value;
}

void ZstPlug::write(std::stringstream & buffer)
{
	//Pack entity
	ZstEntityBase::write(buffer);

	//Pack plug direction
	msgpack::pack(buffer, m_direction);
}

void ZstPlug::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack entity
	ZstEntityBase::read(buffer, length, offset);

	//Unpack direction
	auto handle = msgpack::unpack(buffer, length, offset);
	auto obj = handle.get();
	ZstPlugDirection m_direction = (ZstPlugDirection)obj.via.i64;
}

ZstPlugDirection ZstPlug::direction()
{
	return m_direction;
}


//ZstInputPlug
//------------
ZstInputPlug::~ZstInputPlug()
{
}

ZstInputPlug::ZstInputPlug(const char * name, ZstValueType t) : ZstPlug(name, t)
{
	m_direction = ZstPlugDirection::IN_JACK;
}


//ZstOutputPlug
//-------------

ZstOutputPlug::ZstOutputPlug(const char * name, ZstValueType t) : ZstPlug(name, t), m_sender(NULL)
{
    m_direction = ZstPlugDirection::OUT_JACK;
}

void ZstOutputPlug::register_graph_sender(ZstGraphSender * sender)
{
	m_sender = sender;
}

void ZstOutputPlug::fire()
{
	if(m_sender)
		m_sender->publish(this);
	m_value->clear();
}

MSGPACK_ADD_ENUM(ZstPlugDirection);
