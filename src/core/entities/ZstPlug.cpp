#include <memory>
#include <msgpack.hpp>

#include <entities/ZstPlug.h>
#include <ZstCable.h>

#include "../ZstValue.h"
#include "../ZstINetworkInteractor.h"

using namespace std;

//--------------------
//Plug cables iterator
//--------------------

ZstCableIterator::ZstCableIterator(const ZstPlug * plug, unsigned idx) :
	m_plug(plug),
	m_index(idx)
{
}

bool ZstCableIterator::operator!=(const ZstCableIterator & other)
{
	return (m_index != other.m_index);
}

const ZstCableIterator & ZstCableIterator::operator++()
{
	m_index++;
	return *this;
}

ZstCable * ZstCableIterator::operator*() const
{
	ZstCable * result = NULL;
	if (m_plug->m_cables.size())
		result = m_plug->m_cables[m_index];
	return result;
}


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
	for (auto cable : m_cables) {
		cable->unplug(); 
		delete cable;
	}
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
	m_direction = (ZstPlugDirection)obj.via.i64;
}


//--------------------
// Properties
//--------------------

ZstPlugDirection ZstPlug::direction()
{
	return m_direction;
}


//--------------------
// Cable enerumeration
//--------------------

ZstCableIterator ZstPlug::begin() const
{
	return m_cables.size() > 0 ? ZstCableIterator(this, 0) : NULL;
}

ZstCableIterator ZstPlug::end() const
{
	return m_cables.size() > 0 ? ZstCableIterator(this, m_cables.size()) : NULL;
}

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

ZstCable * ZstPlug::cable_at(size_t index)
{
	return m_cables[index];
}

void ZstPlug::disconnect_cables()
{
	auto cables = m_cables;
	for (auto c : cables) {
		c->unplug();
		delete c;
	}
}

void ZstPlug::add_cable(ZstCable * cable)
{
	std::vector<ZstCable*>::iterator cable_it = std::find(m_cables.begin(), m_cables.end(), cable);
	if (cable_it != m_cables.end()) {
		return;
	}

	m_cables.push_back(cable);
}

void ZstPlug::remove_cable(ZstCable * cable)
{
	m_cables.erase(std::remove(m_cables.begin(), m_cables.end(), cable), m_cables.end());
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

ZstOutputPlug::ZstOutputPlug(const char * name, ZstValueType t) : ZstPlug(name, t)
{
    m_direction = ZstPlugDirection::OUT_JACK;
}

void ZstOutputPlug::fire()
{
	if(network_interactor())
		network_interactor()->publish(this);
	m_value->clear();
}

MSGPACK_ADD_ENUM(ZstPlugDirection);
