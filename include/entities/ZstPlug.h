#pragma once

#include <string>
#include <vector>
#include <ZstExports.h>
#include <ZstConstants.h>
#include <ZstCable.h>
#include <entities/ZstEntityBase.h>

#define PLUG_TYPE "plug"

//Forward declarations
class ZstValue;
class ZstCable;
class ZstPlug;

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
	friend class ZstClient;
    friend class ZstComponent;
	friend class ZstPlugIterator;
	friend class ZstCable;
    
	//Initialisation
	ZST_EXPORT ZstPlug();
	ZST_EXPORT ZstPlug(const char * name, ZstValueType t);
	ZST_EXPORT ZstPlug(const ZstPlug & other);
	ZST_EXPORT virtual ~ZstPlug();
    
	ZST_EXPORT virtual void on_activated() override {};
	ZST_EXPORT virtual void on_deactivated() override {};

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
	ZST_EXPORT ZstValue * raw_value();

	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

	//Properties
	ZST_EXPORT ZstPlugDirection direction();

	//Cable enumeration
	ZST_EXPORT ZstPlugIterator begin() const;
	ZST_EXPORT ZstPlugIterator end() const;
	ZST_EXPORT size_t num_cables();
	ZST_EXPORT bool is_connected_to(ZstPlug * plug);
	ZST_EXPORT void disconnect_cables() override;

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
	friend class ZstClient;
	friend class Showtime;

	ZST_EXPORT ZstInputPlug(const char * name, ZstValueType t);
	ZST_EXPORT ~ZstInputPlug();
};


class ZstOutputPlug : public ZstPlug {
public:
	ZstOutputPlug(const char * name, ZstValueType t);
	ZST_EXPORT void fire();
};
