#include <memory>
#include <msgpack.hpp>

#include <entities/ZstPlug.h>
#include <ZstCable.h>

#include "../ZstValue.h"
#include "../ZstEventDispatcher.hpp"

using namespace std;

//--------------------
// ZstPlug 
//--------------------

ZstPlug::ZstPlug() :
	ZstEntityBase("")
{
	set_entity_type(PLUG_TYPE);
	m_value = new ZstValue(ZST_NONE);
}

ZstPlug::ZstPlug(const char * name, ZstValueType t) : 
	ZstEntityBase(name)
{
	set_entity_type(PLUG_TYPE);
    m_value = new ZstValue(t);
}

ZstPlug::ZstPlug(const ZstPlug & other) : ZstEntityBase(other)
{
	m_value = new ZstValue(*other.m_value);
	m_direction = other.m_direction;
}

ZstPlug::~ZstPlug() {
	delete m_value;
	//Cables should have been cleared at this point
	//for (auto cable : m_cables) {
	//	cable->set_deactivated(); 
	//}
	m_cables.clear();
}

//--------------------
// Value interface 
//--------------------

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


//--------------------
// Serialisation
//--------------------

void ZstPlug::write(std::stringstream & buffer) const
{
	//Pack entity
	ZstEntityBase::write(buffer);

	//Unpack value
	m_value->write(buffer);

	//Pack plug direction
	msgpack::pack(buffer, m_direction);
}

void ZstPlug::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack entity
	ZstEntityBase::read(buffer, length, offset);
	
	//Unpack value
	m_value->read(buffer, length, offset);

	//Unpack direction
	auto handle = msgpack::unpack(buffer, length, offset);
	auto obj = handle.get();
	m_direction = (ZstPlugDirection)obj.via.i64;
}


//--------------------
// Properties
//--------------------

ZstPlugDirection ZstPlug::direction()
{
	return m_direction;
}

ZstCableBundle & ZstPlug::get_child_cables(ZstCableBundle & bundle) const
{
	//TODO: Replace with set?
	//Cables may already be present, so only add unique ones
	for (auto c : m_cables) {
		bool exists = false;
		for (auto cable : bundle) {
			if (c == cable)
				exists = true;
		}
		
		if (!exists) {
			bundle.add(c);
		}
	}
	return ZstEntityBase::get_child_cables(bundle);
}


//--------------------
// Cable enerumeration
//--------------------

size_t ZstPlug::num_cables()
{
	return m_cables.size();
}

bool ZstPlug::is_connected_to(ZstPlug * plug)
{
	bool result = false;
	for (auto c : m_cables) {
		if (c->is_attached(this, plug)) {
			result = true;
		}
	}
	return result;
}

void ZstPlug::add_cable(ZstCable * cable)
{
	m_cables.insert(cable);
}

void ZstPlug::remove_cable(ZstCable * cable)
{
	m_cables.erase(cable);
}


//ZstInputPlug
//------------

ZstInputPlug::ZstInputPlug() : ZstPlug()
{
	m_direction = ZstPlugDirection::IN_JACK;
}

ZstInputPlug::ZstInputPlug(const ZstInputPlug & other) : ZstPlug(other)
{
}

ZstInputPlug::~ZstInputPlug()
{
}

ZstInputPlug::ZstInputPlug(const char * name, ZstValueType t) : ZstPlug(name, t)
{
	m_direction = ZstPlugDirection::IN_JACK;
}


//ZstOutputPlug
//-------------

ZstOutputPlug::ZstOutputPlug() : ZstPlug()
{
	m_direction = ZstPlugDirection::OUT_JACK;
}

ZstOutputPlug::ZstOutputPlug(const ZstOutputPlug & other) : ZstPlug(other)
{
}

ZstOutputPlug::ZstOutputPlug(const char * name, ZstValueType t, bool reliable) : 
	ZstPlug(name, t),
	m_reliable(reliable)
{
	m_direction = ZstPlugDirection::OUT_JACK;
}

ZstOutputPlug::~ZstOutputPlug()
{
}

void ZstOutputPlug::fire()
{
    entity_events()->invoke([this](ZstEntityAdaptor * dlg) { 
		dlg->publish_entity_update(this);
	});
	m_value->clear();
}

bool ZstOutputPlug::is_reliable()
{
	return m_reliable;
}

MSGPACK_ADD_ENUM(ZstPlugDirection);
