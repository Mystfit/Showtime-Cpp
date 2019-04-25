#include <memory>
#include <mutex>
#include <msgpack.hpp>
#include <nlohmann/json.hpp>

#include "entities/ZstPlug.h"
#include "ZstCable.h"

#include "../ZstValue.h"
#include "../ZstEventDispatcher.hpp"
#include "../adaptors/ZstTransportAdaptor.hpp"

using namespace std;

//--------------------
// ZstPlug 
//--------------------

ZstPlug::ZstPlug() :
	ZstEntityBase(""),
	m_max_connected_cables(-1)
{
	set_entity_type(PLUG_TYPE);
	m_value = new ZstValue(ZST_NONE);
}

ZstPlug::ZstPlug(const char * name, ZstValueType t) : 
	ZstEntityBase(name),
	m_max_connected_cables(-1)
{
	set_entity_type(PLUG_TYPE);
    m_value = new ZstValue(t);
}

ZstPlug::ZstPlug(const ZstPlug & other) : ZstEntityBase(other)
{
	m_value = new ZstValue(*other.m_value);
	m_direction = other.m_direction;
	m_max_connected_cables = other.m_max_connected_cables;
}

ZstPlug::~ZstPlug() {
	delete m_value;
	m_cables.clear();
}

void ZstPlug::on_deactivation()
{
	//If this plug is deactivated then cables will be going away soon
    std::lock_guard<std::mutex> lock(m_entity_mtx);
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

void ZstPlug::write_json(json & buffer) const
{
	//Pack entity
	ZstEntityBase::write_json(buffer);

	//Pack value
	buffer["value"] = m_value->as_json();

	//Pack plug direction
	buffer["plug_direction"] = m_direction;

	//Pack max cables
	buffer["max_connected_cables"] = m_max_connected_cables;
}

void ZstPlug::read_json(const json & buffer)
{
	//Unpack entity
	ZstEntityBase::read_json(buffer);

	//Unpack value
	m_value->read_json(buffer["value"]);

	//Unpack direction
	m_direction = buffer["plug_direction"];

	//Unpack max cables
	m_max_connected_cables = buffer["max_connected_cables"];
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
    std::lock_guard<std::mutex> lock(m_entity_mtx);
	for (auto const & c : m_cables) {
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

size_t ZstPlug::max_connected_cables()
{
	return m_max_connected_cables;
}

bool ZstPlug::is_connected_to(ZstPlug * plug)
{
	for (auto const & c : m_cables) {
        if((c.get_input_URI() == this->URI() && c.get_output_URI() ==  plug->URI()) ||
           (c.get_input_URI() == plug->URI() && c.get_output_URI() ==  this->URI())){
            return true;
        }
	}
	return false;
}

void ZstPlug::add_cable(ZstCable * cable)
{
    if(!cable)
        return;
    
    std::lock_guard<std::mutex> lock(m_entity_mtx);
	m_cables.insert(ZstCable(*cable));
}

void ZstPlug::remove_cable(ZstCable * cable)
{
    if(!cable)
        return;
    
    std::lock_guard<std::mutex> lock(m_entity_mtx);
    auto cable_it = m_cables.find(*cable);
    if(cable_it != m_cables.end()){
        m_cables.erase(cable_it);
    }
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

ZstInputPlug::ZstInputPlug(const char * name, ZstValueType t, int max_cables) : ZstPlug(name, t)
{
	m_direction = ZstPlugDirection::IN_JACK;
	m_max_connected_cables = max_cables;
}


//ZstOutputPlug
//-------------

ZstOutputPlug::ZstOutputPlug() : 
	ZstPlug(),
	m_performance_events(NULL)
{
	m_direction = ZstPlugDirection::OUT_JACK;
	m_performance_events = new ZstEventDispatcher<ZstTransportAdaptor*>("plug_out_events");
	m_max_connected_cables = -1;
}

ZstOutputPlug::ZstOutputPlug(const ZstOutputPlug & other) : ZstPlug(other)
{
	m_performance_events = new ZstEventDispatcher<ZstTransportAdaptor*>("plug_out_events");
	m_max_connected_cables = other.m_max_connected_cables;
}

ZstOutputPlug::ZstOutputPlug(const char * name, ZstValueType t, bool reliable) : 
	ZstPlug(name, t),
	m_reliable(reliable)
{
	m_direction = ZstPlugDirection::OUT_JACK;
	m_performance_events = new ZstEventDispatcher<ZstTransportAdaptor*>("plug_out_events");
	m_max_connected_cables = -1;
#ifndef ZST_BUILD_DRAFT_API
    if(!m_reliable){
        ZstLog::entity(LogLevel::warn, "Can't use plug {} in unreliable mode, Showtime not compiled with draft API support. Falling back to reliable.", name);
        m_reliable = true;
    }
#endif
}

ZstOutputPlug::~ZstOutputPlug()
{
	m_performance_events->flush();
	m_performance_events->remove_all_adaptors();
	delete m_performance_events;
}

void ZstOutputPlug::fire()
{
	m_performance_events->invoke([this](ZstTransportAdaptor * adaptor) {
		json val_json;
		this->raw_value()->write_json(val_json);
		adaptor->on_send_msg(ZstMsgKind::PERFORMANCE_MSG, { { get_msg_arg_name(ZstMsgArg::PATH), this->URI().path() } }, val_json);
	});
	m_value->clear();
}

bool ZstOutputPlug::is_reliable()
{
	return m_reliable;
}

//MSGPACK_ADD_ENUM(ZstPlugDirection);
