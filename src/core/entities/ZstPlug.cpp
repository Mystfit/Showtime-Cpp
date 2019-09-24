#include <memory>
#include <mutex>
#include <msgpack.hpp>

#include "entities/ZstPlug.h"
#include "ZstCable.h"

#include "../ZstValue.h"
#include "../ZstEventDispatcher.hpp"
#include "../adaptors/ZstGraphTransportAdaptor.hpp"
#include "../ZstHierarchy.h"

namespace showtime
{

//--------------------
// ZstPlug
//--------------------

ZstPlug::ZstPlug() :
    ZstEntityBase(""),
    m_value(std::make_unique<ZstValue>()),
    m_direction(PlugDirection_NONE),
    m_max_connected_cables(-1)
{
    set_entity_type(EntityType_PLUG);
}

ZstPlug::ZstPlug(const char * name, ValueType t, PlugDirection direction, int max_cables) :
    ZstEntityBase(name),
    m_value(std::make_unique<ZstValue>(t)),
    m_direction(direction),
    m_max_connected_cables(max_cables)
{
    set_entity_type(EntityType_PLUG);
}

ZstPlug::ZstPlug(const ZstPlug & other) :
    ZstEntityBase(other),
    m_value(other.m_value.get()),
    m_direction(other.m_direction),
    m_max_connected_cables(other.m_max_connected_cables)
{
}

ZstPlug::~ZstPlug() {
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
    return m_value.get();
}


//--------------------
// Serialisation
//--------------------

void ZstPlug::serialize(flatbuffers::Offset<Plug> & serialized_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const
{
//    //Pack value
//    buffer["value"] = m_value->as_json();
//
    auto plug_builder = PlugBuilder(buffer_builder);
    plug_builder.add_plug_direction(m_direction);
    
    if(m_max_connected_cables >= 0)
        plug_builder.add_max_cables(m_max_connected_cables);
    
    flatbuffers::Offset<Entity> entity;
    ZstEntityBase::serialize(entity, buffer_builder);
    plug_builder.add_entity(entity);
    
    serialized_offset = plug_builder.Finish();
}

void ZstPlug::deserialize(const Plug* buffer)
{
    m_direction = buffer->plug_direction();
    m_max_connected_cables = buffer->max_cables();
    ZstEntityBase::deserialize(buffer->entity());
}


//--------------------
// Properties
//--------------------

PlugDirection ZstPlug::direction()
{
    return m_direction;
}

void ZstPlug::get_child_cables(ZstCableBundle & bundle)
{
    std::lock_guard<std::mutex> lock(m_entity_mtx);
    for (auto const & cable_path : m_cables) {
        m_session_events->invoke([&cable_path, &bundle](std::shared_ptr<ZstSessionAdaptor> adaptor){
            ZstCable* cable = NULL;
            cable = adaptor->find_cable(cable_path);
            
            if (!cable) {
                ZstLog::entity(LogLevel::error, "No cable found for address {}<-{}", cable_path.get_input_URI().path(), cable_path.get_output_URI().path());
                return;
            }

            // TODO: Replace bundles vectors with sets?
            for (auto existing_cable : bundle) {
                if (existing_cable->get_address() == cable_path)
                    return;
            }
            
            bundle.add(cable);
        });
    }

    ZstEntityBase::get_child_cables(bundle);
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
    m_cables.insert(cable->get_address());
}

void ZstPlug::remove_cable(ZstCable * cable)
{
    if(!cable)
        return;
    
    std::lock_guard<std::mutex> lock(m_entity_mtx);
    auto cable_it = m_cables.find(cable->get_address());
    if(cable_it != m_cables.end()){
        m_cables.erase(cable_it);
    }
}


//ZstInputPlug
//------------

ZstInputPlug::ZstInputPlug() :
    ZstPlug("", ValueType_NONE)
{
}

ZstInputPlug::ZstInputPlug(const ZstInputPlug & other) : ZstPlug(other)
{
}

ZstInputPlug::~ZstInputPlug()
{
}

ZstInputPlug::ZstInputPlug(const char * name, ValueType t, int max_cables) :
    ZstPlug(name, t, PlugDirection_IN_JACK, max_cables)
{
}


//ZstOutputPlug
//-------------

ZstOutputPlug::ZstOutputPlug() :
    ZstPlug("", ValueType_NONE, PlugDirection_OUT_JACK, -1),
    m_graph_out_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstGraphTransportAdaptor> > >("plug_out_events")),
    m_reliable(true),
    m_can_fire(false)
{
}

ZstOutputPlug::ZstOutputPlug(const ZstOutputPlug& other) :
    ZstPlug(other),
    m_graph_out_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstGraphTransportAdaptor> > > ("plug_out_events")),
    m_reliable(other.m_reliable),
    m_can_fire(other.m_can_fire)
{
}

ZstOutputPlug::ZstOutputPlug(const char * name, ValueType t, bool reliable) :
    ZstPlug(name, t, PlugDirection_OUT_JACK, -1),
    m_graph_out_events(std::make_shared< ZstEventDispatcher< std::shared_ptr<ZstGraphTransportAdaptor> > >("plug_out_events")),
    m_reliable(reliable),
    m_can_fire(false)
{
#ifndef ZST_BUILD_DRAFT_API
    if(!m_reliable){
        ZstLog::entity(LogLevel::warn, "Can't use plug {} in unreliable mode, Showtime not compiled with draft API support. Falling back to reliable.", name);
        m_reliable = true;
    }
#endif
}

ZstOutputPlug::~ZstOutputPlug()
{
    m_graph_out_events->flush();
    m_graph_out_events->remove_all_adaptors();
}

bool ZstOutputPlug::can_fire()
{
    return m_can_fire;
}

void ZstOutputPlug::fire()
{
    if (!can_fire())
        return;

    m_graph_out_events->invoke([](std::shared_ptr<ZstGraphTransportAdaptor> adaptor) {
        //ZstTransportArgs args;
        //args.msg_args[get_msg_arg_name(ZstMsgArg::PATH)] = this->URI().path();
        //this->raw_value()->write_json(args.msg_payload);
        //adaptor->send_msg(ZstMsgKind::PERFORMANCE_MSG, args);

        //TODO: Finish graph message flatbuffer implementation
        
        GraphMessageBuilder builder;
        adaptor->send_message(builder);
        
        static_assert(false);
    });
    m_value->clear();
}

bool ZstOutputPlug::is_reliable()
{
    return m_reliable;
}

void ZstOutputPlug::set_owner(const ZstURI & owner)
{
    ZstEntityBase::set_owner(owner);

    ZstPerformer* performer = NULL;
    session_events()->invoke([this, &performer](std::shared_ptr<ZstSessionAdaptor> adaptor){
        performer = adaptor->hierarchy()->get_local_performer();
    });

    if (!performer) {
        set_can_fire(false);
        return;
    }

    if (performer->URI() == owner) {
        set_can_fire(true);
    }
    else {
        set_can_fire(false);
    }
}

void ZstOutputPlug::set_can_fire(bool can_fire)
{
    m_can_fire = can_fire;
}

void ZstOutputPlug::add_adaptor(std::shared_ptr<ZstTransportAdaptor> & adaptor)
{
    m_graph_out_events->add_adaptor(adaptor);
}

void ZstOutputPlug::remove_adaptor(std::shared_ptr<ZstTransportAdaptor> & adaptor)
{
    m_graph_out_events->remove_adaptor(adaptor);
}

}
