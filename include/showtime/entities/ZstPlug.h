#pragma once

#include <showtime/ZstExports.h>
#include <showtime/ZstConstants.h>
#include <showtime/ZstCable.h>
#include <showtime/ZstConstants.h>
#include <showtime/entities/ZstEntityBase.h>

#include <set>
#include <mutex>
#include <memory>

namespace showtime {

//Forward declarations
class ZstValue;
class ZstGraphTransportAdaptor;

template<typename T>
class ZstEventDispatcher;

#define PLUG_DEFAULT_MAX_CABLES -1


enum ZST_CLASS_EXPORTED class ZstPlugDirection {
    NONE = 0,
    IN_JACK = 1,
    OUT_JACK = 2
};

class ZST_CLASS_EXPORTED ZstPlug :
#ifndef SWIG
	public virtual ZstSerialisable<Plug, PlugData>,
#endif
    public ZstEntityBase
{
public:
    friend class ZstPlugLiason;
    friend class ZstComponent;
    friend class ZstPlugIterator;
    
    //Initialisation
    ZST_EXPORT ZstPlug();
    ZST_EXPORT ZstPlug(const Plug* buffer);
    ZST_EXPORT ZstPlug(const char * name, const ZstValueType& t, const ZstPlugDirection& direction = ZstPlugDirection::NONE, int max_cables = -1);
    ZST_EXPORT ZstPlug(const ZstPlug & other);

    //Destruction
    ZST_EXPORT ~ZstPlug();
    ZST_EXPORT virtual void on_deactivation() override;

    //Value interface
    ZST_EXPORT void clear();
    ZST_EXPORT void append_int(int value);
    ZST_EXPORT void append_float(float value);
    ZST_EXPORT void append_char(const char * value);

    ZST_EXPORT const size_t size() const;
    ZST_EXPORT const int int_at(const size_t position) const;
    ZST_EXPORT const float float_at(const size_t position) const;
    ZST_EXPORT void char_at(char * buf, const size_t position) const;
    ZST_EXPORT const size_t size_at(const size_t position) const;

	// Serialisation
	// -------------
    using ZstEntityBase::serialize;
    using ZstEntityBase::deserialize;
    using ZstEntityBase::serialize_partial;
    using ZstEntityBase::deserialize_partial;
    ZST_EXPORT virtual void serialize_partial(flatbuffers::Offset<PlugData> & serialized_offset, flatbuffers::FlatBufferBuilder& buffer_builder) const override;
	ZST_EXPORT virtual flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder& buffer_builder) const override;
    ZST_EXPORT virtual void deserialize_partial(const PlugData* buffer) override;
	ZST_EXPORT virtual void deserialize(const Plug* buffer) override;

    //Properties
    ZST_EXPORT ZstPlugDirection direction();

    //Cables
    ZST_EXPORT size_t num_cables();
    ZST_EXPORT size_t max_connected_cables();
    ZST_EXPORT bool is_connected_to(ZstPlug * plug);
    ZST_EXPORT virtual void get_child_cables(ZstCableBundle* bundle) override;

    //Values
    ZST_EXPORT ZstValue * raw_value();
    ZST_EXPORT ZstValueType get_default_type() const;

protected:
    std::unique_ptr<ZstValue> m_value;
    ZstPlugDirection m_direction;
    int m_max_connected_cables;

private:
    ZST_EXPORT void add_cable(ZstCable * cable);
    ZST_EXPORT void remove_cable(ZstCable * cable);
    
    std::set<ZstCableAddress> m_cables;
};


// --------------------
// Derived plug classes
// --------------------
class ZST_CLASS_EXPORTED ZstInputPlug : public ZstPlug {
public:
    friend class ZstPlugLiason;
    ZST_EXPORT ZstInputPlug();
    ZST_EXPORT ZstInputPlug(const Plug* buffer);
    ZST_EXPORT ZstInputPlug(const ZstInputPlug & other);
    ZST_EXPORT ZstInputPlug(const char * name, const ZstValueType& t, int max_connected_cables = -1);
    ZST_EXPORT ~ZstInputPlug();
};


class ZST_CLASS_EXPORTED ZstOutputPlug : public ZstPlug {
    friend class ZstPlugLiason;
public:
    using ZstEntityBase::add_adaptor;
    using ZstEntityBase::remove_adaptor;
    
    ZST_EXPORT ZstOutputPlug();
    ZST_EXPORT ZstOutputPlug(const Plug* buffer);
    ZST_EXPORT ZstOutputPlug(const ZstOutputPlug & other);
    ZST_EXPORT ZstOutputPlug(const char * name, const ZstValueType& t, bool reliable = true);
    ZST_EXPORT ~ZstOutputPlug();
    ZST_EXPORT virtual void on_activation() override;
    ZST_EXPORT bool can_fire();
    ZST_EXPORT void fire();
    ZST_EXPORT bool is_reliable();
    
protected:
    ZST_EXPORT virtual void set_owner(const ZstURI & fire_owner) override;

private:
    ZST_EXPORT virtual void add_adaptor(std::shared_ptr<ZstGraphTransportAdaptor>& adaptor);
    ZST_EXPORT virtual void remove_adaptor(std::shared_ptr<ZstGraphTransportAdaptor>& adaptor);
    ZST_EXPORT void set_can_fire(bool can_fire);

    std::shared_ptr< ZstEventDispatcher<ZstGraphTransportAdaptor> > m_graph_out_events;

    bool m_reliable;
    bool m_can_fire;
};

}
