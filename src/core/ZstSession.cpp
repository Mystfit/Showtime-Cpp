#include "ZstSession.h"

// ------------------
// Cable creation API
// ------------------

namespace showtime {

ZstSession::ZstSession() : 
	m_session_events(std::make_shared<ZstEventDispatcher<ZstSessionAdaptor> >()),
	m_compute_events(std::make_shared<ZstEventDispatcher<ZstComputeAdaptor> >())
{
}

ZstSession::~ZstSession()
{
    //Clear connected performers - they'll remove us when we leave the graph
    m_connected_performers.clear();
    
    ///Clear cables
    m_cables.clear();
}

void ZstSession::init_adaptors()
{
	//Let this session respond to incoming entities
	hierarchy()->hierarchy_events()->add_adaptor(ZstHierarchyAdaptor::downcasted_shared_from_this<ZstHierarchyAdaptor>());
	
	//Let this session handle incoming graph compute events
	m_compute_events->add_adaptor(ZstComputeAdaptor::downcasted_shared_from_this<ZstComputeAdaptor>());
	
	ZstSynchronisableModule::init_adaptors();
}

void ZstSession::process_events()
{
	hierarchy()->process_events();
	m_session_events->process_events();
	m_compute_events->process_events();
    
    ZstSynchronisableModule::process_events();
}

void ZstSession::flush_events()
{
	hierarchy()->flush_events();
    ZstSynchronisableModule::flush_events();
}

ZstCable * ZstSession::connect_cable(ZstInputPlug * input, ZstOutputPlug * output) {
	return connect_cable(input, output, ZstTransportRequestBehaviour::SYNC_REPLY);
}

ZstCable * ZstSession::connect_cable(ZstInputPlug * input, ZstOutputPlug * output, const ZstTransportRequestBehaviour & sendtype)
{
	ZstCable * cable = NULL;

	if (!input) {
		Log::net(Log::Level::notification, "Can't connect cable, input plug missing.");
		return NULL;
	}

	if (!output) {
		Log::net(Log::Level::notification, "Can't connect cable, output plug missing.");
		return NULL;
	}

	if (!input->is_activated()) {
		Log::net(Log::Level::notification, "Can't connect cable, input plug {} is not activated.", input->URI().path());
		return NULL;
	}

	if (!output->is_activated()) {
		Log::net(Log::Level::notification, "Can't connect cable, output plug {} is not activated.", output->URI().path());
		return NULL;
	}

	if (input->direction() != ZstPlugDirection::IN_JACK || output->direction() != ZstPlugDirection::OUT_JACK) {
		Log::net(Log::Level::notification, "Cable order incorrect");
		return NULL;
	}

	cable = create_cable(input, output);
	if (!cable) {
		Log::net(Log::Level::notification, "Couldn't create cable, already exists!");
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
	return destroy_cable(cable, ZstTransportRequestBehaviour::SYNC_REPLY);
}


void ZstSession::destroy_cable(ZstCable * cable, const ZstTransportRequestBehaviour & sendtype)
{
}

void ZstSession::destroy_cable_complete(ZstCable * cable)
{
	if (!cable) return;

	//Lock the session
	std::lock_guard<std::mutex> lock(m_session_mtex);
    
    //Remove cable from plugs
	ZstInputPlug* input = dynamic_cast<ZstInputPlug*>(hierarchy()->find_entity(cable->get_address().get_input_URI()));
	ZstOutputPlug* output = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(cable->get_address().get_output_URI()));

	if (input) 
		ZstPlugLiason().plug_remove_cable(input, cable);
	
	if (output) 
		ZstPlugLiason().plug_remove_cable(output, cable);

    //Dispatch events
    session_events()->defer([cable = cable->get_address()](ZstSessionAdaptor* adaptor) {
		adaptor->on_cable_destroyed(cable);
	});
    
    //Deactivate the cable
    synchronisable_enqueue_deactivation(cable);

	Log::net(Log::Level::debug, "Destroy cable ({}-->{}) completed",
		cable->get_address().get_output_URI().path(),
		cable->get_address().get_input_URI().path());
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
	auto cable_it = std::find_if(m_cables.begin(), m_cables.end(), [&search_cable](auto& val) { 
		return val.get()->get_address() == search_cable; 
	});
	if (cable_it != m_cables.end()) {
		return cable_it->get();
	}
	Log::net(Log::Level::error, "No cable found for address {}<-{}", input_path.path(), output_path.path());
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
        Log::net(Log::Level::error, "Can't connect cable, a plug is missing.");
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

	//Add adaptors to cable to handle activation and session lookups of entities
	cable_ptr->add_adaptor(ZstSynchronisableAdaptor::downcasted_shared_from_this<ZstSynchronisableAdaptor>());
	cable_ptr->add_adaptor(std::static_pointer_cast<ZstHierarchyAdaptor>(hierarchy()));

	//Cables are always local so they can be cleaned up by the reaper when deactivated
	synchronisable_set_proxy(cable_ptr.get());

	//Enqueue events
	synchronisable_set_activation_status(cable_ptr.get(), ZstSyncStatus::ACTIVATED);
	m_session_events->defer([&cable_ptr](ZstSessionAdaptor* adaptor) {
		adaptor->on_cable_created(cable_ptr.get());
	});

	return cable_ptr.get();
}

void ZstSession::on_compute(ZstComponent * component, ZstInputPlug * plug) {
    try {
        component->compute(plug);
    }
    catch (std::exception e) {
        Log::net(Log::Level::error, "Compute on component {} failed. Error was: {}", component->URI().path(), e.what());
    }
}

void ZstSession::on_performer_arriving(ZstPerformer * performer)
{
	register_entity(performer);
}

void ZstSession::on_entity_arriving(ZstEntityBase * entity)
{
	register_entity(entity);
}

void ZstSession::request_entity_registration(ZstEntityBase* entity)
{
	register_entity(entity);
}

void ZstSession::register_entity(ZstEntityBase* entity)
{
	//New entities need to register the session as an adaptor to query session data
	ZstEntityBundle bundle;
	entity->get_child_entities(&bundle, true, true);
	for (auto child : bundle) {
		child->add_adaptor(ZstSessionAdaptor::downcasted_shared_from_this<ZstSessionAdaptor>());
		//child->add_adaptor(ZstEntityAdaptor::downcasted_shared_from_this<ZstEntityAdaptor>());
	}
}

bool ZstSession::observe_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	if (!entity->is_proxy()) {
		Log::net(Log::Level::warn, "Can't observe local entity {}", entity->URI().path());
		return false;
	}

	auto performer_path = entity->URI().first();
	if (listening_to_performer(performer_path))
	{
		Log::net(Log::Level::warn, "Already observing performer {}", performer_path.path());
		return false;
	}
	
	return true;
}

void ZstSession::add_connected_performer(const ZstURI& performer_path)
{
	if (performer_path.is_empty())
		return;
	m_connected_performers.emplace(performer_path);
}

void ZstSession::remove_connected_performer(const ZstURI& performer_path)
{
	if (performer_path.is_empty())
		return;

	try {
		m_connected_performers.erase(performer_path);
	}
	catch (std::out_of_range) {
		Log::net(Log::Level::warn, "Could not remove performer. Not found");
	}
}

bool ZstSession::listening_to_performer(const ZstURI& performer_path)
{
	return m_connected_performers.find(performer_path) != m_connected_performers.end();
}

void ZstSession::on_synchronisable_destroyed(ZstSynchronisable * synchronisable, bool already_removed)
{
	if (already_removed)
		return;

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

std::shared_ptr<ZstEventDispatcher<ZstSessionAdaptor> >& ZstSession::session_events()
{
	return m_session_events;
}

std::shared_ptr<ZstEventDispatcher<ZstComputeAdaptor> >& ZstSession::compute_events()
{
	return m_compute_events;
}

}
