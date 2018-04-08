#include <chrono>
#include <sstream>

#include "ZstClient.h"

using namespace std;

ZstClient::ZstClient() :
    m_ping(-1),
    m_is_ending(false),
    m_is_destroyed(false),
    m_init_completed(false),
    m_connected_to_stage(false)
{
	//Events and callbacks
	m_client_connected_event_manager = new ZstEventQueue();
	add_event_queue(m_client_connected_event_manager);

	m_client_disconnected_event_manager = new ZstEventQueue();
	add_event_queue(m_client_disconnected_event_manager);

	m_compute_event_manager = new ZstEventQueue();
	m_compute_event = new ZstComputeEvent();
	m_compute_event_manager->attach_event_listener(m_compute_event);
	add_event_queue(m_compute_event_manager);
	
	m_synchronisable_event_manager = new ZstEventQueue();
	m_synchronisable_deferred_event = new ZstSynchronisableDeferredEvent();
	m_synchronisable_event_manager->attach_event_listener(m_synchronisable_deferred_event);
	add_event_queue(m_synchronisable_event_manager);

	//CLient modules
	m_hierarchy = new ZstHierarchy(this);
	m_cable_network = new ZstCableNetwork(this);
	m_transport = new ZstCZMQTransportLayer(this);
	m_msg_dispatch = new ZstMessageDispatcher(this, m_transport);
}

ZstClient::~ZstClient() {
	destroy();

	m_synchronisable_event_manager->remove_event_listener(m_synchronisable_deferred_event);
	m_compute_event_manager->remove_event_listener(m_compute_event);
	remove_event_queue(m_synchronisable_event_manager);
	remove_event_queue(m_compute_event_manager);
	
	delete m_synchronisable_deferred_event;
	delete m_compute_event;
	delete m_synchronisable_event_manager;
	delete m_compute_event_manager;

	m_hierarchy->destroy();
	m_cable_network->destroy();
	m_msg_dispatch->destroy();
	m_transport->destroy();

	delete m_hierarchy;
	delete m_cable_network;
	delete m_msg_dispatch;
	delete m_transport;
}

ZstClient & ZstClient::instance()
{
	static ZstClient client_singleton;
	return client_singleton;
}

void ZstClient::destroy() {
	//Only need to call cleanup once
	if (m_is_ending || m_is_destroyed)
		return;
    m_is_ending = true;
    m_init_completed = false;
    
    //Let stage know we are leaving
	if(is_connected_to_stage())
		leave_stage(true);
	
	ZstActor::destroy();
	
	m_is_ending = false;
	m_is_destroyed = true;
}

void ZstClient::init_client(const char *client_name, bool debug)
{
	if (m_is_ending || m_init_completed) {
		return;
	}

	ZstLog::init_logger(client_name);
	ZstLog::net(LogLevel::notification, "Starting Showtime v{}", SHOWTIME_VERSION);

	ZstActor::init(client_name);
	m_client_name = client_name;
	m_is_destroyed = false;

	m_hierarchy->init(client_name);
	m_cable_network->init();
	m_transport->init();
		
    start();
    m_init_completed = true;
}

void ZstClient::init_file_logging(const char * log_file_path)
{
	ZstLog::init_file_logging(log_file_path);
}

void ZstClient::process_callbacks()
{
    if(!is_init_complete() || m_is_destroyed){
        return;
    }

	ZstEventDispatcher::process_callbacks();

	m_hierarchy->process_callbacks();
	m_cable_network->process_callbacks();
	
	//Only delete entities after all callbacks have finished
	m_reaper.reap_all();
}

void ZstClient::flush()
{
	ZstEventDispatcher::flush();
	m_hierarchy->flush();
	m_cable_network->flush();
}

void ZstClient::start() {
	ZstActor::start();
}

void ZstClient::stop() {
	ZstActor::stop();
}


// -------------
// Client join
// -------------

void ZstClient::join_stage(std::string stage_address, bool async) {
	
	if (m_is_connecting || m_connected_to_stage) {
		ZstLog::net(LogLevel::error, "Connection in progress or already connected, can't connect to stage.");
		return;
	}
	m_is_connecting = true;
	
	ZstLog::net(LogLevel::notification, "Connecting to stage {}", stage_address);
	m_transport->connect_to_stage(stage_address);
    
	ZstMessage * msg = m_msg_dispatch->init_serialisable_message(ZstMsgKind::CLIENT_JOIN, *m_hierarchy->get_local_performer());
	msg->append_str(stage_address.c_str(), stage_address.size());
	m_msg_dispatch->send_to_stage(msg, async, [this](ZstMessageReceipt response) { this->join_stage_complete(response); });
	
	//Run callbacks if we're still on the main app thread
	if (!async) process_callbacks();
}

void ZstClient::join_stage_complete(ZstMessageReceipt response)
{
	m_is_connecting = false;

	//If we didn't receive a OK signal, something went wrong
	if (response.status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Stage connection failed with with status: {}", response.status);
        return;
	}

	ZstLog::net(LogLevel::notification, "Connection to server established");

	//Set up heartbeat timer
	m_heartbeat_timer_id = attach_timer(s_heartbeat_timer, HEARTBEAT_DURATION, this);

	//TODO: Need a handshake with the stage before we mark connection as active
	m_connected_to_stage = true;

	//Enqueue connection events
	client_connected_events()->enqueue(m_hierarchy->get_local_performer());

	m_hierarchy->synchronise_graph(response.async);
}

void ZstClient::leave_stage(bool async)
{
	if (m_connected_to_stage) {
		ZstLog::net(LogLevel::notification, "Leaving stage");
        
        ZstMessage * msg = m_msg_dispatch->init_message(ZstMsgKind::CLIENT_LEAVING);
		m_msg_dispatch->send_to_stage(msg, async, [this](ZstMessageReceipt response) {this->leave_stage_complete(); });

		//Run callbacks if we're still on the main app thread
		if(!async) process_callbacks();
    } else {
        ZstLog::net(LogLevel::debug, "Not connected to stage. Skipping to cleanup. {}");
        leave_stage_complete();
		if (!async) process_callbacks();
    }
}

void ZstClient::leave_stage_complete()
{
    //Deactivate all owned entities
    m_hierarchy->get_local_performer()->enqueue_deactivation();
    
    //Disconnect rest of sockets and timers
	detach_timer(m_heartbeat_timer_id);
	m_transport->disconnect_from_stage();

	m_is_connecting = false;
	m_connected_to_stage = false;
}

bool ZstClient::is_connected_to_stage()
{
	return m_connected_to_stage;
}

bool ZstClient::is_connecting_to_stage()
{
	return m_is_connecting;
}

bool ZstClient::is_init_complete() {
    return m_init_completed;
}

long ZstClient::ping()
{
	return m_ping;
}


// ---------------
// Socket handlers
// ---------------


int ZstClient::graph_message_handler(zmsg_t * msg) {
	//Get sender from msg
	char * sender_c = zmsg_popstr(msg);
	ZstURI sender(sender_c);
	zstr_free(&sender_c);

	//Get payload from msg
	zframe_t * payload = zmsg_pop(msg);

	//Find local proxy of the sneding plug
	ZstPlug * sending_plug = dynamic_cast<ZstPlug*>(hierarchy()->find_entity(sender));
	ZstInputPlug * receiving_plug = NULL;

	if (!sending_plug) {
		ZstLog::net(LogLevel::warn, "No sending plug found");
		return -1;
	}

	//Iterate over all connected cables from the sending plug
	for (auto cable : *sending_plug) {
		receiving_plug = dynamic_cast<ZstInputPlug*>(cable->get_input());
		if (receiving_plug) {
			if (hierarchy()->entity_is_local(*receiving_plug)) {
				//TODO: Lock plug value when deserialising
				size_t offset = 0;
				receiving_plug->raw_value()->read((char*)zframe_data(payload), zframe_size(payload), offset);
				compute_events()->enqueue(receiving_plug);
			}
		}
	}
	
	//Cleanup
	zframe_destroy(&payload);

	return 0;
}



void ZstClient::stage_update_handler(ZstMessage * msg)
{
	ZstMsgKind payload_kind = msg->kind();

	for (size_t i = 0; i < msg->num_payloads(); ++i)
	{
		if (msg->kind() == ZstMsgKind::GRAPH_SNAPSHOT) {
			payload_kind = msg->payload_at(i).kind();
		}
		else {
			payload_kind = (msg->kind());
		}

		//Do something with payload
		switch (payload_kind) {
		case ZstMsgKind::CREATE_CABLE:
		{
			const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(i);
			//Only dispatch cable event if we don't already have the cable (we might have created it)
			if(!cable_network()->find_cable(cable.get_input_URI(), cable.get_output_URI())) {
				ZstCable * cable_ptr = cable_network()->create_cable(cable);
                cable_ptr->set_activation_status(ZstSyncStatus::ACTIVATED);
				cable_network()->cable_arriving_events()->enqueue(cable_ptr);
			}
			break;
		}
		case ZstMsgKind::DESTROY_CABLE:
		{
			const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(i);
			ZstCable * cable_ptr = cable_network()->find_cable(cable.get_input_URI(), cable.get_output_URI());
			if (cable_ptr) {
				if (cable_ptr->is_activated()) {
					cable_ptr->enqueue_deactivation();
					cable_network()->cable_leaving_events()->enqueue(cable_ptr);
				}
			}
			break;
		}
		case ZstMsgKind::CREATE_PLUG:
		{
			ZstPlug plug = msg->unpack_payload_serialisable<ZstPlug>(i);
			hierarchy()->add_proxy_entity(plug);
			break;
		}
		case ZstMsgKind::CREATE_PERFORMER:
		{
			ZstPerformer performer = msg->unpack_payload_serialisable<ZstPerformer>(i);
			hierarchy()->add_performer(performer);
			break;
		}
		case ZstMsgKind::CREATE_COMPONENT:
		{
			ZstComponent component = msg->unpack_payload_serialisable<ZstComponent>(i);
			hierarchy()->add_proxy_entity(component);
			break;
		}
		case ZstMsgKind::CREATE_CONTAINER:
		{
			ZstContainer container = msg->unpack_payload_serialisable<ZstContainer>(i);
			hierarchy()->add_proxy_entity(container);
			break;
		}
		case ZstMsgKind::DESTROY_ENTITY:
		{
			ZstURI entity_path = ZstURI((char*)msg->payload_at(i).data(), msg->payload_at(i).size());
			
			//Only dispatch entity leaving events for non-local entities (for the moment)
			if (!hierarchy()->path_is_local(entity_path)) {
				ZstEntityBase * entity = hierarchy()->find_entity(entity_path);
                if(entity){
                    entity->enqueue_deactivation();
                    if (strcmp(entity->entity_type(), COMPONENT_TYPE) == 0 || strcmp(entity->entity_type(), CONTAINER_TYPE) == 0) {
						hierarchy()->component_leaving_events()->enqueue(entity);
                    } else if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
						hierarchy()->plug_leaving_events()->enqueue(entity);
                    } else if (strcmp(entity->entity_type(), PERFORMER_TYPE) == 0) {
						hierarchy()->performer_leaving_events()->enqueue(entity);
                    }
                }
			}
			
			break;
		}
		case ZstMsgKind::REGISTER_COMPONENT_TEMPLATE:
		{
			throw std::logic_error("Handler for component type arriving mesages not implemented");
			break;
		}
		case ZstMsgKind::UNREGISTER_COMPONENT_TEMPLATE:
		{
			throw std::logic_error("Handler for component type leaving mesages not implemented");
			break;
		}
		default:
			ZstLog::net(LogLevel::notification, "Didn't understand message type of {}", payload_kind);
			throw std::logic_error("Didn't understand message type");
			break;
		}
	}
}

int ZstClient::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
	ZstClient * client = (ZstClient*)arg;
	chrono::time_point<chrono::system_clock> start = std::chrono::system_clock::now();

	ZstMessage * msg = client->msg_dispatch()->init_message(ZstMsgKind::CLIENT_HEARTBEAT);
	client->msg_dispatch()->send_to_stage(msg, true, [client, &start](ZstMessageReceipt response) {
		chrono::time_point<chrono::system_clock> end = chrono::system_clock::now();
		chrono::milliseconds delta = chrono::duration_cast<chrono::milliseconds>(end - start);
		ZstLog::net(LogLevel::notification, "Ping roundtrip {} ms", delta.count());
		client->m_ping = static_cast<long>(delta.count());
	});

	return 0;
}

ZstEventQueue * ZstClient::client_connected_events()
{
	return m_client_connected_event_manager;
}

ZstEventQueue * ZstClient::client_disconnected_events()
{
	return m_client_connected_event_manager;
}

ZstEventQueue * ZstClient::compute_events()
{
	return m_compute_event_manager;
}

void ZstClient::enqueue_synchronisable_event(ZstSynchronisable * synchronisable)
{
	m_synchronisable_event_manager->enqueue(synchronisable);
}

void ZstClient::enqueue_synchronisable_deletion(ZstSynchronisable * synchronisable)
{
	m_reaper.add(synchronisable);
}

void ZstClient::send_to_performance(ZstPlug * plug)
{
	msg_dispatch()->send_to_performance(plug);
}

ZstHierarchy * ZstClient::hierarchy()
{
	return m_hierarchy;
}

ZstCableNetwork * ZstClient::cable_network()
{
	return m_cable_network;
}

ZstMessageDispatcher * ZstClient::msg_dispatch()
{
	return m_msg_dispatch;
}
