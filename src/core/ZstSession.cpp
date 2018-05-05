#include "ZstSession.h"
#include <ZstLogging.h>

// ------------------
// Cable creation API
// ------------------

ZstSession::ZstSession() : 
	m_session_events("session"),
	m_synchronisable_events("session synchronisable")
{
}

void ZstSession::process_events()
{
	hierarchy()->process_events();
	m_session_events.process_events();
}

void ZstSession::flush()
{
	hierarchy()->events().flush();
	m_session_events.flush();
}

void ZstSession::destroy()
{
}

ZstCable * ZstSession::connect_cable(ZstPlug * input, ZstPlug * output, bool async)
{
	ZstCable * cable = NULL;

	if (!input || !output) {
		ZstLog::net(LogLevel::notification, "Can't connect cable, plug missing.");
		return cable;
	}

	if (!input->is_activated() || !output->is_activated()) {
		ZstLog::net(LogLevel::notification, "Can't connect cable, plug is not activated.");
		return cable;
	}

	if (input->direction() != ZstPlugDirection::IN_JACK || output->direction() != ZstPlugDirection::OUT_JACK) {
		ZstLog::net(LogLevel::notification, "Cable order incorrect");
		return NULL;
	}

	cable = create_cable(input, output);
	if (!cable) {
		ZstLog::net(LogLevel::notification, "Couldn't create cable, already exists!");
		return NULL;
	}

	synchronisable_set_activating(cable);

	//If either of the cable plugs are a local entity, then the cable is local as well
	if (!input->is_proxy() || !output->is_proxy()) {
		cable_set_local(cable);
	}

	//Create the cable early so we have something to return immediately
	return cable;
}


void ZstSession::destroy_cable(ZstCable * cable, bool async)
{
	if (!cable) return;

	//Remove cable from local list so that other threads don't assume it still exists
	m_cables.erase(cable);
	synchronisable_set_deactivating(cable);
}


void ZstSession::disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug)
{
	ZstCable * cable = find_cable(input_plug->URI(), output_plug->URI());
	destroy_cable(cable);
}

ZstCable * ZstSession::find_cable(const ZstURI & input_path, const ZstURI & output_path)
{
	ZstCable * cable = NULL;
	if (m_cables.size()) {
		auto search_cable = ZstCable(input_path, output_path);
		auto cable_ptr = m_cables.find(&search_cable);
		if (cable_ptr != m_cables.end()) {
			cable = *cable_ptr;
		}
	}
	return cable;
}

ZstCable * ZstSession::find_cable(ZstPlug * input, ZstPlug * output)
{
	if (!input || !output) {
		return NULL;
	}
	return find_cable(input->URI(), output->URI());
}

ZstCable * ZstSession::create_cable(const ZstCable & cable)
{
	return create_cable(cable.get_input_URI(), cable.get_output_URI());
}

ZstCable * ZstSession::create_cable(ZstPlug * input, ZstPlug * output)
{
	if (!input || !output) {
		return NULL;
	}
	return create_cable(input->URI(), output->URI());
}

ZstCable * ZstSession::create_cable(const ZstURI & input_path, const ZstURI & output_path)
{
	ZstCable * cable_ptr = find_cable(input_path, output_path);
	if (cable_ptr) {
		return cable_ptr;
	}

	//Create and store new cable
	cable_ptr = ZstCable::create(input_path, output_path);
	try {
		m_cables.insert(cable_ptr);
	}
	catch (std::exception e) {
		ZstLog::net(LogLevel::notification, "Couldn't insert cable. Reason:", e.what());
		ZstCable::destroy(cable_ptr);
		cable_ptr = NULL;
		return cable_ptr;
	}

	//Add cable to plugs
	ZstPlug * input_plug = dynamic_cast<ZstPlug*>(hierarchy()->find_entity(input_path));
	ZstPlug * output_plug = dynamic_cast<ZstPlug*>(hierarchy()->find_entity(output_path));
	plug_add_cable(input_plug, cable_ptr);
	plug_add_cable(output_plug, cable_ptr);
	cable_ptr->set_input(input_plug);
	cable_ptr->set_output(output_plug);

	//Set adaptors
	cable_ptr->add_adaptor(this);

	//Cables are always local so they can be cleaned up by the reaper
	synchronisable_set_proxy(cable_ptr);

	//Enqueue events
	synchronisable_set_activation_status(cable_ptr, ZstSyncStatus::ACTIVATED);

	m_session_events.defer([cable_ptr](ZstSessionAdaptor * dlg) { dlg->on_cable_created(cable_ptr); });
	return cable_ptr;
}

ZstEventDispatcher<ZstSessionAdaptor*> & ZstSession::session_events()
{
	return m_session_events;
}

ZstEventDispatcher<ZstSynchronisableAdaptor*> & ZstSession::synchronisable_events()
{
	return m_synchronisable_events;
}
