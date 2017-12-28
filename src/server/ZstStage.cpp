#include <sstream>
#include "ZstStage.h"
#include "ZstURI.h"
#include "ZstCable.h"
#include "entities/ZstPerformer.h"

//Core headers
#include "../core/ZstUtils.hpp"


using namespace std;

ZstStage::ZstStage()
{
}

ZstStage::~ZstStage()
{
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

    m_update_timer_id = attach_timer(stage_update_timer_func, 50, this);
	m_heartbeat_timer_id = attach_timer(stage_heartbeat_timer_func, HEARTBEAT_DURATION, this);

	start();
}

ZstStage* ZstStage::create_stage()
{
	ZstStage* stage = new ZstStage();
	stage->init();
	return stage;
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

	enqueue_stage_update(ZstMessage::Kind::CLIENT_LEAVE, zframe_from(performer->URI().path()));
	delete performer;
}


//------
//Cables
//------

ZstCable * ZstStage::create_cable_ptr(const ZstURI & a, const ZstURI & b)
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
	zmsg_t *msg = zmsg_recv(socket);
	

	//If we're dealing with a new client, we don't need to search for it
	if (message_type != ZstMessage::Kind::CLIENT_JOIN) {
		sender = stage->get_client(sender_s);
	}
	
	//Get message payload


	ZstMessage::Signal result;
	bool send_reply = true;
	bool announce_creation = true;

	//Message type handling
	switch (message_type) {
	case ZstMessage::Kind::SIGNAL: {
		result = stage->signal_handler(msg_payload, sender);
		break;
	}
	case ZstMessage::Kind::CLIENT_JOIN:
	{
		result = stage->create_client_handler(sender_s, msg_payload);
		sender = stage->get_client(sender_s);
		break;
	}
	case ZstMessage::Kind::CREATE_COMPONENT:
	{
		result = stage->create_entity_handler<ZstComponent>(msg_payload, sender);
		break;
	}
	case ZstMessage::Kind::CREATE_CONTAINER:
	{
		result = stage->create_entity_handler<ZstContainer>(msg_payload, sender);
		break;
	}
	case ZstMessage::Kind::CREATE_PLUG:
	{
		result = stage->create_entity_handler<ZstPlug>(msg_payload, sender);
		break;
	}
	case ZstMessage::Kind::CREATE_CABLE:
	{
		result = stage->create_cable_handler(msg_payload);
		break;
	}
	case ZstMessage::Kind::CREATE_ENTITY_FROM_TEMPLATE:
	{
		result = stage->create_entity_from_template_handler(msg_payload);
		break;
	}
	case ZstMessage::Kind::DESTROY_ENTITY:
	{
		result = stage->destroy_entity_handler(msg_payload);
		break;
	}
	case ZstMessage::Kind::DESTROY_PLUG:
	{
		result = stage->destroy_entity_handler(msg_payload);
		break;
	}
	case ZstMessage::Kind::DESTROY_CABLE:
	{
		result = stage->destroy_cable_handler(msg_payload);
		break;
	}
	default:
		cout << "ZST_STAGE: Didn't understand received message type of " << (char)message_type << endl;
		result = ZstMessage::Signal::ERR_STAGE_MSG_TYPE_UNKNOWN;
		break;
	}

	//Send ack
	if (send_reply)
		stage->reply_with_signal(socket, result, sender);

	if (announce_creation) {
		stage->enqueue_stage_update(message_type, msg_payload);
	}
	else {
		//Need to clean up the frame if we aren't forwarding it to the rest of the network
		zframe_destroy(&msg_payload);
	}

	//Cleanup
	zmsg_destroy(&msg);
	return 0;
}


//--------------------
//Client communication
//--------------------

void ZstStage::reply_with_signal(zsock_t * socket, ZstMessage::Signal status, ZstPerformer * destination)
{
	zmsg_t * signal_msg = ZstMessage::build_signal(status);

	if (destination != NULL) {
		zframe_t * empty = zframe_new_empty();
		zframe_t * identity = zframe_from(get_socket_ID(destination).c_str());

		zmsg_prepend(signal_msg, &empty);
		zmsg_prepend(signal_msg, &identity);
	}
	zmsg_print(signal_msg);
	zmsg_send(&signal_msg, socket);
}


void ZstStage::send_to_client(zmsg_t * msg, ZstPerformer * destination) {
	
	zframe_t * identity = zframe_from(get_socket_ID(destination).c_str());
	zframe_t * empty = zframe_new_empty();
	zmsg_prepend(msg, &empty);
	zmsg_prepend(msg, &identity);
	zmsg_send(&msg, m_performer_router);
}


// ----------------
// Message handlers
// ----------------

ZstMessage::Signal ZstStage::signal_handler(zframe_t * frame, ZstPerformer * sender)
{
	ZstMessage::Signal signal = ZstMessage::unpack_signal(frame);
	ZstMessage::Signal result = ZstMessage::Signal::OK;

	if (signal == ZstMessage::Signal::SYNC) {
		send_to_client(create_snapshot(), sender);
	}
	else if (signal == ZstMessage::Signal::LEAVING) {
		result = destroy_client_handler(sender);
	}
	else if (signal == ZstMessage::Signal::HEARTBEAT) {
		sender->set_heartbeat_active();
	}

	return result;
}

ZstMessage::Signal ZstStage::create_client_handler(std::string sender, zframe_t * frame)
{
	ZstPerformer * client = ZstMessage::unpack_entity<ZstPerformer>(frame);
	cout << "ZST_STAGE: Registering new client " << client->URI().path() << endl;
	
	//Only one client with this UUID at a time
	if (get_client(client->URI())) {
		cout << "ZST_STAGE: Client already exists " << client->URI().path() << endl;
		return ZstMessage::Signal::ERR_STAGE_PERFORMER_ALREADY_EXISTS;
	}

	m_clients[client->URI()] = client;
	m_client_socket_index[sender] = client;
    return ZstMessage::Signal::OK;
}

ZstMessage::Signal ZstStage::destroy_client_handler(ZstPerformer * performer)
{
	destroy_client(performer);
	return ZstMessage::Signal::OK;
}

template ZstMessage::Signal ZstStage::create_entity_handler<ZstPlug>(zframe_t * frame, ZstPerformer * performer);
template ZstMessage::Signal ZstStage::create_entity_handler<ZstComponent>(zframe_t * frame, ZstPerformer * performer);
template ZstMessage::Signal ZstStage::create_entity_handler<ZstContainer>(zframe_t * frame, ZstPerformer * performer);
template <typename T>
ZstMessage::Signal ZstStage::create_entity_handler(zframe_t * frame, ZstPerformer * performer) {
	if (!performer)
		return ZstMessage::Signal::ERR_STAGE_PERFORMER_NOT_FOUND;

	T* entity = ZstMessage::unpack_entity<T>(frame);

	if (performer->find_child_by_URI(entity->URI())) {
		//Entity already exists
		std::cout << "ZST_STAGE: Entity already exists! " << entity->URI().path() << std::endl;
		delete entity;
		return ZstMessage::Signal::ERR_STAGE_PLUG_ALREADY_EXISTS;
	}
	
	//Find the parent for this entity
	ZstURI parent_path = entity->URI().range(0, entity->URI().size() - 1);
	ZstEntityBase * parent = dynamic_cast<T*>(performer->find_child_by_URI(parent_path));

	if (parent == NULL) {
        std::cout << "ZST_STAGE: Couldn't register entity. No parent found at " << parent_path.path() << std::endl;
        return ZstMessage::Signal::ERR_STAGE_ENTITY_NOT_FOUND;
	}

	std::cout << "ZST_STAGE: Registering new entity " << entity->URI().path() << std::endl;
	if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		dynamic_cast<ZstComponent*>(parent)->add_plug(dynamic_cast<ZstPlug*>(entity));
	}
	else {
		dynamic_cast<ZstContainer*>(parent)->add_child(entity);
	}
    return ZstMessage::Signal::OK;
}

ZstMessage::Signal ZstStage::destroy_entity_handler(zframe_t * frame)
{
	ZstURI entity_path = ZstURI((char*)zframe_data(frame));
	ZstPerformer * owning_performer = get_client(entity_path);
	if (!owning_performer) {
		return ZstMessage::Signal::ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	ZstEntityBase * entity = owning_performer->find_child_by_URI(entity_path);
	if (!entity) {
		return ZstMessage::Signal::ERR_STAGE_ENTITY_NOT_FOUND;
	}

	if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		dynamic_cast<ZstComponent*>(entity->parent())->remove_plug(dynamic_cast<ZstPlug*>(entity));
	}
	else {
		dynamic_cast<ZstContainer*>(entity->parent())->remove_child(entity);
	}

	std::vector<ZstCable*> cables = get_cables_in_entity(entity);
	for (auto c : cables) {
		destroy_cable(c);
	}

	delete entity;
	return ZstMessage::Signal::OK;
}


ZstMessage::Signal ZstStage::create_entity_template_handler(zframe_t * frame)
{
    //TODO:Implement entity template handler on stage
	throw new std::exception("Entity template creation not implemented");
    return ZstMessage::Signal::ERR_STAGE_ENTITY_NOT_FOUND;
}


ZstMessage::Signal ZstStage::create_entity_from_template_handler(zframe_t * frame)
{
	throw new std::exception("ZST_STAGE: Creating template entities not implemented yet");
	return ZstMessage::Signal::OK;
}

ZstMessage::Signal ZstStage::create_cable_handler(zframe_t * frame)
{
	ZstCable cable = ZstMessage::unpack_streamable<ZstCable>(frame);
	std::cout << "ZST_STAGE: Received connect cable request for " << cable.get_output_URI().path() << " and " << cable.get_output_URI().path() << endl;

	ZstCable * cable_ptr = create_cable_ptr(cable.get_output_URI(), cable.get_input_URI());

	if (!cable_ptr) {
		return ZstMessage::Signal::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
	}

	//Create connection request for the entity who owns the input plug
	ZstPerformer * input_performer = get_client(cable_ptr->get_input_URI());
	ZstPerformer * output_performer = get_client(cable_ptr->get_output_URI());

	std::cout << "ZST_STAGE: Sending cable connection request to " << input_performer->URI().path() << std::endl;

	//Add kind frame
	zframe_t * kind_frame = ZstMessage::append_kind_frame(ZstMessage::Kind::SUBSCRIBE_TO_PERFORMER);
	zmsg_t * msg = zmsg_new();
	zmsg_append(msg, &kind_frame);

	//Pack target performer address
	zmsg_addstr(msg, input_performer->URI().path());
	
	//...and send it
	send_to_client(msg, input_performer);
    return ZstMessage::Signal::OK;
}

ZstMessage::Signal ZstStage::destroy_cable_handler(zframe_t * frame)
{
	ZstCable cable = ZstMessage::unpack_streamable<ZstCable>(frame);
	cout << "ZST_STAGE: Received destroy cable connection request" << endl;

	ZstCable * cable_ptr = get_cable_by_URI(cable.get_input_URI(), cable.get_output_URI());

	if (!destroy_cable(cable_ptr)) {
		return ZstMessage::Signal::ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST;
	}
   
    return ZstMessage::Signal::OK;
}


//---------------------
// Outgoing event queue
//---------------------

void ZstStage::enqueue_stage_update(ZstMessage::Kind k, zframe_t * frame)
{
    MessagePair p = {k, frame};
	m_stage_updates.push(p);
}

zmsg_t * ZstStage::create_snapshot() {

	zmsg_t * batch_msg = zmsg_new();
	zframe_t * batch_kind_frame = ZstMessage::append_kind_frame(ZstMessage::Kind::GRAPH_SNAPSHOT);
	zframe_t * item_frame = NULL;
	zmsg_append(batch_msg, &batch_kind_frame);

	std::stringstream buffer;
	for (auto performer : m_clients) {
		performer.second->write(buffer);
		item_frame = ZstMessage::append_kind_frame(ZstMessage::Kind::CREATE_PERFORMER);
		zmsg_append(batch_msg, &item_frame);
		zmsg_addmem(batch_msg, buffer.str().c_str(), buffer.str().size());
		buffer.str("");
	}

	for (auto cable : m_cables) {
		cable->write(buffer);
		item_frame = ZstMessage::append_kind_frame(ZstMessage::Kind::CREATE_CABLE);
		zmsg_append(batch_msg, &item_frame);
		zmsg_addmem(batch_msg, buffer.str().c_str(), buffer.str().size());
		buffer.str("");
	}
	
    return batch_msg;
}

int ZstStage::stage_update_timer_func(zloop_t * loop, int timer_id, void * arg)
{
	ZstStage *stage = (ZstStage*)arg;

	if (stage->m_stage_updates.size()) {
		zmsg_t * batch_msg = zmsg_new();
		zframe_t * batch_kind_frame = ZstMessage::append_kind_frame(ZstMessage::Kind::GRAPH_SNAPSHOT);
		zframe_t * item_frame = NULL;
		zmsg_append(batch_msg, &batch_kind_frame);

		std::stringstream buffer;
		while (stage->m_stage_updates.size()) {
            MessagePair p = stage->m_stage_updates.pop();
			item_frame = ZstMessage::append_kind_frame(p.kind);
			zmsg_append(batch_msg, &item_frame);
			zmsg_append(batch_msg, &(p.packed));
			buffer.str("");
		}

		zmsg_send(&batch_msg, stage->m_graph_update_pub);
	}
	return 0;
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
