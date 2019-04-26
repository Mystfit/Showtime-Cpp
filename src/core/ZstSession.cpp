#include "ZstSession.h"

// ------------------
// Cable creation API
// ------------------

ZstSession::ZstSession() : 
	m_session_events("session")
{
}

void ZstSession::process_events()
{
	hierarchy()->process_events();
	m_session_events.process_events();
	m_compute_events.process_events();
    
    ZstSynchronisableModule::process_events();
}

void ZstSession::flush_events()
{
	hierarchy()->flush_events();
	m_session_events.flush();
    
    ZstSynchronisableModule::flush_events();
}

void ZstSession::init()
{
    //Add adaptors
	m_compute_events.add_adaptor(this);
    
    //Attach session as an adaptor to the hierarchy module to handle events that will need to modify cables
    hierarchy()->hierarchy_events().add_adaptor(this);
    
    ZstSynchronisableModule::init();
}

void ZstSession::destroy()
{
	//Clear events
	m_compute_events.flush();
	m_compute_events.remove_all_adaptors();
	m_session_events.flush();
	m_session_events.remove_all_adaptors();

	//Clear connected performers - they'll remove us when we leave the graph
	m_connected_performers.clear();
    
    ZstSynchronisableModule::destroy();
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
	std::lock_guard<std::mutex> lock(m_session_mtex);
    
    //Remove cable from plugs
	ZstInputPlug * input = dynamic_cast<ZstInputPlug*>(hierarchy()->walk_to_entity(cable->get_address().get_input_URI()));
	if (input) {
		ZstPlugLiason().plug_remove_cable(input, cable);
	}
	ZstOutputPlug * output = dynamic_cast<ZstOutputPlug*>(hierarchy()->walk_to_entity(cable->get_address().get_output_URI()));
	if (output) {
		ZstPlugLiason().plug_remove_cable(output, cable);
	}

    //Dispatch events
    session_events().defer([cable](ZstSessionAdaptor * dlg) { dlg->on_cable_destroyed(cable); });
    
    //Deactivate the cable
    synchronisable_enqueue_deactivation(cable);
}

void ZstSession::disconnect_plugs(ZstInputPlug * input_plug, ZstOutputPlug * output_plug)
{
	ZstCable * cable = find_cable(input_plug->URI(), output_plug->URI());
	destroy_cable(cable);
}

ZstCable * ZstSession::find_cable(const ZstCableAddress & cable_path)
{
    return find_cable(cable_path.get_input_URI(), cable_path.get_output_URI());
}

ZstCable * ZstSession::find_cable(const ZstURI & input_path, const ZstURI & output_path)
{
    std::lock_guard<std::mutex> lock(m_session_mtex);
    
    auto search_cable = ZstCableAddress(input_path, output_path);
    for(auto const & c : m_cables){
        if(search_cable == c.get()->get_address()){
            return c.get();
        }
    }
	return NULL;
}

ZstCable * ZstSession::find_cable(ZstInputPlug * input, ZstOutputPlug * output)
{
	if (!input || !output) {
		return NULL;
	}
	return find_cable(input->URI(), output->URI());
}

ZstCableBundle & ZstSession::get_cables(ZstCableBundle & bundle)
{
	for (auto const & c : m_cables) {
		bundle.add(c.get());
	}
	return bundle;
}

ZstCable * ZstSession::create_cable(ZstInputPlug * input, ZstOutputPlug * output)
{
    if (!input || !output) {
        ZstLog::net(LogLevel::error, "Can't connect cable, a plug is missing.");
        return NULL;
    }

    //Find existing cables that match
	ZstCable * cable = find_cable(ZstCableAddress(input->URI(), output->URI()));
	if (cable) {
		//If we created this cable already, we need to finish its activation
		if(!cable->is_activated()){
			synchronisable_enqueue_activation(cable);
		}
		return cable;
	}
	
	//Create and store new cable
    auto cable_it = m_cables.insert(std::make_unique<ZstCable>(input, output));
    const std::unique_ptr<ZstCable> & cable_ptr = *(cable_it.first);

	//Set up plug and cable references
	plug_add_cable(input, cable_ptr.get());
	plug_add_cable(output, cable_ptr.get());
	cable_ptr->set_input(input);
	cable_ptr->set_output(output);

	//Add synchronisable adaptor to cable to handle activation
	cable_ptr->add_adaptor(this);
        
	//Cables are always local so they can be cleaned up by the reaper when deactivated
	synchronisable_set_proxy(cable_ptr.get());

	//Enqueue events
	synchronisable_set_activation_status(cable_ptr.get(), ZstSyncStatus::ACTIVATED);
	m_session_events.defer([&cable_ptr](ZstSessionAdaptor * dlg) { dlg->on_cable_created(cable_ptr.get()); });

	return cable_ptr.get();
}

void ZstSession::on_compute(ZstComponent * component, ZstInputPlug * plug) {
    try {
        component->compute(plug);
    }
    catch (std::exception e) {
        ZstLog::entity(LogLevel::error, "Compute on component {} failed. Error was: {}", component->URI().path(), e.what());
    }
}

void ZstSession::on_entity_arriving(ZstEntityBase * entity)
{
    //New entities need to register the session as an adaptor to query session data
    ZstEntityBundle bundle;
    for(auto child : entity->get_child_entities(bundle)){
        child->add_adaptor(static_cast<ZstSessionAdaptor*>(this));
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
    //Check if synchronisable is a cable
    ZstCable * cable = dynamic_cast<ZstCable*>(synchronisable);
    if(cable){
        reaper().add_cleanup_op([this, cable](){
            //Remove cable from local list so that other threads don't assume it still exists
            for(auto const & c : this->m_cables){
                //Find the unique_ptr that owns this cable
                if(c.get() == cable){
                    auto cable_it = this->m_cables.find(c.get()->get_address());
                    if(cable_it != this->m_cables.end()){
                        //Erasing the unique pointer will destroy the cable
                        this->m_cables.erase(cable_it);
                    }
                    break;
                }
            }
        });
    } else {
        //All other synchronisables go to the reaper (for now)
        if (synchronisable->is_proxy())
            reaper().add(synchronisable);
    }
}

ZstEventDispatcher<ZstSessionAdaptor*> & ZstSession::session_events()
{
	return m_session_events;
}

ZstEventDispatcher<ZstComputeAdaptor*>& ZstSession::compute_events()
{
	return m_compute_events;
}
