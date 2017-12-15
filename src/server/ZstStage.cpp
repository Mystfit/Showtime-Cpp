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

ZstPerformer * ZstStage::get_client_by_URI(const ZstURI & path)
{
	if (m_clients.find(path) != m_clients.end()) {
		return m_clients[path];
	}
	return NULL;
}

void ZstStage::destroy_client(ZstPerformer * performer)
{
	//Nothing to do
	if (performer == NULL) {
		return;
	}

	std::vector<ZstCable*> cables = get_cables_in_entity(performer);
	for (auto c : cables) {
		destroy_cable(c);
	}

	//Remove client and call all destructors in its hierarchy
	std::unordered_map<ZstURI, ZstPerformer*>::iterator client_it = m_clients.find(performer->URI());
	if (client_it != m_clients.end()) {
		m_clients.erase(client_it);
	}

	std::stringstream entity_buffer;
	ZstURI c = performer->URI();
	c.write(entity_buffer);
	zframe_t * uri_frame = zframe_from(entity_buffer.str().c_str());

	enqueue_stage_update(ZstMessages::Kind::CLIENT_LEAVE, uri_frame);
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
	ZstPlug * input_plug = get_client_by_URI(cable_ptr->get_input())->get_plug_by_URI(cable_ptr->get_input());
	ZstPlug * output_plug = get_client_by_URI(cable_ptr->get_output())->get_plug_by_URI(cable_ptr->get_output());

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
	std::vector<ZstCable*> cables = get_cables_by_URI(path);
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
		cout << "ZST_STAGE: Destroying cable " << cable->get_output().path() << " " << cable->get_input().path() << endl;
		for (vector<ZstCable*>::iterator cable_iter = m_cables.begin(); cable_iter != m_cables.end(); ++cable_iter) {
			if ((*cable_iter) == cable) {
				m_cables.erase(cable_iter);
				break;
			}
		}

		ZstURI out_uri = cable->get_output();
		ZstURI in_uri = cable->get_input();

		delete cable;
		cable = 0;
		return 1;
	}
	return 0;
}

vector<ZstCable*> ZstStage::get_cables_by_URI(const ZstURI & uri) {

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
		if (cable->get_input().contains(entity->URI()) || cable->get_output().contains(entity->URI())) {
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

	//Receive waiting message
	zmsg_t *msg = zmsg_recv(socket);

	//Get client
    char * sender_s = zmsg_popstr(msg);
	ZstPerformer * sender = stage->get_client_by_URI(sender_s);
    zstr_free(&sender_s);

	if (sender == NULL) {
		//TODO: Can't return error to caller if the client doesn't exist! Will need to implement timeouts
		//stage->reply_with_signal(socket, ZstMessages::ENDPOINT_NOT_FOUND);
		return 0;
	}

	//Pop off empty frame
	zframe_t * empty = zmsg_pop(msg);
    zframe_destroy(&empty);

	//Get message type and payload
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
	zframe_t * msg_payload = zmsg_pop(msg);

	ZstMessages::Signal result;
	bool send_reply = true;
	bool announce_creation = true;

	//Message type handling
	switch (message_type) {
	case ZstMessages::Kind::SIGNAL: {
		result = stage->signal_handler(msg_payload, sender);
		announce_creation = false;
		break;
	}
	case ZstMessages::Kind::CLIENT_JOIN:
	{
		result = stage->create_client_handler(msg_payload);
		break;
	}
	case ZstMessages::Kind::CREATE_COMPONENT:
	{
		result = stage->create_entity_handler<ZstComponent>(msg_payload, sender);
		break;
	}
	case ZstMessages::Kind::CREATE_CONTAINER:
	{
		result = stage->create_entity_handler<ZstContainer>(msg_payload, sender);
		break;
	}
	case ZstMessages::Kind::CREATE_PLUG:
	{
		result = stage->create_entity_handler<ZstPlug>(msg_payload, sender);
		break;
	}
	case ZstMessages::Kind::CREATE_CABLE:
	{
		result = stage->create_cable_handler(msg_payload);
		break;
	}
	case ZstMessages::Kind::CREATE_ENTITY_FROM_TEMPLATE:
	{
		result = stage->create_entity_from_template_handler(msg_payload);
		break;
	}
	case ZstMessages::Kind::DESTROY_ENTITY:
	{
		result = stage->destroy_entity_handler(msg_payload);
		break;
	}
	case ZstMessages::Kind::DESTROY_PLUG:
	{
		result = stage->destroy_entity_handler(msg_payload);
		break;
	}
	case ZstMessages::Kind::DESTROY_CABLE:
	{
		result = stage->destroy_cable_handler(msg_payload);
		break;
	}
	default:
		cout << "ZST_STAGE: Didn't understand received message type of " << (char)message_type << endl;
		result = ZstMessages::Signal::ERR_STAGE_MSG_TYPE_UNKNOWN;
		break;
	}

	//Send ack
	if (send_reply)
		stage->reply_with_signal(socket, result);

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

int ZstStage::s_handle_performer_requests(zloop_t * loop, zsock_t * socket, void * arg)
{
	ZstStage *stage = (ZstStage*)arg;

	//Receive waiting message
	zmsg_t *msg = zmsg_recv(socket);

	//Get message type
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
    
    

	zmsg_destroy(&msg);
	return 0;
}


//--------------------
//Client communication
//--------------------

void ZstStage::reply_with_signal(zsock_t * socket, ZstMessages::Signal status, ZstPerformer * destination)
{
	zmsg_t * signal_msg = ZstMessages::build_signal(status);

	if (destination != NULL) {
		zframe_t * empty = zframe_new_empty();
		zframe_t * identity = zframe_from(destination->uuid());

		zmsg_prepend(signal_msg, &empty);
		zmsg_prepend(signal_msg, &identity);
	}

	zmsg_send(&signal_msg, socket);
}


void ZstStage::send_to_client(zmsg_t * msg, ZstPerformer * destination) {
	zframe_t * identity = zframe_from(destination->uuid());
	zframe_t * empty = zframe_new_empty();
	zmsg_prepend(msg, &empty);
	zmsg_prepend(msg, &identity);
	zmsg_send(&msg, m_performer_router);
}


// ----------------
// Message handlers
// ----------------

ZstMessages::Signal ZstStage::signal_handler(zframe_t * frame, ZstPerformer * sender)
{
	ZstMessages::Signal signal = ZstMessages::unpack_signal(frame);
	ZstMessages::Signal result = ZstMessages::Signal::OK;

	if (signal == ZstMessages::Signal::SYNC) {
		send_to_client(create_snapshot(), sender);
	}
	else if (signal == ZstMessages::Signal::LEAVING) {
		result = destroy_client_handler(sender);
	}
	else if (signal == ZstMessages::Signal::HEARTBEAT) {
		sender->set_heartbeat_active();
	}

	return result;
}

ZstMessages::Signal ZstStage::create_client_handler(zframe_t * frame)
{
	ZstPerformer * client = ZstMessages::unpack_entity<ZstPerformer>(frame);
	cout << "ZST_STAGE: Registering new client " << client->URI().path() << endl;

	//Only one client with this UUID at a time
	if (get_client_by_URI(client->uuid())) {
		cout << "ZST_STAGE: Client already exists " << client->URI().path() << endl;
		return ZstMessages::Signal::ERR_STAGE_PERFORMER_ALREADY_EXISTS;
	}

	m_clients[client->URI()] = client;
    return ZstMessages::Signal::OK;
}

ZstMessages::Signal ZstStage::destroy_client_handler(ZstPerformer * performer)
{
	destroy_client(performer);
	return ZstMessages::Signal::OK;
}

template ZstMessages::Signal ZstStage::create_entity_handler<ZstPlug>(zframe_t * frame, ZstPerformer * performer);
template ZstMessages::Signal ZstStage::create_entity_handler<ZstComponent>(zframe_t * frame, ZstPerformer * performer);
template ZstMessages::Signal ZstStage::create_entity_handler<ZstContainer>(zframe_t * frame, ZstPerformer * performer);
template <typename T>
ZstMessages::Signal ZstStage::create_entity_handler(zframe_t * frame, ZstPerformer * performer) {
	T* entity = ZstMessages::unpack_entity<T>(frame);

	if (performer->find_child_by_URI(entity->URI())) {
		//Entity already exists
		std::cout << "ZST_STAGE: Entity already exists! " << entity->URI().path() << std::endl;
		delete entity;
		return ZstMessages::Signal::ERR_STAGE_PLUG_ALREADY_EXISTS;
	}
	
	//Find the parent for this entity
	ZstURI parent_path = entity->URI().range(0, entity->URI().size() - 1);
	ZstEntityBase * parent = dynamic_cast<T*>(performer->find_child_by_URI(parent_path));

	if (parent == NULL) {
        std::cout << "ZST_STAGE: Couldn't register entity. No parent found at " << parent_path.path() << std::endl;
        return ZstMessages::Signal::ERR_STAGE_ENTITY_NOT_FOUND;
	}

	std::cout << "ZST_STAGE: Registering new entity " << entity->URI().path() << std::endl;
	if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		dynamic_cast<ZstComponent*>(parent)->add_plug(dynamic_cast<ZstPlug*>(entity));
	}
	else {
		dynamic_cast<ZstContainer*>(parent)->add_child(entity);
	}
    return ZstMessages::Signal::OK;
}

ZstMessages::Signal ZstStage::destroy_entity_handler(zframe_t * frame)
{
	ZstURI entity_path = ZstMessages::unpack_streamable<ZstURI>(frame);
	ZstPerformer * owning_performer = get_client_by_URI(entity_path);
	if (!owning_performer) {
		return ZstMessages::Signal::ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	ZstEntityBase * entity = owning_performer->find_child_by_URI(entity_path);
	if (!entity) {
		return ZstMessages::Signal::ERR_STAGE_ENTITY_NOT_FOUND;
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
	return ZstMessages::Signal::OK;
}


ZstMessages::Signal ZstStage::create_entity_template_handler(zframe_t * frame)
{
    //TODO:Implement entity template handler on stage
	throw new std::exception("Entity template creation not implemented");
    return ZstMessages::Signal::ERR_STAGE_ENTITY_NOT_FOUND;
}


ZstMessages::Signal ZstStage::create_entity_from_template_handler(zframe_t * frame)
{
	throw new std::exception("ZST_STAGE: Creating template entities not implemented yet");
	return ZstMessages::Signal::OK;
}

ZstMessages::Signal ZstStage::create_cable_handler(zframe_t * frame)
{
	ZstCable cable = ZstMessages::unpack_streamable<ZstCable>(frame);
	std::cout << "ZST_STAGE: Received connect cable request for " << cable.get_output().path() << " and " << cable.get_output().path() << endl;

	ZstCable * cable_ptr = create_cable(cable.get_output(), cable.get_input());

	if (!cable_ptr) {

		return ZstMessages::Signal::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
	}

	//Create connection request for the entity who owns the input plug
	ZstPerformer * input_performer = get_client_by_URI(cable_ptr->get_input());
	ZstPerformer * output_performer = get_client_by_URI(cable_ptr->get_output());

	std::cout << "ZST_STAGE: Sending cable connection request to " << input_performer->URI().path() << std::endl;

	//Add kind frame
	zframe_t * kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::SUBSCRIBE_TO_PERFORMER);
	zmsg_t * msg = zmsg_new();
	zmsg_append(msg, &kind_frame);

	//Pack target performer address
	std::stringstream address_buffer;
	ZstURI(input_performer->URI()).write(address_buffer);
	zmsg_addstr(msg, address_buffer.str().c_str());
	
	//And send it
	send_to_client(msg, input_performer);
    return ZstMessages::Signal::OK;
}

ZstMessages::Signal ZstStage::destroy_cable_handler(zframe_t * frame)
{
	ZstCable cable = ZstMessages::unpack_streamable<ZstCable>(frame);
	cout << "ZST_STAGE: Received destroy cable connection request" << endl;

	ZstCable * cable_ptr = get_cable_by_URI(cable.get_input(), cable.get_output());

	if (!destroy_cable(cable_ptr)) {
		return ZstMessages::Signal::ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST;
	}
   
    return ZstMessages::Signal::OK;
}


//---------------------
// Outgoing event queue
//---------------------

void ZstStage::enqueue_stage_update(ZstMessages::Kind k, zframe_t * frame)
{
    ZstMessages::MessagePair p = {k, frame};
	m_stage_updates.push(p);
}

zmsg_t * ZstStage::create_snapshot() {

	zmsg_t * batch_msg = zmsg_new();
	zframe_t * batch_kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::GRAPH_SNAPSHOT);
	zframe_t * item_frame = NULL;
	zmsg_append(batch_msg, &batch_kind_frame);

	std::stringstream buffer;
	for (auto performer : m_clients) {
		performer.second->write(buffer);
		item_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::CREATE_PERFORMER);
		zmsg_append(batch_msg, &item_frame);
		zmsg_addstr(batch_msg, buffer.str().c_str());
		buffer.str("");
	}

	for (auto cable : m_cables) {
		cable->write(buffer);
		item_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::CREATE_CABLE);
		zmsg_append(batch_msg, &item_frame);
		zmsg_addstr(batch_msg, buffer.str().c_str());
		buffer.str("");
	}
	
    return batch_msg;
}

int ZstStage::stage_update_timer_func(zloop_t * loop, int timer_id, void * arg)
{
	ZstStage *stage = (ZstStage*)arg;

	if (stage->m_stage_updates.size()) {
		zmsg_t * batch_msg = zmsg_new();
		zframe_t * batch_kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::GRAPH_SNAPSHOT);
		zframe_t * item_frame = NULL;
		zmsg_append(batch_msg, &batch_kind_frame);

		std::stringstream buffer;
		while (stage->m_stage_updates.size()) {
            ZstMessages::MessagePair p = stage->m_stage_updates.pop();
			item_frame = ZstMessages::build_message_kind_frame(p.kind);
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
			stage->destroy_client(performer);
		}
	}
	return 0;
}
