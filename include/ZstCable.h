#pragma once
#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstStreamable.h"
#include "entities/ZstPlug.h"

class ZstCable : public ZstStreamable {
public:
	friend class ZstClient;
	
	ZST_EXPORT ZstCable();
	ZST_EXPORT ZstCable(const ZstCable & copy);
	ZST_EXPORT ZstCable(ZstPlug * input_plug, ZstPlug * output_plug);
	ZST_EXPORT ~ZstCable();

	ZST_EXPORT bool operator==(const ZstCable & other);
	ZST_EXPORT bool operator!=(const ZstCable & other);
	ZST_EXPORT bool is_attached(const ZstURI & uri) const;
	ZST_EXPORT bool is_attached(const ZstURI & uriA, const ZstURI & uriB) const;
	ZST_EXPORT bool is_attached(ZstPlug * plug) const;
	ZST_EXPORT bool is_attached(ZstPlug * plugA, ZstPlug * plugB) const;
	ZST_EXPORT bool is_activated();

	ZST_EXPORT ZstPlug * get_input();
	ZST_EXPORT ZstPlug * get_output();
	ZST_EXPORT const ZstURI & get_input_URI() const;
	ZST_EXPORT const ZstURI & get_output_URI() const;
	ZST_EXPORT void unplug();
	
	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;
protected:
	ZstPlug * m_input;
	ZstPlug * m_output;
private:
	//Cached URI
	ZstURI m_input_URI;
	ZstURI m_output_URI;

	ZST_EXPORT void set_activated();
	bool m_is_activated;
};
