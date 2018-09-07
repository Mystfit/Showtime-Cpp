#include "ZstSession.h"

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
	m_synchronisable_events.process_events();
	m_session_events.process_events();
	m_compute_events.process_events();
}

void ZstSession::flush()
{
	hierarchy()->flush_events();
	m_synchronisable_events.flush();
	m_session_events.flush();
}

void ZstSession::init()
{
	//We add this instance as an adaptor to make sure we can process local queued events
	m_synchronisable_events.add_adaptor(this);
	m_compute_events.add_adaptor(this);
}

void ZstSession::destroy()
{
	m_synchronisable_events.flush();
	m_synchronisable_events.remove_all_adaptors();
	m_compute_events.flush();
	m_compute_events.remove_all_adaptors();
}

ZstCable * ZstSession::connect_cable(ZstInputPlug * input, ZstOutputPlug * output) {
	return connect_cable(input, output, ZstTransportSendType::SYNC_REPLY);
}

ZstCable * ZstSession::connect_cable(ZstInputPlug * input, ZstOutputPlug * output, const ZstTransportSendType & sendtype)
{
	ZstCable * cable = NULL;

	if (!input || !output) {
		ZstLog::net(LogLevel::notification, "Can't connect cable, plug missing.");
		return NULL;
	}

	if (!input->is_activated()) {
		ZstLog::net(LogLevel::notification, "Can't connect cable, input plug {} is not activated.", input->URI().path());
		return NULL;
	}

	if (!output->is_activated()) {
		ZstLog::net(LogLevel::notification, "Can't connect cable, output plug {} is not activated.", output->URI().path());
		return NULL;
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

void ZstSession::destroy_cable(ZstCable * cable)
{
	return destroy_cable(cable, ZstTransportSendType::SYNC_REPLY);
}


void ZstSession::destroy_cable(ZstCable * cable, const ZstTransportSendType & sendtype)
{

}

void ZstSession::destroy_cable_complete(ZstCable * cable)
{
	if (!cable) return;

	//Lock the session
	//std::lock_guard<std::mutex> lock(m_session_mtex);

	//Remove cable from local list so that other threads don't assume it still exists
	m_cables.erase(cable);

	//Disconnect the cable
	cable->disconnect();
}

void ZstSession::disconnect_plugs(ZstInputPlug * input_plug, ZstOutputPlug * output_plug)
{
	ZstCable * cable = find_cable(input_plug->URI(), output_plug->URI());
	destroy_cable(cable);
}


std::shared_ptr<ZstEntityBase> ZstSession::create_entity(const ZstURI & creatable_path, const char * name)
{
	//Search through available factories to find a creatable entity
	for (auto factory : m_factories) {
	}
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

ZstCable * ZstSession::find_cable(ZstInputPlug * input, ZstOutputPlug * output)
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

ZstCable * ZstSession::create_cable(ZstInputPlug * input, ZstOutputPlug * output)
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
		if (cable_ptr->is_proxy()) {

		}

		//If we created this cable already, we need to finish its activation
		if(!cable_ptr->is_activated()){
			synchronisable_enqueue_activation(cable_ptr);
		}
		return cable_ptr;
	}
	
	//Create and store new cable
	bool success = true;
	cable_ptr = ZstCable::create(input_path, output_path);
	try {
		m_cables.insert(cable_ptr);
	}
	catch (std::exception e) {
		ZstLog::net(LogLevel::notification, "Couldn't insert cable. Reason:", e.what());
		success = false;
	}

	//Add cable to plugs
	ZstInputPlug * input_plug = dynamic_cast<ZstInputPlug*>(hierarchy()->find_entity(input_path));
	ZstOutputPlug * output_plug = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(output_path));
	if (!input_plug || !output_plug) {
		ZstLog::net(LogLevel::error, "Can't connect cable, a plug is missing.");
		success = false;
	}

	//If we failed to create the cable, we should cleanup our resources before returning
	if (!success) {
		ZstCable::destroy(cable_ptr);
		cable_ptr = NULL;
		return cable_ptr;
	}

	//Set up plug and cable references
	plug_add_cable(input_plug, cable_ptr);
	plug_add_cable(output_plug, cable_ptr);
	cable_ptr->set_input(input_plug);
	cable_ptr->set_output(output_plug);

	//Add synchronisable adaptor to cable to handle activation
	cable_ptr->add_adaptor(this);

	//Cables are always local so they can be cleaned up by the reaper when deactivated
	synchronisable_set_proxy(cable_ptr);

	//Enqueue events
	synchronisable_set_activation_status(cable_ptr, ZstSyncStatus::ACTIVATED);
	m_session_events.defer([cable_ptr](ZstSessionAdaptor * dlg) { dlg->on_cable_created(cable_ptr); });

	return cable_ptr;
}

void ZstSession::on_compute(ZstComponent * component, ZstInputPlug * plug) {
	try {
		component->compute(plug);
	}
	catch (std::exception e) {
		ZstLog::entity(LogLevel::error, "Compute on component {} failed. Error was: {}", component->URI().path(), e.what());
	}
}

bool ZstSession::observe_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	if (!entity->is_proxy()) {
		ZstLog::entity(LogLevel::warn, "Can't observe local entity {}", entity->URI().path());
		return false;
	}

	if (listening_to_performer(dynamic_cast<ZstPerformer*>(hierarchy()->find_entity(entity->URI().first()))))
	{
		ZstLog::entity(LogLevel::warn, "Already observing performer {}", entity->URI().first().path());
		return false;
	}
	
	return true;
}

void ZstSession::add_connected_performer(ZstPerformer * performer)
{
	if (!performer)
		return;
	m_connected_performers[performer->URI()] = performer;
}

void ZstSession::remove_connected_performer(ZstPerformer * performer)
{
	if (!performer)
		return;

	try {
		m_connected_performers.erase(performer->URI());
	}
	catch (std::out_of_range) {

	}
}

bool ZstSession::listening_to_performer(ZstPerformer * performer)
{
	return m_connected_performers.find(performer->URI()) != m_connected_performers.end();
}

void ZstSession::on_synchronisable_destroyed(ZstSynchronisable * synchronisable)
{
	if (synchronisable->is_proxy())
		reaper().add(synchronisable);
}

void ZstSession::synchronisable_has_event(ZstSynchronisable * synchronisable)
{
	synchronisable_events().defer([this, synchronisable](ZstSynchronisableAdaptor * dlg) {
		this->synchronisable_process_events(synchronisable);
	});
}

ZstEventDispatcher<ZstSessionAdaptor*> & ZstSession::session_events()
{
	return m_session_events;
}

ZstEventDispatcher<ZstSynchronisableAdaptor*> & ZstSession::synchronisable_events()
{
	return m_synchronisable_events;
}

ZstEventDispatcher<ZstComputeAdaptor*>& ZstSession::compute_events()
{
	return m_compute_events;
}
