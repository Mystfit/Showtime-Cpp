#pragma once
#include <unordered_set>
#include <vector>
#include <ZstExports.h>
#include <ZstURI.h>
#include <ZstSerialisable.h>
#include <ZstSynchronisable.h>
#include <ZstEvents.h>

//Forwards
class ZstPlug;

class ZstCable : public ZstSerialisable, public ZstSynchronisable {
public:
	friend class ZstClient;

	ZST_EXPORT ZstCable();
	ZST_EXPORT ZstCable(const ZstCable & copy);
	ZST_EXPORT ZstCable(const ZstURI & input_plug_URI, const ZstURI & output_plug_URI);
	ZST_EXPORT ZstCable(ZstPlug * input_plug, ZstPlug * output_plug);
	ZST_EXPORT ~ZstCable();
	ZST_EXPORT virtual void on_activated() override {};
	ZST_EXPORT virtual void on_deactivated() override;
	ZST_EXPORT virtual void set_deactivated() override;

	// Status

	ZST_EXPORT bool operator==(const ZstCable & other) const;
	ZST_EXPORT bool operator!=(const ZstCable & other);
	ZST_EXPORT bool is_attached(const ZstURI & uri) const;
	ZST_EXPORT bool is_attached(const ZstURI & uriA, const ZstURI & uriB) const;
	ZST_EXPORT bool is_attached(ZstPlug * plug) const;
	ZST_EXPORT bool is_attached(ZstPlug * plugA, ZstPlug * plugB) const;

	//Plugs and addresses

	ZST_EXPORT void set_input(ZstPlug * input);
	ZST_EXPORT void set_output(ZstPlug * output);
	ZST_EXPORT ZstPlug * get_input();
	ZST_EXPORT ZstPlug * get_output();
	ZST_EXPORT const ZstURI & get_input_URI() const;
	ZST_EXPORT const ZstURI & get_output_URI() const;
	ZST_EXPORT void unplug();
	ZST_EXPORT bool is_local();

	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

private:
	//Cached URIs

	ZstURI m_input_URI;
	ZstURI m_output_URI;

	//Plugs

	ZstPlug * m_input;
	ZstPlug * m_output;

	ZST_EXPORT void set_local();
	bool m_is_local;
};

struct ZstCableHash
{
	ZST_EXPORT size_t operator()(ZstCable* const& k) const;
};

struct ZstCableEq {
	ZST_EXPORT bool operator()(ZstCable const * lhs, ZstCable const * rhs) const;
};
typedef std::unordered_set<ZstCable*, ZstCableHash, ZstCableEq> ZstCableList;


class ZstCableBundle {
public:
	ZST_EXPORT ZstCableBundle();
	ZST_EXPORT ~ZstCableBundle();
	ZST_EXPORT void add(ZstCable * cable);
	ZST_EXPORT ZstCable * cable_at(size_t index);
	ZST_EXPORT size_t size();
private:
	std::vector<ZstCable*> m_cables;
};
