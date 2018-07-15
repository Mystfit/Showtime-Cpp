#pragma once

#include <ZstExports.h>
#include <ZstConstants.h>
#include <ZstCable.h>
#include <entities/ZstEntityBase.h>

#define PLUG_TYPE "plug"

//Forward declarations
class ZstValue;
class ZstPlug;

template<typename T>
class ZstEventDispatcher;

class ZstPlugIterator {
public:
	ZST_EXPORT ZstPlugIterator(const ZstPlug * p, ZstCableList::iterator it);
	ZST_EXPORT bool operator!=(const ZstPlugIterator& other);
	ZST_EXPORT const ZstPlugIterator& operator++();
	ZST_EXPORT ZstCable * operator*() const;

private:
	const ZstPlug * m_plug;
	ZstCableList::iterator m_it;
};

class ZstPlug : public ZstEntityBase {
public:
	friend class ZstPlugLiason;
    friend class ZstComponent;
	friend class ZstPlugIterator;
    
	//Initialisation
	ZST_EXPORT ZstPlug();
	ZST_EXPORT ZstPlug(const char * name, ZstValueType t);
	ZST_EXPORT ZstPlug(const ZstPlug & other);
	ZST_EXPORT ~ZstPlug();

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
	ZST_EXPORT void write(std::stringstream & buffer) const override;
	ZST_EXPORT void read(const char * buffer, size_t length, size_t & offset) override;

	//Properties
	ZST_EXPORT ZstPlugDirection direction();

	//Cable enumeration
	ZST_EXPORT ZstPlugIterator begin();
	ZST_EXPORT ZstPlugIterator end();
	ZST_EXPORT size_t num_cables();
	ZST_EXPORT bool is_connected_to(ZstPlug * plug);
	ZST_EXPORT void disconnect_cables() override;

	//Values
	ZST_EXPORT ZstValue * raw_value();

protected:
	ZstValue * m_value;
	ZstPlugDirection m_direction;

private:
	ZST_EXPORT void add_cable(ZstCable * cable);
	ZST_EXPORT void remove_cable(ZstCable * cable);

	ZstCableList m_cables;
};


// --------------------
// Derived plug classes
// --------------------
class ZstInputPlug : public ZstPlug {
public:
	friend class ZstPlugLiason;
	ZST_EXPORT ZstInputPlug();
	ZST_EXPORT ZstInputPlug(const ZstInputPlug & other);
	ZST_EXPORT ZstInputPlug(const char * name, ZstValueType t);
	ZST_EXPORT ~ZstInputPlug();
};


class ZstOutputPlug : public ZstPlug {
	friend class ZstPlugLiason;
    using ZstSynchronisable::add_adaptor;
    using ZstSynchronisable::remove_adaptor;
public:
	ZST_EXPORT ZstOutputPlug();
	ZST_EXPORT ZstOutputPlug(const ZstOutputPlug & other);
	ZST_EXPORT ZstOutputPlug(const char * name, ZstValueType t);
	ZST_EXPORT ~ZstOutputPlug();
	ZST_EXPORT void fire();
};
