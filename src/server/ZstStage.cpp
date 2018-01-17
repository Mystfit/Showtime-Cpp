#include <sstream>
#include "ZstStage.h"
#include "ZstURI.h"
#include "ZstCable.h"
#include "entities/ZstPerformer.h"

//Core headers
#include "../core/ZstUtils.hpp"
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
	delete m_message_pool;
	detach_timer(m_heartbeat_timer_id);
	detach_timer(m_heartbeat_timer_id);
	ZstActor::~ZstActor();
	//Close stage pipes
	zsock_destroy(&m_performer_router);
	zsock_destroy(&m_graph_update_pub);
}

void ZstStage::init()
{
	ZstActor::init();
    
	std::stringstream addr;
	m_performer_router = zsock_new(ZMQ_ROUTER);
	zsock_set_linger(m_performer_router, 0);
	attach_pipe_listener(m_performer_router, s_handle_router, this);
    
	addr << "tcp://*:" << STAGE_ROUTER_PORT;
	zsock_bind(m_performer_router, "%s", addr.str().c_str());
	addr.str("");

	addr << "@tcp://*:" << STAGE_PUB_PORT;
	m_graph_update_pub = zsock_new_pub(addr.str().c_str());
	zsock_set_linger(m_graph_update_pub, 0);

	m_heartbeat_timer_id = attach_timer(stage_heartbeat_timer_func, HEARTBEAT_DURATION, this);

	start();
}


//------
//Client
//------

ZstPerformer * ZstStage::get_client(const ZstURI & path)
{
	ZstPerformer * performer = NULL;
	if (m_clients.find(path) != m_clients.end()) {
		performer = m_clients[path];
	}
	return performer;
}

ZstPerformer * ZstStage::get_client(const std::string & socket_id)
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

	std::cout << "ZST_STAGE: Performer " << performer->URI().path() << " leaving" << std::endl;

	std::vector<ZstCable*> cables = get_cables_in_entity(performer);
	for (auto c : cables) {
		destroy_cable(c);
	}

	//Remove client and call all destructors in its hierarchy
	std::unordered_map<ZstURI, ZstPerformer*>::iterator client_it = m_clients.find(performer->URI());
	if (client_it != m_clients.end()) {
		m_clients.erase(client_it);
	}

	ZstMessage * leave_msg = msg_pool()->get()->init_message(ZstMessage::Kind::CLIENT_LEAVE);
	leave_msg->append_str(performer->URI().path());
	publish_stage_update(leave_msg);
	delete performer;
}


//------
//Cables
//------

ZstCable * ZstStage::create_cable(const ZstURI & a, const ZstURI & b)
{
	ZstCable * cable_ptr = NULL;
	cable_ptr = get_cable_by_URI(a, b);

	//Check to see if we already have a cable
	if (cable_ptr != NULL) {
		cout << "ZST_STAGE: Cable already exists" << endl;
		return NULL;
	}

	//Find target plugs, they shyould already exist on the graph
	int connect_status = 0;
	ZstPlug * input_plug = get_client(cable_ptr->get_input_URI())->get_plug_by_URI(cable_ptr->get_input_URI());
	ZstPlug * output_plug = get_client(cable_ptr->get_output_URI())->get_plug_by_URI(cable_ptr->get_output_URI());

	//Verify plug directions are correct
	if (input_plug && output_plug) {
		if (input_plug->direction() == ZstPlugDirection::OUT_JACK && output_plug->direction() == ZstPlugDirection::IN_JACK) {
			connect_status = 1;
		}
		else if (input_plug->direction() == ZstPlugDirection::IN_JACK && output_plug->direction() == ZstPlugDirection::OUT_JACK) {
			connect_status = 1;
		}
	}

	if (!connect_status) {
		cout << "ZST_STAGE: Cable plug direction mismatch" << endl;
		return NULL;
	}

	cable_ptr = new ZstCable(input_plug, output_plug);

	//Cable verified. Store it
	m_cables.push_back(cable_ptr);
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

int ZstStage::destroy_cable(const ZstURI & output_plug, const ZstURI & input_plug) {
	return destroy_cable(get_cable_by_URI(output_plug, input_plug));
}

int ZstStage::destroy_cable(ZstCable * cable) {
	if (cable != NULL) {
		cout << "ZST_STAGE: Destroying cable " << cable->get_output_URI().path() << " " << cable->get_input_URI().path() << endl;
		for (vector<ZstCable*>::iterator cable_iter = m_cables.begin(); cable_iter != m_cables.end(); ++cable_iter) {
			if ((*cable_iter) == cable) {
				m_cables.erase(cable_iter);
				break;
			}
		}

		delete cable;
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

	for (it; it != m_cables.end(); ++it) {
		cables.push_back((*it));
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

ZstCable * ZstStage::get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB) {

	auto it = find_if(m_cables.begin(), m_cables.end(), [&uriA, &uriB](ZstCable * current) {
		return current->is_attached(uriA, uriB);
	});

	if (it != m_cables.end()) {
		return (*it);
	}
	return NULL;
}



//------------------------
//Incoming socket handlers
//------------------------

int ZstStage::s_handle_router(zloop_t * loop, zsock_t * socket, void * arg)
{
	ZstStage *stage = (ZstStage*)arg;
	ZstPerformer * sender = NULL;

	//Receive waiting message
	zmsg_t * recv_msg = zmsg_recv(socket);
	ZstMessage * msg = NULL;

	if (recv_msg) {
		msg = stage->msg_pool()->get();
		msg->unpack(recv_msg);
	}
	
	//If we're dealing with a new client, we don't need to search for it
	if (msg->kind() != ZstMessage::Kind::CLIENT_JOIN) {
		sender = stage->get_client(msg->sender_as_URI());
	}
	
	bool send_reply = true;
	bool announce_creation = true;

	ZstMessage * response = NULL;

	//Message type handling
	switch (msg->kind()) {
		case ZstMessage::Kind::CLIENT_SYNC:
		{
			stage->send_snapshot(sender);
			break;
		}
		case ZstMessage::Kind::CLIENT_LEAVING:
		{
			stage->destroy_client_handler(sender);
			break;
		}
		case ZstMessage::Kind::CLIENT_HEARTBEAT:
		{
			sender->set_heartbeat_active();
			break;
		}
		case ZstMessage::Kind::CLIENT_JOIN:
		{
			response = stage->create_client_handler(msg->sender(), msg);
			break;
		}
		case ZstMessage::Kind::CREATE_COMPONENT:
		{
			response = stage->create_entity_handler<ZstComponent>(msg, sender);
			break;
		}
		case ZstMessage::Kind::CREATE_CONTAINER:
		{
			response = stage->create_entity_handler<ZstContainer>(msg, sender);
			break;
		}
		case ZstMessage::Kind::CREATE_PLUG:
		{
			response = stage->create_entity_handler<ZstPlug>(msg, sender);
			break;
		}
		case ZstMessage::Kind::CREATE_CABLE:
		{
			response = stage->create_cable_handler(msg);
			break;
		}
		case ZstMessage::Kind::CREATE_ENTITY_FROM_TEMPLATE:
		{
			response = stage->create_entity_from_template_handler(msg);
			break;
		}
		case ZstMessage::Kind::DESTROY_ENTITY:
		{
			response = stage->destroy_entity_handler(msg);
			break;
		}
		case ZstMessage::Kind::DESTROY_PLUG:
		{
			response = stage->destroy_entity_handler(msg);
			break;
		}
		case ZstMessage::Kind::DESTROY_CABLE:
		{
			response = stage->destroy_cable_handler(msg);
			break;
		}
		default:
		{
			cout << "ZST_STAGE: Didn't understand received message type of " << msg->kind() << endl;
			response->init_message(ZstMessage::Kind::ERR_STAGE_MSG_TYPE_UNKNOWN);
			break;
		}
	}

	//Send ack
	if (response) {
		//Copy ID of the original message so we can match this message to a promise on the client
		response->copy_id(msg);
		stage->send_to_client(response, sender);
	}
			
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

ZstMessage * ZstStage::create_client_handler(std::string sender, ZstMessage * msg)
{
	ZstMessage * response = msg_pool()->get();

	//Copy the id of the message so the sender will eventually match the response to a message promise
	ZstPerformer * client = msg->ZstMessage::unpack_payload_entity<ZstPerformer>(0);
	cout << "ZST_STAGE: Registering new client " << client->URI().path() << endl;

	//Only one client with this UUID at a time
	if (get_client(client->URI())) {
		cout << "ZST_STAGE: Client already exists " << client->URI().path() << endl;
		return response->init_message(ZstMessage::Kind::ERR_STAGE_PERFORMER_ALREADY_EXISTS);
	}
	
	//Save our new client
	m_clients[client->URI()] = client;
	m_client_socket_index[sender] = client;
		
	//Update rest of network
	publish_stage_update(msg_pool()->get()->init_entity_message(client));
	
	return response->init_message(ZstMessage::Kind::OK);
}

void ZstStage::destroy_client_handler(ZstPerformer * performer)
{
	destroy_client(performer);
	send_to_client(msg_pool()->get()->init_message(ZstMessage::Kind::OK), performer);
}

template ZstMessage * ZstStage::create_entity_handler<ZstPlug>(ZstMessage * msg, ZstPerformer * performer);
template ZstMessage * ZstStage::create_entity_handler<ZstComponent>(ZstMessage * msg, ZstPerformer * performer);
template ZstMessage * ZstStage::create_entity_handler<ZstContainer>(ZstMessage * msg, ZstPerformer * performer);
template <typename T>
ZstMessage * ZstStage::create_entity_handler(ZstMessage * msg, ZstPerformer * performer) {
	
	ZstMessage * response = msg_pool()->get();
	
	if (!performer) {
		return response->init_message(ZstMessage::Kind::ERR_STAGE_PERFORMER_NOT_FOUND);
	}
	
	T* entity = msg->unpack_payload_entity<T>(0);

	//Make sure this entity doesn't already exist
	if (performer->find_child_by_URI(entity->URI())) {
		//Entity already exists
		std::cout << "ZST_STAGE: Entity already exists! " << entity->URI().path() << std::endl;
		delete entity;
		return response->init_message(ZstMessage::Kind::ERR_STAGE_ENTITY_ALREADY_EXISTS);
	}
	
	//Find the parent for this entity
	ZstURI parent_path = entity->URI().range(0, entity->URI().size() - 1);
	ZstEntityBase * parent = dynamic_cast<T*>(performer->find_child_by_URI(parent_path));

	//If we can't find the parent this entity says it belongs to then we have a problem
	if (parent == NULL) {
        std::cout << "ZST_STAGE: Couldn't register entity. No parent found at " << parent_path.path() << std::endl;
		return response->init_message(ZstMessage::Kind::ERR_STAGE_ENTITY_NOT_FOUND);
	}

	//Handle plugs and entities differently (for now)
	if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		dynamic_cast<ZstComponent*>(parent)->add_plug(dynamic_cast<ZstPlug*>(entity));
	}
	else {
		dynamic_cast<ZstContainer*>(parent)->add_child(entity);
	}

	std::cout << "ZST_STAGE: Registering new entity " << entity->URI().path() << std::endl;

	//Update rest of network
	publish_stage_update(msg_pool()->get()->init_entity_message(entity));

	return response->init_message(ZstMessage::Kind::OK);
}

ZstMessage * ZstStage::destroy_entity_handler(ZstMessage * msg)
{
	ZstMessage * response = msg_pool()->get();

	//Unpack entity to destroy from message
	ZstURI entity_path = ZstURI(msg->payload_at(0).data());

	//Find owner of entity
	ZstPerformer * owning_performer = get_client(entity_path);

	if (!owning_performer) {
		return response->init_message(ZstMessage::Kind::ERR_STAGE_PERFORMER_NOT_FOUND);
	}

	//Find existing entity
	ZstEntityBase * entity = owning_performer->find_child_by_URI(entity_path);
	if (!entity) {
		return response->init_message(ZstMessage::Kind::ERR_STAGE_ENTITY_NOT_FOUND);
	}

	//Handle plugs and entites differently (for now)
	if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		dynamic_cast<ZstComponent*>(entity->parent())->remove_plug(dynamic_cast<ZstPlug*>(entity));
	}
	else {
		dynamic_cast<ZstContainer*>(entity->parent())->remove_child(entity);
	}

	//Remove all cables linked to this entity
	std::vector<ZstCable*> cables = get_cables_in_entity(entity);
	for (auto c : cables) {
		destroy_cable(c);
	}

	//Update rest of network
	ZstMessage * stage_update_msg = msg_pool()->get()->init_message(msg->kind());
	stage_update_msg->append_str(entity_path.path());
	publish_stage_update(stage_update_msg);

	delete entity;
	return response->init_message(ZstMessage::Kind::OK);
}


ZstMessage * ZstStage::create_entity_template_handler(ZstMessage * msg)
{
	//TODO: Implement this
	throw new std::exception("Entity template creation not implemented");
	return msg_pool()->get()->init_message(ZstMessage::Kind::ERR_STAGE_ENTITY_NOT_FOUND);
}


ZstMessage * ZstStage::create_entity_from_template_handler(ZstMessage * msg)
{
	throw new std::exception("ZST_STAGE: Creating template entities not implemented yet");
	return msg_pool()->get()->init_message(ZstMessage::Kind::ERR_STAGE_ENTITY_NOT_FOUND);
}

ZstMessage * ZstStage::create_cable_handler(ZstMessage * msg)
{
	ZstMessage * response = msg_pool()->get();

	//Unpack cable from message
	ZstCable cable = msg->unpack_payload_streamable<ZstCable>(0);
	std::cout << "ZST_STAGE: Received connect cable request for " << cable.get_output_URI().path() << " and " << cable.get_output_URI().path() << endl;

	//Create cable 
	ZstCable * cable_ptr = create_cable(cable.get_output_URI(), cable.get_input_URI());

	if (!cable_ptr) {
		return response->init_message(ZstMessage::Kind::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST);
	}

	//Create connection request for the entity who owns the input plug
	ZstPerformer * input_performer = get_client(cable_ptr->get_input_URI());
	ZstPerformer * output_performer = get_client(cable_ptr->get_output_URI());

	std::cout << "ZST_STAGE: Sending cable connection request to " << input_performer->URI().path() << std::endl;

	ZstMessage * connection_msg = msg_pool()->get()->init_message(ZstMessage::Kind::SUBSCRIBE_TO_PERFORMER);
	connection_msg->append_str(output_performer->address());	//IP of output client
	connection_msg->append_str(cable.get_output_URI().path());	//Output plug path
	send_to_client(msg, input_performer);

	//Update rest of network
	publish_stage_update(msg_pool()->get()->init_streamable_message(msg->kind(), cable));

    return response->init_message(ZstMessage::Kind::OK);
}

ZstMessage * ZstStage::destroy_cable_handler(ZstMessage * msg)
{
	ZstMessage * response = msg_pool()->get();
	ZstCable cable = msg->unpack_payload_streamable<ZstCable>(0);
	cout << "ZST_STAGE: Received destroy cable connection request" << endl;

	ZstCable * cable_ptr = get_cable_by_URI(cable.get_input_URI(), cable.get_output_URI());

	if (!destroy_cable(cable_ptr)) {
		response->init_message(ZstMessage::Kind::ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST);
	}

	//Update rest of network
	publish_stage_update(msg_pool()->get()->init_streamable_message(msg->kind(), cable));

	return response->init_message(ZstMessage::Kind::OK);
}


//---------------------
// Outgoing event queue
//---------------------


void ZstStage::send_snapshot(ZstPerformer * client) {

	ZstMessage * snapshot = msg_pool()->get()->init_message(ZstMessage::Kind::GRAPH_SNAPSHOT);
	
	//Pack performer root entities
	for (auto performer : m_clients) {
		//Only pack performers that aren't the destination client
		if(performer.second->URI() != client->URI())
			snapshot->append_streamable(ZstMessage::Kind::CREATE_PERFORMER, *(performer.second));
	}

	//Pack cables
	for (auto cable : m_cables) {
		snapshot->append_streamable(ZstMessage::Kind::CREATE_CABLE, *cable);
	}

	send_to_client(snapshot, client);
}

void ZstStage::publish_stage_update(ZstMessage * msg)
{
	msg->send(m_graph_update_pub);
}

int ZstStage::stage_heartbeat_timer_func(zloop_t * loop, int timer_id, void * arg)
{
	ZstStage * stage = (ZstStage*)arg;
	std::unordered_map<ZstURI, ZstPerformer*> endpoints = stage->m_clients;
	std::vector<ZstPerformer*> removed_clients;
	for (auto performer_it : stage->m_clients) {
		ZstPerformer * performer = performer_it.second;
		if (performer->get_active_heartbeat()) {
			performer->clear_active_hearbeat();
		}
		else {
			cout << "ZST_STAGE: Endpoint " << performer->URI().path() << " missed a heartbeat. " << (MAX_MISSED_HEARTBEATS - performer->get_missed_heartbeats()) << " remaining" << endl;
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
