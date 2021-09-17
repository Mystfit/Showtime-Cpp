#pragma once

#include <unordered_set>
#include <set>
#include <memory>
#include <vector>

#include <showtime/ZstExports.h>
#include <showtime/ZstURI.h>
#include <showtime/adaptors/ZstHierarchyAdaptor.hpp>

#include "ZstCableAddress.h"
#include "ZstSynchronisable.h"
#include "ZstBundle.hpp"

namespace showtime {

//Forwards
class ZstInputPlug;
class ZstOutputPlug;
class ZstPlug;
class ZstEntityBase;
typedef ZstBundle<ZstEntityBase*> ZstEntityBundle;


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
    
	ZST_EXPORT ZstInputPlug * get_input() const;
	ZST_EXPORT ZstOutputPlug * get_output() const;
    ZST_EXPORT const ZstCableAddress & get_address() const;
    ZST_EXPORT void get_cable_route(ZstEntityBundle& bundle) const;

    // Adaptors
#ifndef SWIG
    //Include base class adaptors
    //Swig mistakenly adds these twice when dealing treating ZstSynchronisable as an interface (C#, Java)
    using ZstSynchronisable::add_adaptor;
    using ZstSynchronisable::remove_adaptor;
#endif
    ZST_EXPORT virtual void add_adaptor(std::shared_ptr<ZstHierarchyAdaptor> adaptor);
    ZST_EXPORT virtual void remove_adaptor(std::shared_ptr<ZstHierarchyAdaptor> adaptor);


private:
    ZstEntityBase* get_entity(const ZstURI& entity_path) const;
    ZstCableAddress m_address;
    std::shared_ptr<ZstEventDispatcher<ZstHierarchyAdaptor> > m_hierarchy_events;
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

