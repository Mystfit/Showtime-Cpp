#pragma once

#include <ZstExports.h>
#include <ZstEventDispatcher.hpp>
#include <adaptors/ZstSessionAdaptor.hpp>
#include "ZstHierarchy.h"
#include "liasons/ZstSynchronisableLiason.hpp"
#include "liasons/ZstCableLiason.hpp"
#include "liasons/ZstPlugLiason.hpp"


class ZstSession : 
	public ZstEventDispatcher<ZstSessionAdaptor*>,
	public ZstSessionAdaptor,
	public ZstSynchronisableAdaptor,
	protected ZstSynchronisableLiason,
	protected ZstCableLiason,
	protected ZstPlugLiason
{
	using ZstEventDispatcher<ZstSessionAdaptor*>::add_adaptor;
	using ZstEventDispatcher<ZstSessionAdaptor*>::remove_adaptor;
	using ZstEventDispatcher<ZstSessionAdaptor*>::process_events;
	using ZstEventDispatcher<ZstSessionAdaptor*>::run_event;
	using ZstEventDispatcher<ZstSessionAdaptor*>::add_event;
	using ZstEventDispatcher<ZstSessionAdaptor*>::flush;

public:
	ZST_EXPORT virtual void process_events();
	ZST_EXPORT virtual void flush();

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

protected:
	// --------------------------
	// Cable creation/destruction
	// --------------------------

	ZST_EXPORT virtual ZstCable * create_cable(const ZstCable & cable);
	ZST_EXPORT virtual ZstCable * create_cable(ZstPlug * output, ZstPlug * input);
	ZST_EXPORT virtual ZstCable * create_cable(const ZstURI & input_path, const ZstURI & output_path);

private:
	ZstCableList m_cables;
	ZstHierarchy * m_hierarchy;
};