#pragma once

#include <unordered_set>
#include <set>
#include <memory>
#include <vector>

#include "ZstCableAddress.h"
#include <showtime/ZstExports.h>
#include <showtime/ZstURI.h>
#include "ZstSynchronisable.h"
#include "ZstBundle.hpp"

namespace showtime {

//Forwards
class ZstInputPlug;
class ZstOutputPlug;
class ZstPlug;


class ZST_CLASS_EXPORTED ZstCable : public ZstSynchronisable {
public:
	friend class ZstCableLiason;
	friend class ZstStage;
    friend class ZstComponent;
    friend class ZstPlug;

	ZST_EXPORT ZstCable();
	ZST_EXPORT ZstCable(const ZstCable & copy);
	ZST_EXPORT ZstCable(ZstInputPlug * input_plug, ZstOutputPlug * output_plug);
    ZST_EXPORT virtual ~ZstCable();
	ZST_EXPORT void disconnect();

	// Status
    
	ZST_EXPORT bool is_attached(const ZstURI & uri) const;
	ZST_EXPORT bool is_attached(const ZstURI & uriA, const ZstURI & uriB) const;
	ZST_EXPORT bool is_attached(ZstPlug * plug) const;
	ZST_EXPORT bool is_attached(ZstPlug * plugA, ZstPlug * plugB) const;

	//Plugs and addresses
    
	ZST_EXPORT void set_input(ZstInputPlug * input);
	ZST_EXPORT void set_output(ZstOutputPlug * output);
	ZST_EXPORT ZstInputPlug * get_input();
	ZST_EXPORT ZstOutputPlug * get_output();
    ZST_EXPORT const ZstCableAddress & get_address() const;

private:
    ZstCableAddress m_address;
	ZstInputPlug * m_input;
	ZstOutputPlug * m_output;
};


struct ZstCableCompare
{
    using is_transparent = std::true_type;
    
    bool operator()(std::unique_ptr<ZstCable> const& lhs, std::unique_ptr<ZstCable> const& rhs ) const
    {
        if(!lhs || !rhs)
            return false;
        return lhs->get_address() < rhs->get_address();
    }
    
    bool operator()(const ZstCableAddress & address, std::unique_ptr<ZstCable> const& cable) const
    {
        if(!cable)
            return false;
        return address < cable->get_address();
    }
    
    bool operator()(std::unique_ptr<ZstCable> const& cable, const ZstCableAddress & address) const
    {
        if(!cable)
            return false;
        return cable->get_address() < address;
    }
};


//Cable typedefs

typedef std::set< std::unique_ptr<ZstCable>, ZstCableCompare > ZstCableSet;
typedef ZstBundle<ZstCable*> ZstCableBundle;
typedef ZstBundle<ZstCableAddress> ZstCableAddressBundle;
typedef ZstBundleIterator<ZstCable*> ZstCableBundleIterator;
    
}

