#pragma once

#include <ZstExports.h>
#include <ZstEventDispatcher.hpp>
#include <adaptors/ZstSessionAdaptor.hpp>
#include <adaptors/ZstEntityAdaptor.hpp>
#include <adaptors/ZstComputeAdaptor.hpp>

#include "ZstModule.h"
#include "ZstHierarchy.h"
#include "liasons/ZstSynchronisableLiason.hpp"
#include "liasons/ZstCableLiason.hpp"
#include "liasons/ZstPlugLiason.hpp"
#include "adaptors/ZstTransportAdaptor.hpp"


class ZstSession : 
	public ZstModule,
	public ZstSynchronisableAdaptor,
	public ZstTransportAdaptor,
	public ZstComputeAdaptor,
    public ZstEntityAdaptor,
	protected ZstSynchronisableLiason,
	protected ZstCableLiason,
	protected ZstPlugLiason
{
public:
	ZST_EXPORT ZstSession();
	ZST_EXPORT virtual void process_events();
	ZST_EXPORT virtual void flush();
	ZST_EXPORT virtual void init();
	ZST_EXPORT virtual void destroy();

	// ------------------
	// Cable creation
	// ------------------

	ZST_EXPORT virtual ZstCable * connect_cable(ZstInputPlug * input, ZstOutputPlug * output);
	ZST_EXPORT virtual ZstCable * connect_cable(ZstInputPlug * input, ZstOutputPlug * output, const ZstTransportSendType & sendtype);
	ZST_EXPORT virtual void destroy_cable(ZstCable * cable);
	ZST_EXPORT virtual void destroy_cable(ZstCable * cable, const ZstTransportSendType & sendtype);
	ZST_EXPORT virtual void destroy_cable_complete(ZstCable * cable);
	ZST_EXPORT virtual void disconnect_plugs(ZstInputPlug * input_plug, ZstOutputPlug * output_plug);


	// -------------
	// Cable queries
	// -------------

	ZST_EXPORT virtual ZstCable * find_cable(const ZstURI & input_path, const ZstURI & output_path);
	ZST_EXPORT virtual ZstCable * find_cable(ZstInputPlug * input, ZstOutputPlug * output);
	

	// -------------
	// Modules
	// -------------

	ZST_EXPORT virtual ZstHierarchy * hierarchy() = 0;


	// -------------------------------
	// Syncronisable adaptor overrides
	// -------------------------------
	
	ZST_EXPORT void on_synchronisable_destroyed(ZstSynchronisable * synchronisable) override;
	ZST_EXPORT void synchronisable_has_event(ZstSynchronisable * synchronisable) override;

	// -------------
	// Compute adaptor overrides
	// -------------

	ZST_EXPORT virtual void on_compute(ZstComponent * component, ZstInputPlug * plug);


	// -----------------
	// Event dispatchers
	// -----------------

	ZST_EXPORT ZstEventDispatcher<ZstSessionAdaptor*> & session_events();
	ZST_EXPORT ZstEventDispatcher<ZstSynchronisableAdaptor*> & synchronisable_events();
	ZST_EXPORT ZstEventDispatcher<ZstComputeAdaptor*> & compute_events();

protected:
	// --------------------------
	// Cable creation/destruction
	// --------------------------

	ZST_EXPORT virtual ZstCable * create_cable(const ZstCable & cable);
	ZST_EXPORT virtual ZstCable * create_cable(ZstInputPlug * input, ZstOutputPlug * output);
	ZST_EXPORT virtual ZstCable * create_cable(const ZstURI & input_path, const ZstURI & output_path);
	ZstCableList m_cables;

private:
	// -----------------
	// Event dispatchers
	// -----------------
	ZstEventDispatcher<ZstSessionAdaptor*> m_session_events;
	ZstEventDispatcher<ZstSynchronisableAdaptor*> m_synchronisable_events;
	ZstEventDispatcher<ZstComputeAdaptor*> m_compute_events;

};
