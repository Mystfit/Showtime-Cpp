#pragma once

#include "ZstLogging.h"

#include "ZstExports.h"
#include "entities/ZstEntityFactory.h"
#include "adaptors/ZstHierarchyAdaptor.hpp"
#include "adaptors/ZstSessionAdaptor.hpp"
#include "adaptors/ZstEntityAdaptor.hpp"
#include "adaptors/ZstComputeAdaptor.hpp"

#include "ZstSynchronisableModule.h"
#include "ZstHierarchy.h"
#include "liasons/ZstEntityLiason.hpp"
#include "liasons/ZstCableLiason.hpp"
#include "liasons/ZstPlugLiason.hpp"
#include "adaptors/ZstTransportAdaptor.hpp"
#include "ZstEventDispatcher.hpp"


class ZstSession : 
	public ZstSynchronisableModule,
	public ZstTransportAdaptor,
	public ZstComputeAdaptor,
    public ZstHierarchyAdaptor,
    public ZstSessionAdaptor,
	protected ZstCableLiason,
	protected ZstPlugLiason,
    protected ZstEntityLiason
{
public:
	ZST_EXPORT ZstSession();
	ZST_EXPORT virtual void process_events() override;
	ZST_EXPORT virtual void flush_events() override;
	ZST_EXPORT virtual void init() override;
	ZST_EXPORT virtual void destroy() override;

	// ------------------
	// Cable creation
	// ------------------

	ZST_EXPORT virtual ZstCable * connect_cable(ZstInputPlug * input, ZstOutputPlug * output);
	ZST_EXPORT virtual ZstCable * connect_cable(ZstInputPlug * input, ZstOutputPlug * output, const ZstTransportRequestBehaviour & sendtype);
	ZST_EXPORT virtual void destroy_cable(ZstCable * cable);
	ZST_EXPORT virtual void destroy_cable(ZstCable * cable, const ZstTransportRequestBehaviour & sendtype);
	ZST_EXPORT void destroy_cable_complete(ZstCable * cable);
	ZST_EXPORT virtual void disconnect_plugs(ZstInputPlug * input_plug, ZstOutputPlug * output_plug);


	// -------------
	// Cable queries
	// -------------

    ZST_EXPORT virtual ZstCable * find_cable(const ZstCableAddress & cable_path) override;
	ZST_EXPORT virtual ZstCable * find_cable(const ZstURI & input_path, const ZstURI & output_path);
	ZST_EXPORT virtual ZstCable * find_cable(ZstInputPlug * input, ZstOutputPlug * output);
	ZST_EXPORT virtual ZstCableBundle & get_cables(ZstCableBundle & bundle) override;
	
    
	// -------------------------------
	// Syncronisable adaptor overrides
	// -------------------------------
	
	ZST_EXPORT void on_synchronisable_destroyed(ZstSynchronisable * synchronisable) override;

	// -------------
	// Compute adaptor overrides
	// -------------

	ZST_EXPORT virtual void on_compute(ZstComponent * component, ZstInputPlug * plug) override;

    // ---------------------------
    // Hierarchy adaptor overrides
    // ---------------------------
    
	ZST_EXPORT virtual void on_performer_arriving(ZstPerformer * performer) override;
    ZST_EXPORT virtual void on_entity_arriving(ZstEntityBase * entity) override;
    
	// ------------------
	// Entity observation
	// ------------------
	ZST_EXPORT virtual bool observe_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype);
	ZST_EXPORT void add_connected_performer(ZstPerformer * performer);
	ZST_EXPORT void remove_connected_performer(ZstPerformer * performer);
	ZST_EXPORT bool listening_to_performer(ZstPerformer * performer);


	// -----------------
	// Event dispatchers
	// -----------------

	ZST_EXPORT ZstEventDispatcher<ZstSessionAdaptor*> & session_events();
	ZST_EXPORT ZstEventDispatcher<ZstComputeAdaptor*> & compute_events();

protected:
	// --------------------------
	// Cable creation/destruction
	// --------------------------

	ZST_EXPORT virtual ZstCable * create_cable(ZstInputPlug * input, ZstOutputPlug * output);
	ZstCableSet m_cables;
    
    //Locking
    mutable std::mutex m_session_mtex;

private:
	// -----------------
	// Event dispatchers
	// -----------------
	ZstEventDispatcher<ZstSessionAdaptor*> m_session_events;
	ZstEventDispatcher<ZstComputeAdaptor*> m_compute_events;
	ZstPerformerMap m_connected_performers;
};
