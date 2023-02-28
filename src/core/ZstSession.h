#pragma once

#include <showtime/ZstExports.h>
#include <showtime/entities/ZstComputeComponent.h>
#include <showtime/entities/ZstEntityFactory.h>
#include <showtime/adaptors/ZstEntityAdaptor.hpp>
#include <showtime/adaptors/ZstHierarchyAdaptor.hpp>
#include <showtime/adaptors/ZstSessionAdaptor.hpp>
#include <showtime/adaptors/ZstEntityAdaptor.hpp>
#include <showtime/adaptors/ZstComputeAdaptor.hpp>

#include "ZstSynchronisableModule.h"
#include "ZstHierarchy.h"
#include "liasons/ZstEntityLiason.hpp"
#include "liasons/ZstCableLiason.hpp"
#include "liasons/ZstPlugLiason.hpp"
#include "adaptors/ZstTransportAdaptor.hpp"
#include "ZstEventDispatcher.hpp"

namespace showtime {

class ZST_CLASS_EXPORTED ZstSession :
	public ZstSynchronisableModule,
	public ZstComputeAdaptor,
    public ZstHierarchyAdaptor,
	public ZstEntityAdaptor,
    public ZstSessionAdaptor,
	protected ZstCableLiason,
	protected ZstPlugLiason,
    protected ZstEntityLiason
{
public:
	ZST_EXPORT ZstSession();
    ZST_EXPORT virtual ~ZstSession();
	ZST_EXPORT virtual void init_adaptors() override;
	ZST_EXPORT virtual void process_events() override;
	ZST_EXPORT virtual void flush_events() override;

	// ------------------
	// Cable creation
	// ------------------

	ZST_EXPORT virtual ZstCable * connect_cable(ZstInputPlug * input, ZstOutputPlug * output);
	ZST_EXPORT virtual ZstCable * connect_cable(ZstInputPlug * input, ZstOutputPlug * output, const ZstTransportRequestBehaviour & sendtype) override;
	ZST_EXPORT virtual void destroy_cable(ZstCable * cable) override;
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
	
	ZST_EXPORT void on_synchronisable_destroyed(ZstSynchronisable * synchronisable, bool already_removed) override;

	// -------------
	// Compute adaptor overrides
	// -------------

	ZST_EXPORT virtual void on_request_compute(ZstComputeComponent* compute_component, ZstInputPlug * plug) override;

    // ---------------------------
    // Hierarchy adaptor overrides
    // ---------------------------
    
	ZST_EXPORT virtual void on_performer_arriving(ZstPerformer * performer) override;
    ZST_EXPORT virtual void on_entity_arriving(ZstEntityBase * entity) override;
	ZST_EXPORT virtual void update_cable_paths(ZstEntityBase* entity, const ZstURI& original_path) override;
	ZST_EXPORT virtual void request_entity_registration(ZstEntityBase* entity) override;
    
	// ------------------
	// Entity observation
	// ------------------
	ZST_EXPORT void register_entity(ZstEntityBase* entity);
	ZST_EXPORT virtual bool observe_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype);
	ZST_EXPORT void add_connected_performer(const ZstURI& performer_path);
	ZST_EXPORT void remove_connected_performer(const ZstURI& performer_path);
	ZST_EXPORT bool listening_to_performer(const ZstURI& performer_path);


	// -----------------
	// Event dispatchers
	// -----------------

	ZST_EXPORT std::shared_ptr<ZstEventDispatcher<ZstSessionAdaptor> > & session_events();
	ZST_EXPORT std::shared_ptr<ZstEventDispatcher<ZstComputeAdaptor> > & compute_events();


	// -------
	// Modules
	// -------

	virtual std::shared_ptr<ZstHierarchy> hierarchy() = 0;


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
	std::shared_ptr<ZstEventDispatcher<ZstSessionAdaptor> > m_session_events;
	std::shared_ptr<ZstEventDispatcher<ZstComputeAdaptor> > m_compute_events;
	std::set<ZstURI> m_connected_performers;
};

}
