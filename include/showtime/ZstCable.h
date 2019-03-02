#pragma once

#include <unordered_set>
#include <vector>

#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstSerialisable.h"
#include "ZstSynchronisable.h"

//Forwards
class ZstInputPlug;
class ZstOutputPlug;
class ZstPlug;

class ZstCable : public ZstSynchronisable, public ZstSerialisable {
public:
	friend class ZstCableLiason;
	friend class ZstStage;
    friend class ZstComponent;
    friend class ZstPlug;

	ZST_EXPORT ZstCable();
	ZST_EXPORT ZstCable(const ZstCable & copy);
	ZST_EXPORT ZstCable(const ZstURI & input_plug_URI, const ZstURI & output_plug_URI);
	ZST_EXPORT ZstCable(ZstInputPlug * input_plug, ZstOutputPlug * output_plug);
	ZST_EXPORT static ZstCable * create(const ZstURI & input, const ZstURI & output);
	ZST_EXPORT static ZstCable * create(ZstInputPlug * input, ZstOutputPlug * output);

	ZST_EXPORT static void destroy(ZstCable * cable);
    ZST_EXPORT virtual ~ZstCable();
	ZST_EXPORT void disconnect();

	// Status

	ZST_EXPORT bool operator==(const ZstCable & other) const;
	ZST_EXPORT bool operator!=(const ZstCable & other);
	ZST_EXPORT bool is_attached(const ZstURI & uri) const;
	ZST_EXPORT bool is_attached(const ZstURI & uriA, const ZstURI & uriB) const;
	ZST_EXPORT bool is_attached(ZstPlug * plug) const;
	ZST_EXPORT bool is_attached(ZstPlug * plugA, ZstPlug * plugB) const;

	//Plugs and addresses

	ZST_EXPORT void set_input(ZstInputPlug * input);
	ZST_EXPORT void set_output(ZstOutputPlug * output);
	ZST_EXPORT ZstInputPlug * get_input();
	ZST_EXPORT ZstOutputPlug * get_output();
	ZST_EXPORT const ZstURI & get_input_URI() const;
	ZST_EXPORT const ZstURI & get_output_URI() const;

	//Serialisation

	ZST_EXPORT void write_json(json & buffer) const override;
	ZST_EXPORT void read_json(const json & buffer) override;

	//Testing
	ZST_EXPORT static void self_test();

private:
    //Cached URIs
	ZstURI m_input_URI;
	ZstURI m_output_URI;

	//Plugs
	ZstInputPlug * m_input;
	ZstOutputPlug * m_output;
};


struct ZstCableHash
{
	ZST_EXPORT size_t operator()(ZstCable* const& k) const;
};


struct ZstCableEq {
	ZST_EXPORT bool operator()(ZstCable const * lhs, ZstCable const * rhs) const;
};
typedef std::unordered_set<ZstCable*, ZstCableHash, ZstCableEq> ZstCableList;