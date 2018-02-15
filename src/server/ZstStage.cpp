#include <sstream>
#include "ZstStage.h"

//Core headers
#include <ZstVersion.h>
#include "../core/ZstMessage.h"
#include "../core/ZstMessagePool.h"

using namespace std;

ZstStage::ZstStage()
{
	m_message_pool = new ZstMessagePool();
	m_message_pool->populate(STAGE_MESSAGE_POOL_BLOCK);
}

ZstStage::~ZstStage()
{
	m_client_socket_index.clear();
	delete m_message_pool;
}

void ZstStage::init(const char * stage_name)
{
	ZstLog::init_logger(stage_name, true);
	ZstLog::info("Starting Showtime v{} stage server", SHOWTIME_VERSION);

	ZstActor::init(stage_name);
    
	std::stringstream addr;
	m_performer_router = zsock_new(ZMQ_ROUTER);
	zsock_set_linger(m_performer_router, 0);
	attach_pipe_listener(m_performer_router, s_handle_router, this);
    
	addr << "tcp://*:" << STAGE_ROUTER_PORT;
	zsock_bind(m_performer_router, "%s", addr.str().c_str());
	if (!m_performer_router) {
		ZstLog::error("Could not bind stage router socket to {}", addr.str());
		return;
	}
	addr.str("");

	addr << "@tcp://*:" << STAGE_PUB_PORT;
	m_graph_update_pub = zsock_new_pub(addr.str().c_str());
	if (!m_graph_update_pub) {
		ZstLog::error("Could not bind stage graph publisher socket to {}", addr.str());
		return;
	}
	zsock_set_linger(m_graph_update_pub, 0);

	m_heartbeat_timer_id = attach_timer(stage_heartbeat_timer_func, HEARTBEAT_DURATION, this);

	start();
}

void ZstStage::destroy()
{
	for (auto c : m_cables) {
		destroy_cable(c);
	}

	for (auto p : m_clients) {
		destroy_client(p.second);
	}
	detach_timer(m_heartbeat_timer_id);

	ZstActor::destroy();
	zsock_destroy(&m_performer_router);
	zsock_destroy(&m_graph_update_pub);
	zsys_shutdown();
	ZstLog::destroy_logger();
}


//------
//Client
//------

ZstPerformer * ZstStage::get_client(const ZstURI & path)
{
	ZstPerformer * performer = NULL;
	ZstURI first = path.first();
	if (m_clients.find(first) != m_clients.end()) {
		performer = m_clients[first];
	}
	return performer;
}

ZstPerformer * ZstStage::get_client_from_socket_id(const std::string & socket_id)
{
	ZstPerformer * performer = NULL;
	if (m_client_socket_index.find(socket_id) != m_client_socket_index.end()) {
		performer = m_client_socket_index[socket_id];
	}
	return performer;
}

std::string ZstStage::get_socket_ID(const ZstPerformer * performer)
{
	for (auto client : m_client_socket_index) {
		if (client.second == performer) {
			return client.first;
		}
	}
	return "";
}

void ZstStage::destroy_client(ZstPerformer * performer)
{
	//Nothing to do
	if (performer == NULL) {
		return;
	}

	ZstLog::info("Performer {} leaving", performer->URI().path());

	std::vector<ZstCable*> cables = get_cables_in_entity(performer);
	for (auto c : cables) {
		destroy_cable(c);
	}

	//Remove client and call all destructors in its hierarchy
	ZstPerformerMap::iterator client_it = m_clients.find(performer->URI());
	if (client_it != m_clients.end()) {
		m_clients.erase(client_it);
	}

	ZstMessage * leave_msg = msg_pool()->get()->init_message(ZstMsgKind::DESTROY_ENTITY);
	leave_msg->append_str(performer->URI().path(), performer->URI().full_size());
	publish_stage_update(leave_msg);
	delete performer;
}


//------
//Cables
//------

ZstCable * ZstStage::create_cable(const ZstCable & cable)
{
	return create_cable(cable.get_input_URI(), cable.get_output_URI());
}

ZstCable * ZstStage::create_cable(const ZstURI & input_URI, const ZstURI & output_URI)
{
	ZstCable * cable_ptr = NULL;
	cable_ptr = get_cable_by_URI(input_URI, output_URI);

	//Check to see if we already have a cable
	if (cable_ptr != NULL) {
		ZstLog::warn("Cable already exists");
		return NULL;
	}

	//Find target plugs, they shyould already exist on the graph
	int connect_status = 0;

	ZstPerformer * input_perf = get_client(input_URI);
	ZstPerformer * output_perf = get_client(output_URI);

	if (!input_perf || !output_perf) {
		ZstLog::error("Create cable could not find performer");
		return cable_ptr;
	}

	ZstPlug * input_plug = dynamic_cast<ZstPlug*>(input_perf->find_child_by_URI(input_URI));
	ZstPlug * output_plug = dynamic_cast<ZstPlug*>(output_perf->find_child_by_URI(output_URI));

	if (!input_plug || !output_plug) {
		ZstLog::error("Create cable could not find plugs");
		return cable_ptr;
	}

	//Verify plug directions are correct
	if (input_plug->direction() == ZstPlugDirection::OUT_JACK && output_plug->direction() == ZstPlugDirection::IN_JACK) {
		connect_status = 1;
	}
	else if (input_plug->direction() == ZstPlugDirection::IN_JACK && output_plug->direction() == ZstPlugDirection::OUT_JACK) {
		connect_status = 1;
	}
	else {
		ZstLog::error("Cable can't connect input to input or output to output");
		return cable_ptr;
	}

	//Finally create the cable
	cable_ptr = ZstCable::create(input_plug, output_plug);
	try {
		m_cables.insert(cable_ptr);
	} catch(std::exception e) {
		ZstLog::error("Couldn't insert cable. Reason:", e.what());
		ZstCable::destroy(cable_ptr);
		cable_ptr = NULL;
	}
	return cable_ptr;
}

int ZstStage::destroy_cable(const ZstURI & path) {
	int result = 1;
	bool fail = false;
	std::vector<ZstCable*> cables = find_cables(path);
	for (ZstCable * cable : cables) {
		if (!destroy_cable(cable))
		{
			fail = true;
		}
	}
	if (fail)
		result = 0;
	return result;
}

int ZstStage::destroy_cable(const ZstURI & input_plug, const ZstURI & output_plug) {
	return destroy_cable(get_cable_by_URI(input_plug, output_plug));
}

int ZstStage::destroy_cable(ZstCable * cable) {
	if (cable != NULL) {
		ZstLog::info("Destroying cable {} {}", cable->get_output_URI().path(), cable->get_input_URI().path());
		
		//Update rest of network
		publish_stage_update(msg_pool()->get()->init_serialisable_message(ZstMsgKind::DESTROY_CABLE, *cable));
		
		m_cables.erase(cable);
		ZstCable::destroy(cable);
		cable = 0;
		return 1;
	}
	return 0;
}

vector<ZstCable*> ZstStage::find_cables(const ZstURI & uri) {

	vector<ZstCable*> cables;
	auto it = find_if(m_cables.begin(), m_cables.end(), [&uri](ZstCable* current) {
		return current->is_attached(uri);
	});
    
    while(it++ != m_cables.end()){
        cables.push_back(*it);
    }

	return cables;
}

std::vector<ZstCable*> ZstStage::get_cables_in_entity(ZstEntityBase * entity)
{
	std::vector<ZstCable*> cables;
	for (auto cable : m_cables) {
		if (cable->get_input_URI().contains(entity->URI()) || cable->get_output_URI().contains(entity->URI())) {
			cables.push_back(cable);
		}
	}
	return cables;
}

ZstCable * ZstStage::get_cable_by_URI(const ZstURI & input, const ZstURI & output) {
	ZstCable * cable = NULL;
	if (m_cables.size()) {
		ZstCable search_cable = ZstCable(input, output);
		auto cable_it = m_cables.find(&search_cable);
		if (cable_it != m_cables.end()) {
			cable = *cable_it;
		}
	}
	return cable;
}



//------------------------
//Incoming socket handlers
//------------------------

int ZstStage::s_handle_router(zloop_t * loop, zsock_t * socket, void * arg)
{
	ZstStage *stage = (ZstStage*)arg;
	zframe_t * identity_frame = NULL;
	std::string sender_identity;
	ZstPerformer * sender = NULL;

	//Receive waiting message
	zmsg_t * recv_msg = zmsg_recv(socket);
	ZstMessage * msg = NULL;
	
	if (recv_msg) {
		//Get identity of sender from first frame
		zframe_t * identity_frame = zmsg_pop(recv_msg);
		sender_identity = std::string((char*)zframe_data(identity_frame), zframe_size(identity_frame));
		zframe_t * empty = zmsg_pop(recv_msg);
		zframe_destroy(&empty);

		msg = stage->msg_pool()->get();
		msg->unpack(recv_msg);
	}
	
	//If we're dealing with a new client, we don't need to search for it
	if (msg->kind() != ZstMsgKind::CLIENT_JOIN) {
		sender = stage->get_client_from_socket_id(sender_identity);
		if (!sender) {
			//If the sender hasn't joined yet, ignore the message
			return 0;
		}
	}
	
    //Message type handling
	ZstMessage * response = NULL;
	switch (msg->kind()) {
		case ZstMsgKind::CLIENT_SYNC:
		{
			response = stage->create_snapshot(sender);
			break;
		}
		case ZstMsgKind::CLIENT_LEAVING:
		{
			response = stage->destroy_client_handler(sender);
			break;
		}
		case ZstMsgKind::CLIENT_HEARTBEAT:
		{
			sender->set_heartbeat_active();
			break;
		}
		case ZstMsgKind::CLIENT_JOIN:
		{
			response = stage->create_client_handler(sender_identity, msg);
			sender = stage->get_client_from_socket_id(sender_identity);
			break;
		}
		case ZstMsgKind::CREATE_COMPONENT:
		{
			response = stage->create_entity_handler<ZstComponent>(msg, sender);
			break;
		}
		case ZstMsgKind::CREATE_CONTAINER:
		{
			response = stage->create_entity_handler<ZstContainer>(msg, sender);
			break;
		}
		case ZstMsgKind::CREATE_PLUG:
		{
			response = stage->create_entity_handler<ZstPlug>(msg, sender);
			break;
		}
		case ZstMsgKind::CREATE_CABLE:
		{
			response = stage->create_cable_handler(msg);
			break;
		}
		case ZstMsgKind::CREATE_ENTITY_FROM_TEMPLATE:
		{
			response = stage->create_entity_from_template_handler(msg);
			break;
		}
		case ZstMsgKind::DESTROY_ENTITY:
		{
			response = stage->destroy_entity_handler(msg);
			break;
		}
		case ZstMsgKind::DESTROY_PLUG:
		{
			response = stage->destroy_entity_handler(msg);
			break;
		}
		case ZstMsgKind::DESTROY_CABLE:
		{
			response = stage->destroy_cable_handler(msg);
			break;
		}
		default:
		{
			ZstLog::error("Didn't understand received message type of {}", msg->kind());
			response = stage->msg_pool()->get()->init_message(ZstMsgKind::ERR_STAGE_MSG_TYPE_UNKNOWN);
			break;
		}
	}

	//Send ack
	if (response) {
		//Copy ID of the original message so we can match this message to a promise on the client
		//once upon a time there was a null pointer, it pointed to nothing.
		response->copy_id(msg);
		stage->send_to_client(response, sender);
	}

	//Cleanup
	zframe_destroy(&identity_frame);
			
	return 0;
}


//--------------------
//Client communication
//--------------------

void ZstStage::send_to_client(ZstMessage * msg, ZstPerformer * destination) {
	
	zmsg_t * msg_handle = msg->handle();
	zframe_t * identity = zframe_from(get_socket_ID(destination).c_str());
	zframe_t * empty = zframe_new_empty();
	zmsg_prepend(msg_handle, &empty);
	zmsg_prepend(msg_handle, &identity);
	msg->send(m_performer_router);
	msg_pool()->release(msg);
}


// ----------------
// Message handlers
// ----------------

ZstMessage * ZstStage::create_client_handler(std::string sender_identity, ZstMessage * msg)
{
	ZstMessage * response = msg_pool()->get();

	//Copy the id of the message so the sender will eventually match the response to a message promise
	ZstPerformer client = msg->ZstMessage::unpack_payload_serialisable<ZstPerformer>(0);

	ZstLog::info("Registering new client {}", client.URI().path());

	//Only one client with this UUID at a time
	if (get_client(client.URI())) {
		ZstLog::warn("Client already exists ", client.URI().path());
		return response->init_message(ZstMsgKind::ERR_STAGE_PERFORMER_ALREADY_EXISTS);
	}

	//Copy streamable so we have a local ptr for the client
	ZstPerformer * client_proxy = new ZstPerformer(client);
	assert(client_proxy);
	
	//Save our new client
	m_clients[client_proxy->URI()] = client_proxy;
	m_client_socket_index[std::string(sender_identity)] = client_proxy;
		
	//Update rest of network
	publish_stage_update(msg_pool()->get()->init_entity_message(client_proxy));
	
	return response->init_message(ZstMsgKind::OK);
}

ZstMessage * ZstStage::destroy_client_handler(ZstPerformer * performer)
{
	destroy_client(performer);
	return msg_pool()->get()->init_message(ZstMsgKind::OK);
}

template ZstMessage * ZstStage::create_entity_handler<ZstPlug>(ZstMessage * msg, ZstPerformer * performer);
template ZstMessage * ZstStage::create_entity_handler<ZstComponent>(ZstMessage * msg, ZstPerformer * performer);
template ZstMessage * ZstStage::create_entity_handler<ZstContainer>(ZstMessage * msg, ZstPerformer * performer);
template <typename T>
ZstMessage * ZstStage::create_entity_handler(ZstMessage * msg, ZstPerformer * performer) {
	
	ZstMessage * response = msg_pool()->get();
	
	if (!performer) {
		return response->init_message(ZstMsgKind::ERR_STAGE_PERFORMER_NOT_FOUND);
	}

	ZstLog::debug("Found performer, unpacking entity");

	T entity = msg->unpack_payload_serialisable<T>(0);

	//Make sure this entity doesn't already exist
	if (performer->find_child_by_URI(entity.URI())) {
		//Entity already exists
		ZstLog::warn("Entity already exists! {}", entity.URI().path());
		return response->init_message(ZstMsgKind::ERR_STAGE_ENTITY_ALREADY_EXISTS);
	}

    ZstLog::debug("Entity {} doesn't exist yet, registering it now", entity.URI().path());
	
	//Find the parent for this entity
	ZstURI parent_path = entity.URI().parent();
	ZstEntityBase * parent = NULL;
	if (parent_path.size() == 1) {
		parent = performer;
	}
	else {
		parent = dynamic_cast<T*>(performer->find_child_by_URI(parent_path));
	}

	//If we can't find the parent this entity says it belongs to then we have a problem
	if (parent == NULL) {
        ZstLog::warn("Couldn't register entity {}. No parent found at {}", entity.URI().path(), parent_path.path());
		return response->init_message(ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND);
	}

	ZstLog::debug("Found parent of entity. ");

	//Copy streamable so we have a local ptr for the entity
	T* entity_proxy = new T(entity);
	assert(entity_proxy);

	//Handle plugs and entities differently (for now)
	if (strcmp(entity_proxy->entity_type(), PLUG_TYPE) == 0) {
		dynamic_cast<ZstComponent*>(parent)->add_plug(dynamic_cast<ZstPlug*>(entity_proxy));
	}
	else {
		ZstContainer * parent_container = dynamic_cast<ZstContainer*>(parent);
		assert(parent_container);
		parent_container->add_child(entity_proxy);
	}

	ZstLog::info("Registering new entity {}", entity_proxy->URI().path());
	
	//Update rest of network
	publish_stage_update(msg_pool()->get()->init_entity_message(entity_proxy));

	return response->init_message(ZstMsgKind::OK);
}

ZstMessage * ZstStage::destroy_entity_handler(ZstMessage * msg)
{
	ZstMessage * response = msg_pool()->get();

	//Unpack entity to destroy from message
	ZstURI entity_path = ZstURI(msg->payload_at(0).data(), msg->payload_at(0).size());
	
	ZstLog::info("Destroying entity {}", entity_path.path());

	//Find owner of entity
	ZstPerformer * owning_performer = get_client(entity_path.first());

	if (!owning_performer) {
		ZstLog::error("Could not find performer for destroyed entity {}", entity_path.path());
		return response->init_message(ZstMsgKind::ERR_STAGE_PERFORMER_NOT_FOUND);
	}

	//Find existing entity
	ZstEntityBase * entity = owning_performer->find_child_by_URI(entity_path);
	if (!entity) {
		return response->init_message(ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND);
	}

	//Handle plugs and entities differently (for now)
	if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		dynamic_cast<ZstComponent*>(entity->parent())->remove_plug(dynamic_cast<ZstPlug*>(entity));
	}
	else {
		dynamic_cast<ZstContainer*>(entity->parent())->remove_child(entity);
	}

	//Remove all cables linked to this entity
	std::vector<ZstCable*> cables = get_cables_in_entity(entity);
	for (auto c : cables) {
		ZstLog::info("Removing cables linked to leaving entity");
		destroy_cable(c);
	}

	//Update rest of network
	ZstMessage * stage_update_msg = msg_pool()->get()->init_message(msg->kind());
	stage_update_msg->append_str(entity_path.path(), entity_path.full_size());
	publish_stage_update(stage_update_msg);

	delete entity;
	return response->init_message(ZstMsgKind::OK);
}


ZstMessage * ZstStage::create_entity_template_handler(ZstMessage * msg)
{
	//TODO: Implement this
    throw std::logic_error("Creating entity types not implemented yet");
	return msg_pool()->get()->init_message(ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND);
}


ZstMessage * ZstStage::create_entity_from_template_handler(ZstMessage * msg)
{
    throw std::logic_error("Creating entity types not implemented yet");
	return msg_pool()->get()->init_message(ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND);
}

ZstMessage * ZstStage::create_cable_handler(ZstMessage * msg)
{
	ZstMessage * response = msg_pool()->get();

	//Unpack cable from message
	const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(0);
	ZstLog::info("Received connect cable request for In:{} and Out:{}", cable.get_input_URI().path(), cable.get_output_URI().path());

	//Create cable 
	ZstCable * cable_ptr = create_cable(cable);

	if (!cable_ptr) {
		return response->init_message(ZstMsgKind::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST);
	}

	//Create connection request for the entity who owns the input plug
	ZstPerformer * input_performer = get_client(cable_ptr->get_input_URI());
	ZstPerformer * output_performer = get_client(cable_ptr->get_output_URI());

	ZstLog::info("Sending cable connection request to {}", input_performer->URI().path());

	ZstMessage * connection_msg = msg_pool()->get()->init_message(ZstMsgKind::SUBSCRIBE_TO_PERFORMER);
	connection_msg->append_str(output_performer->address(), strlen(output_performer->address()));	//IP of output client
	connection_msg->append_str(cable.get_output_URI().path(), cable.get_output_URI().full_size());	//Output plug path
	send_to_client(connection_msg, input_performer);

	//Update rest of network
	publish_stage_update(msg_pool()->get()->init_serialisable_message(msg->kind(), cable));

    return response->init_message(ZstMsgKind::OK);
}

ZstMessage * ZstStage::destroy_cable_handler(ZstMessage * msg)
{
	ZstMessage * response = msg_pool()->get();
	const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(0);
	ZstLog::info("Received destroy cable connection request");
	
	ZstCable * cable_ptr = get_cable_by_URI(cable.get_input_URI(), cable.get_output_URI());

	if (!destroy_cable(cable_ptr)) {
		response->init_message(ZstMsgKind::ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST);
	}

	return response->init_message(ZstMsgKind::OK);
}


//---------------------
// Outgoing event queue
//---------------------


ZstMessage * ZstStage::create_snapshot(ZstPerformer * client) {

	ZstMessage * snapshot = msg_pool()->get()->init_message(ZstMsgKind::GRAPH_SNAPSHOT);
	
	//Pack performer root entities
	for (auto performer : m_clients) {
		//Only pack performers that aren't the destination client
		if (performer.second->URI() != client->URI()) {
			snapshot->append_serialisable(ZstMsgKind::CREATE_PERFORMER, *(performer.second));
			ZstLog::debug("Adding performer {} to snapshot", performer.second->URI().path());
		}
	}
	
	//Pack cables
	for (auto cable : m_cables) {
		snapshot->append_serialisable(ZstMsgKind::CREATE_CABLE, *cable);
		ZstLog::debug("Adding cable {}-{} to snapshot", cable->get_output_URI().path(), cable->get_input_URI().path());
	}

	ZstLog::info("Sending graph snapshot to {}", client->URI().path());
	
    return snapshot;
}

void ZstStage::publish_stage_update(ZstMessage * msg)
{
	msg->send(m_graph_update_pub);
}

int ZstStage::stage_heartbeat_timer_func(zloop_t * loop, int timer_id, void * arg)
{
	ZstStage * stage = (ZstStage*)arg;
	ZstPerformerMap clients = stage->m_clients;
	std::vector<ZstPerformer*> removed_clients;
	for (auto performer_it : stage->m_clients) {
		ZstPerformer * performer = performer_it.second;
		if (performer->get_active_heartbeat()) {
			performer->clear_active_hearbeat();
		}
		else {
			ZstLog::warn("Client {} missed a heartbeat. {} remaining", performer->URI().path(), MAX_MISSED_HEARTBEATS - performer->get_missed_heartbeats());
			performer->set_heartbeat_inactive();
		}

		if (performer->get_missed_heartbeats() > MAX_MISSED_HEARTBEATS) {
			removed_clients.push_back(performer);
		}
	}

	for (auto client : removed_clients) {
		stage->destroy_client(client);
	}
	return 0;
}

ZstMessagePool * ZstStage::msg_pool()
{
	return m_message_pool;
}
