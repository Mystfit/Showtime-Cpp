#pragma once

#include "ZstExports.h"
#include "ZstConstants.h"
#include "ZstCable.h"
#include "entities/ZstEntityBase.h"

#include <set>
#include <mutex>

#define PLUG_TYPE "plug"

//Forward declarations
class ZstValue;
class ZstPlug;
class ZstTransportAdaptor;

template<typename T>
class ZstEventDispatcher;

class ZstPlug : public ZstEntityBase {
public:
	friend class ZstPlugLiason;
    friend class ZstComponent;
	friend class ZstPlugIterator;
    
	//Initialisation
	ZST_EXPORT ZstPlug();
	ZST_EXPORT ZstPlug(const char * name, ZstValueType t);
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

	//Serialisation
	ZST_EXPORT virtual void write_json(json & buffer) const override;
	ZST_EXPORT virtual void read_json(const json & buffer) override;

	//Properties
	ZST_EXPORT ZstPlugDirection direction();

	//Cables
	ZST_EXPORT size_t num_cables();
	ZST_EXPORT size_t max_connected_cables();
	ZST_EXPORT bool is_connected_to(ZstPlug * plug);
	ZST_EXPORT virtual void get_child_cables(ZstCableBundle & bundle) override;

	//Values
	ZST_EXPORT ZstValue * raw_value();

protected:
	ZstValue * m_value;
	ZstPlugDirection m_direction;
	size_t m_max_connected_cables;

private:
	ZST_EXPORT void add_cable(ZstCable * cable);
	ZST_EXPORT void remove_cable(ZstCable * cable);
    
    std::set<ZstCableAddress> m_cables;
};


// --------------------
// Derived plug classes
// --------------------
class ZstInputPlug : public ZstPlug {
public:
	friend class ZstPlugLiason;
	ZST_EXPORT ZstInputPlug();
	ZST_EXPORT ZstInputPlug(const ZstInputPlug & other);
	ZST_EXPORT ZstInputPlug(const char * name, ZstValueType t, int max_connected_cables = -1);
	ZST_EXPORT ~ZstInputPlug();
};


class ZstOutputPlug : public ZstPlug {
	friend class ZstPlugLiason;
public:
    using ZstEntityBase::add_adaptor;
    using ZstEntityBase::remove_adaptor;
    
	ZST_EXPORT ZstOutputPlug();
	ZST_EXPORT ZstOutputPlug(const ZstOutputPlug & other);
	ZST_EXPORT ZstOutputPlug(const char * name, ZstValueType t, bool reliable = true);
	ZST_EXPORT ~ZstOutputPlug();

	ZST_EXPORT void fire();
	ZST_EXPORT bool is_reliable();
	ZST_EXPORT const ZstURI & get_fire_control_owner();
	ZST_EXPORT void aquire_fire_control();
	ZST_EXPORT void release_fire_control();
private:
    ZST_EXPORT virtual void add_adaptor(ZstTransportAdaptor* adaptor);
    ZST_EXPORT virtual void remove_adaptor(ZstTransportAdaptor* adaptor);
    
	void set_fire_control_owner(const ZstURI & fire_owner);
	ZstEventDispatcher<ZstTransportAdaptor*> * m_graph_out_events;

	bool m_reliable;
	ZstURI m_fire_control_owner;
};
