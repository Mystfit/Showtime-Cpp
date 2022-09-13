#include <memory>
#include <mutex>
#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>
#include <showtime/entities/ZstPlug.h>
#include <showtime/ZstCable.h>

#include "../ZstValue.h"
#include "../ZstEventDispatcher.hpp"
#include "../adaptors/ZstGraphTransportAdaptor.hpp"
#include "../ZstHierarchy.h"

using namespace flatbuffers;

namespace showtime
{

typedef boost::bimap<ZstPlugDirection, PlugDirection> FlatbuffersPlugDirectionMap;
static const FlatbuffersPlugDirectionMap plug_direction_lookup = boost::assign::list_of< FlatbuffersPlugDirectionMap::relation >
    (ZstPlugDirection::NONE, PlugDirection_NONE)
    (ZstPlugDirection::IN_JACK, PlugDirection_IN_JACK)
    (ZstPlugDirection::OUT_JACK, PlugDirection_OUT_JACK);

//--------------------
// ZstPlug
//--------------------

ZstPlug::ZstPlug() :
    ZstEntityBase(),
    m_value(nullptr),
    m_direction(ZstPlugDirection::NONE),
    m_reliable(true),
    m_max_connected_cables(PLUG_DEFAULT_MAX_CABLES)
{
    //init_value();
    set_entity_type(ZstEntityType::PLUG);
}

ZstPlug::ZstPlug(const char * name, const ZstValueType& t, const ZstPlugDirection& direction, int max_cables, bool reliable) :
    ZstEntityBase(name),
    m_value(std::make_unique<ZstDynamicValue>(t)),
    m_direction(direction),
    m_reliable(reliable),
    m_max_connected_cables(max_cables)
{
    //init_value(t);
    set_entity_type(ZstEntityType::PLUG);
}
    
ZstPlug::ZstPlug(const Plug* buffer) : 
	ZstEntityBase(),
    m_value(std::make_unique<ZstDynamicValue>()),
	m_direction(ZstPlugDirection::NONE),
    m_reliable(true),
	m_max_connected_cables(PLUG_DEFAULT_MAX_CABLES)
{
    //init_value();
	set_entity_type(ZstEntityType::PLUG);
	ZstEntityBase::deserialize_partial(buffer->entity());
    ZstPlug::deserialize_partial(buffer->plug());
}


ZstPlug::ZstPlug(const ZstPlug & other) :
    ZstEntityBase(other),
    m_value(other.m_value.get()),
    m_direction(other.m_direction),
    m_reliable(other.m_reliable),
    m_max_connected_cables(other.m_max_connected_cables)
{
}

void ZstPlug::init_value()
{
    m_value = std::make_unique<ZstDynamicValue>();
}

void ZstPlug::init_value(const ZstValueType& val_type)
{
    m_value = std::make_unique<ZstDynamicValue>(val_type);
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

void ZstPlug::append_string(const char * value, const size_t size)
{
    m_value->append_string(value, size);
}

void ZstPlug::append_byte(uint8_t value)
{
    m_value->append_byte(value);
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

const char* ZstPlug::string_at(const size_t position, size_t& out_str_size) const
{
    return m_value->string_at(position, out_str_size);
}

const uint8_t ZstPlug::byte_at(const size_t position) const
{
    return m_value->byte_at(position);
}

const size_t ZstPlug::size_at(const size_t position) const
{
    return m_value->size_at(position);
}

void ZstPlug::swap_values(ZstPlug* other)
{
    m_value.swap(other->m_value);
}

ZstIValue * ZstPlug::raw_value()
{
    return m_value.get();
}

ZstValueType ZstPlug::get_default_type() const
{
    return m_value->get_default_type();
}


//--------------------
// Serialisation
//--------------------

flatbuffers::uoffset_t ZstPlug::serialize(FlatBufferBuilder& buffer_builder) const
{
    Offset<PlugData> plug_offset;
    serialize_partial(plug_offset, buffer_builder);
    
	Offset<EntityData> entity_offset;
	ZstEntityBase::serialize_partial(entity_offset, buffer_builder);
	return CreatePlug(buffer_builder, entity_offset, plug_offset).o;
}
    
void ZstPlug::serialize_partial(flatbuffers::Offset<PlugData> & serialized_offset, flatbuffers::FlatBufferBuilder& buffer_builder) const
{
    serialized_offset = CreatePlugData(buffer_builder, 
        plug_direction_lookup.left.at(m_direction), 
        m_reliable,
        m_max_connected_cables, 
        m_value->serialize(buffer_builder)
    );
}
    
void ZstPlug::deserialize_partial(const PlugData* buffer)
{
    if (!buffer) return;

	m_value->deserialize(buffer->value());
    m_direction = plug_direction_lookup.right.at(buffer->plug_direction());
    m_reliable = buffer->reliable();
    m_max_connected_cables = buffer->max_cables();
}

void ZstPlug::deserialize(const Plug* buffer)
{
    ZstPlug::deserialize_partial(buffer->plug());
    ZstEntityBase::deserialize_partial(buffer->entity());
};


//--------------------
// Properties
//--------------------

ZstPlugDirection ZstPlug::direction()
{
    return m_direction;
}

bool ZstPlug::is_reliable()
{
    return m_reliable;
}

void ZstPlug::get_child_cables(ZstCableBundle* bundle)
{
    std::lock_guard<std::mutex> lock(m_entity_mtx);
    for (auto const & cable_path : m_cables) {
        m_session_events->invoke([&cable_path, &bundle](ZstSessionAdaptor* adaptor){
            ZstCable* cable = NULL;
            cable = adaptor->find_cable(cable_path);
            
            if (!cable) {
                return;
            }

            // TODO: Replace bundles vectors with sets?
            for (auto existing_cable : *bundle) {
                if (existing_cable->get_address() == cable_path)
                    return;
            }
            
            bundle->add(cable);
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
    ZstPlug("", ZstValueType::NONE),
    m_triggers_compute(false)
{
}
    
ZstInputPlug::ZstInputPlug(const Plug* buffer) : 
    ZstPlug(buffer),
    m_triggers_compute(false)
{
}

ZstInputPlug::ZstInputPlug(const ZstInputPlug & other) : 
    ZstPlug(other),
    m_triggers_compute(false)
{
}

ZstInputPlug::ZstInputPlug(const char * name, const ZstValueType& t, int max_cables, bool triggers_compute, bool reliable) :
    ZstPlug(name, t, ZstPlugDirection::IN_JACK, max_cables, reliable),
    m_triggers_compute(triggers_compute)
{
}

ZstInputPlug::~ZstInputPlug()
{
}

ZstCable* ZstInputPlug::connect_cable(ZstOutputPlug* output_plug)
{
    ZstCable* cable = nullptr;
    session_events()->invoke([&cable, this, output_plug](ZstSessionAdaptor* adp) {
        auto new_cable = adp->connect_cable(this, output_plug, ZstTransportRequestBehaviour::SYNC_REPLY);
        if (new_cable)
            cable = new_cable;
    });
    return cable;
}

ZstCable* ZstInputPlug::connect_cable_async(ZstOutputPlug* output_plug)
{
    ZstCable* cable = nullptr;
    session_events()->invoke([&cable, this, output_plug](ZstSessionAdaptor* adp) {
        auto new_cable = adp->connect_cable(this, output_plug, ZstTransportRequestBehaviour::ASYNC_REPLY);
        if (new_cable)
            cable = new_cable;
    });
    return cable;
}

bool ZstInputPlug::triggers_compute()
{
    return m_triggers_compute;
}


//ZstOutputPlug
//-------------

ZstOutputPlug::ZstOutputPlug() :
    ZstPlug("", ZstValueType::NONE, ZstPlugDirection::OUT_JACK, -1),
    m_graph_out_events(std::make_shared< ZstEventDispatcher<ZstGraphTransportAdaptor> >()),
    m_can_fire(false)
{
}
    
ZstOutputPlug::ZstOutputPlug(const Plug* buffer) : 
	ZstPlug(buffer),
	m_graph_out_events(std::make_shared< ZstEventDispatcher<ZstGraphTransportAdaptor> >()),
	m_can_fire(false)
{
}


ZstOutputPlug::ZstOutputPlug(const ZstOutputPlug& other) :
    ZstPlug(other),
    m_graph_out_events(std::make_shared< ZstEventDispatcher<ZstGraphTransportAdaptor> > ()),
    m_can_fire(other.m_can_fire)
{
}

ZstOutputPlug::ZstOutputPlug(const char * name, const ZstValueType& t, bool reliable) :
    ZstPlug(name, t, ZstPlugDirection::OUT_JACK, -1, reliable),
    m_graph_out_events(std::make_shared< ZstEventDispatcher<ZstGraphTransportAdaptor> >()),
    m_can_fire(false)
{
#ifndef ZST_BUILD_DRAFT_API
    if(!m_reliable){
        Log::entity(Log::Level::warn, "Can't use plug {} in unreliable mode, Showtime not compiled with draft API support. Falling back to reliable.", name);
        m_reliable = true;
    }
#endif
}

ZstOutputPlug::~ZstOutputPlug()
{
    //m_graph_out_events->flush();
    m_graph_out_events->remove_all_adaptors();
}

ZstCable* ZstOutputPlug::connect_cable(ZstInputPlug* input_plug)
{
    ZstCable* cable = nullptr;
    session_events()->invoke([&cable, this, input_plug](ZstSessionAdaptor* adp) {
        auto new_cable = adp->connect_cable(input_plug, this, ZstTransportRequestBehaviour::SYNC_REPLY);
        if (new_cable)
            cable = new_cable;
    });
    return cable;
}

ZstCable* ZstOutputPlug::connect_cable_async(ZstInputPlug* input_plug)
{
    ZstCable* cable = nullptr;
    session_events()->invoke([&cable, this, input_plug](ZstSessionAdaptor* adp) {
        auto new_cable = adp->connect_cable(input_plug, this, ZstTransportRequestBehaviour::ASYNC_REPLY);
        if (new_cable)
            cable = new_cable;
    });
    return cable;
}

void ZstOutputPlug::on_activation()
{
    ZstEntityBase::on_activation();

    // Set local plugs as fireable by default
    if (!this->is_proxy()) {
        set_can_fire(true);
    }
}

bool ZstOutputPlug::can_fire()
{
    return m_can_fire;
}

void ZstOutputPlug::fire()
{
    if (!can_fire())
        return;

    // Send message to local plugs first
    ZstCableBundle bundle;
    get_child_cables(&bundle);
    int num_local_cables = 0;
    for (auto c : bundle) {
        auto input_plug = c->get_input();

        // Cable is local - this component can execute immediately
        if (input_plug->URI().first() == this->URI().first()) {
            
            // If we have more than one cable, we have to copy the value instead of swapping it.
            // If the types don't match - conversion needs to bne applied during the copy
            // TODO: Replace swap with passing a ZstValue pointer along instead
            if (bundle.size() > 1 || input_plug->raw_value()->get_default_type() != this->raw_value()->get_default_type())
                input_plug->raw_value()->copy(this->raw_value());
            else
                this->swap_values(input_plug);

            // TODO: Optionally queue plug compute event?
           /* session_events()->invoke([&c](ZstSessionAdaptor* adp) {
                adp->plug_received_value(c->get_input());
            });*/

            // Find the parent component of the input plug
            /*ZstComponent* parent_component = nullptr;
            hierarchy_events()->invoke([c, &parent_component](ZstHierarchyAdaptor* adp) {
                if (auto p = adp->find_entity(c->get_address().get_input_URI().parent())) {
                    if (p->entity_type() == ZstEntityType::COMPONENT) {
                        parent_component = static_cast<ZstComponent*>(p);
                    }
                }
            });*/

            // TODO: This needs to be moved out of fire - seperate plug data from computation
            /*if(parent_component)
                parent_component->compute(input_plug);*/

            num_local_cables++;
        }
    }

    // Publish message to any remaining remote plugs
    if (num_local_cables <= bundle.size()){
        m_graph_out_events->invoke([this](ZstGraphTransportAdaptor* adaptor) {
            auto builder = std::make_shared<flatbuffers::FlatBufferBuilder>();
            auto plugval_offset = this->raw_value()->serialize(*builder);
            auto graph_msg_offset = CreateGraphMessage(*builder, builder->CreateString(this->URI().path()), plugval_offset);
            adaptor->send_msg(graph_msg_offset, builder);
            });
    }
    m_value->clear();
}

void ZstOutputPlug::set_owner(const ZstURI & owner)
{
    ZstEntityBase::set_owner(owner);

    ZstPerformer* performer = NULL;
    hierarchy_events()->invoke([&performer](ZstHierarchyAdaptor* adaptor){
        performer = adaptor->get_local_performer();
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

void ZstOutputPlug::add_adaptor(std::shared_ptr<ZstGraphTransportAdaptor> & adaptor)
{
    m_graph_out_events->add_adaptor(adaptor);
}

void ZstOutputPlug::remove_adaptor(std::shared_ptr<ZstGraphTransportAdaptor> & adaptor)
{
    m_graph_out_events->remove_adaptor(adaptor);
}

}


