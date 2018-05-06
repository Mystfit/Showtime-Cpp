#pragma once

#include <ZstExports.h>
#include <ZstEventDispatcher.hpp>
#include <adaptors/ZstSessionAdaptor.hpp>
#include <adaptors/ZstPlugAdaptors.hpp>

#include "ZstHierarchy.h"
#include "liasons/ZstSynchronisableLiason.hpp"
#include "liasons/ZstCableLiason.hpp"
#include "liasons/ZstPlugLiason.hpp"
#include "adaptors/ZstStageDispatchAdaptor.hpp"
#include "adaptors/ZstPerformanceDispatchAdaptor.hpp"


class ZstSession : 
	public ZstSynchronisableAdaptor,
	public ZstStageDispatchAdaptor,
	public ZstPerformanceDispatchAdaptor,
	public ZstOutputPlugAdaptor,
	protected ZstSynchronisableLiason,
	protected ZstCableLiason,
	protected ZstPlugLiason
{
public:
	ZST_EXPORT ZstSession();
	ZST_EXPORT virtual void process_events();
	ZST_EXPORT virtual void flush();
	ZST_EXPORT virtual void destroy();

	// ------------------
	// Cable creation
	// ------------------

	ZST_EXPORT virtual ZstCable * connect_cable(ZstPlug * input, ZstPlug * output, bool async = false);
	ZST_EXPORT virtual void destroy_cable(ZstCable * cable, bool async = false);
	ZST_EXPORT virtual void disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug);


	// -------------
	// Cable queries
	// -------------

	ZST_EXPORT virtual ZstCable * find_cable(const ZstURI & input_path, const ZstURI & output_path);
	ZST_EXPORT virtual ZstCable * find_cable(ZstPlug * input, ZstPlug * output);
	

	// -------------
	// Modules
	// -------------

	ZST_EXPORT virtual ZstHierarchy * hierarchy() = 0;


	// -----------------
	// Event dispatchers
	// -----------------

	ZST_EXPORT ZstEventDispatcher<ZstSessionAdaptor*> & session_events();
	ZST_EXPORT ZstEventDispatcher<ZstSynchronisableAdaptor*> & synchronisable_events();

protected:
	// --------------------------
	// Cable creation/destruction
	// --------------------------

	ZST_EXPORT virtual ZstCable * create_cable(const ZstCable & cable);
	ZST_EXPORT virtual ZstCable * create_cable(ZstPlug * output, ZstPlug * input);
	ZST_EXPORT virtual ZstCable * create_cable(const ZstURI & input_path, const ZstURI & output_path);


private:
	ZstCableList m_cables;

	ZstEventDispatcher<ZstSessionAdaptor*> m_session_events;
	ZstEventDispatcher<ZstSynchronisableAdaptor*> m_synchronisable_events;
};