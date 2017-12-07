#include "entities/ZstPlug.h"
#include "ZstValue.h"
#include "entities/ZstEntityBase.h"
#include "entities/ZstComponent.h"
#include "ZstGraphSender.h"

using namespace std;

ZstPlug::ZstPlug() :
	ZstEntityBase(PLUG_TYPE)
{
	m_value = new ZstValue(ZST_NONE);
}

ZstPlug::ZstPlug(ZstComponent * owner, const char * name, ZstValueType t) : 
	ZstEntityBase(PLUG_TYPE, name)
{
    m_value = new ZstValue(t);
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


//ZstInputPlug
//------------
ZstInputPlug::~ZstInputPlug()
{
}

ZstInputPlug::ZstInputPlug(ZstComponent * owner, const char * name, ZstValueType t) : ZstPlug(owner, name, t)
{
	m_direction = ZstPlugDirection::IN_JACK;
}

ZstOutputPlug::ZstOutputPlug(ZstComponent * owner, const char * name, ZstValueType t) : ZstPlug(owner, name, t), m_sender(NULL)
{
    m_direction = ZstPlugDirection::OUT_JACK;
}

void ZstOutputPlug::register_graph_sender(ZstGraphSender * sender)
{
	m_sender = sender;
}

//ZstOutputPlug
//-------------
void ZstOutputPlug::fire()
{
	m_sender->send_to_graph(this);
	m_value->clear();
}

MSGPACK_ADD_ENUM(ZstPlugDirection);
