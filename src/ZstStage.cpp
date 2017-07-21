#include "ZstStage.h"
#include "ZstURI.h"
#include "ZstUtils.hpp"
#include "ZstEventWire.h"

using namespace std;

ZstStage::ZstStage()
{
	m_stage_identity = ZstURI("stage", "", "");
}

ZstStage::~ZstStage()
{
	detach_timer(m_heartbeat_timer_id);
	detach_timer(m_heartbeat_timer_id);
	ZstActor::~ZstActor();
	//Close stage pipes
	zsock_destroy(&m_performer_requests);
	zsock_destroy(&m_performer_router);
	zsock_destroy(&m_graph_update_pub);
}

void ZstStage::init()
{
	ZstActor::init();
    
    Str255 stage_rep_addr;
    sprintf(stage_rep_addr, "@tcp://*:%d", STAGE_REP_PORT);
    
	m_performer_requests = zsock_new_rep(stage_rep_addr);
	zsock_set_linger(m_performer_requests, 0);
	attach_pipe_listener(m_performer_requests, s_handle_performer_requests, this);

	m_performer_router = zsock_new(ZMQ_ROUTER);
	zsock_set_linger(m_performer_router, 0);
	attach_pipe_listener(m_performer_router, s_handle_router, this);
    
    Str255 stage_router_addr;
    sprintf(stage_router_addr, "tcp://*:%d", STAGE_ROUTER_PORT);
	zsock_bind(m_performer_router, "%s", stage_router_addr);

    Str255 stage_pub_addr;
    sprintf(stage_pub_addr, "@tcp://*:%d", STAGE_PUB_PORT);
	m_graph_update_pub = zsock_new_pub(stage_pub_addr);
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


std::vector<ZstPlugRef*> ZstStage::get_all_plug_refs(){
    return get_all_plug_refs(NULL);
}

std::vector<ZstPlugRef*> ZstStage::get_all_plug_refs(ZstEndpointRef * endpoint)
{
    vector<ZstPlugRef*> plugs;

    if(endpoint != NULL){
        vector<ZstPerformerRef*> performers = endpoint->get_performer_refs();
        for (auto performer : performers){
            plugs = performer->get_plug_refs();
        }

    } else {
        vector<ZstPerformerRef*> performers = get_all_performer_refs();
        if (performers.size() > 0) {
            for (auto performer : performers){
                plugs.insert(plugs.end(), performer->get_plug_refs().begin(), performer->get_plug_refs().end());
            }
        }
    }
	
	return plugs;
}

std::vector<ZstPerformerRef*> ZstStage::get_all_performer_refs()
{
	vector<ZstPerformerRef*> all_performers;
    for (auto endpnt_iter : m_endpoint_refs) {
		vector<ZstPerformerRef*> endpoint_performers = endpnt_iter.second->get_performer_refs();
		all_performers.insert(all_performers.end(), endpoint_performers.begin(), endpoint_performers.end());
	}

	return all_performers;
}

ZstPerformerRef * ZstStage::get_performer_ref_by_name(const char * performer_name) {

	ZstPerformerRef * performer = NULL;
    for (auto endpnt_iter : m_endpoint_refs) {
		performer = endpnt_iter.second->get_performer_by_name(string(performer_name));
		if (performer != NULL)
			break;
	}
	return performer;
}

ZstEndpointRef * ZstStage::get_performer_endpoint(ZstPerformerRef * performer)
{
	ZstEndpointRef * endpoint = NULL;
    for (auto endpnt_iter : m_endpoint_refs) {
		if (endpnt_iter.second->get_performer_by_name(performer->get_URI().performer()) != NULL) {
			endpoint = endpnt_iter.second;
			break;
		}
	}
	return endpoint;
}

ZstEndpointRef * ZstStage::create_endpoint(std::string starting_uuid, std::string endpoint)
{
	string new_uuid = "ENDPNT_" + (string)zuuid_str(zuuid_new());
	ZstEndpointRef * endpointRef = new ZstEndpointRef(starting_uuid, new_uuid, endpoint);
	m_endpoint_refs[new_uuid] = endpointRef;
	return endpointRef;
}

ZstEndpointRef * ZstStage::get_endpoint_ref_by_UUID(std::string uuid)
{
	if (m_endpoint_refs.find(uuid) != m_endpoint_refs.end()) {
		return m_endpoint_refs[uuid];
	}
	return NULL;
}

void ZstStage::destroy_endpoint(ZstEndpointRef * endpoint)
{
    //Nothing to do
	if (endpoint == NULL) {
		return;
	}
    
    //Remove all cables
    vector<ZstPlugRef*> plugs = get_all_plug_refs(endpoint);
    
    for (auto plug : plugs) {
        if(plug != NULL){
            destroy_cable(plug->get_URI());
        }
    }
    
    //Remove endpoint and call all destructors in its hierarchy
	for (std::map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter)
	{
		if ((endpnt_iter->second) == endpoint)
		{
			cout << "ZST_STAGE: Destroying endpoint " << endpnt_iter->second->client_assigned_uuid << endl;
			m_endpoint_refs.erase(endpnt_iter);
			break;
        }
	}

	delete endpoint;
}



// ------------------------
//--------------------------


int ZstStage::s_handle_router(zloop_t * loop, zsock_t * socket, void * arg)
{
	ZstStage *stage = (ZstStage*)arg;

	//Receive waiting message
	zmsg_t *msg = zmsg_recv(socket);

	//Get endpoint	
	ZstEndpointRef * sender = stage->get_endpoint_ref_by_UUID(zmsg_popstr(msg));
	if (sender == NULL) {
		//TODO: Can't return error to caller if the endpoint doesn't exist! Will need to implement timeouts
		//stage->reply_with_signal(socket, Signal::ENDPOINT_NOT_FOUND);
		return 0;
	}

	//Pop off empty frame
	zmsg_pop(msg);

	//Get message type
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);


	if (message_type == ZstMessages::Kind::SIGNAL) {
		ZstMessages::Signal s = ZstMessages::unpack_signal(msg);

		if (s == ZstMessages::Signal::SYNC) {
			//Endpoint is requesting to catch up with the graph. Send a full snapshot
			ZstMessages::StageUpdates su;
			su.updates = stage->create_snapshot();
			zmsg_t *updatemsg = ZstMessages::build_message<ZstMessages::StageUpdates>(ZstMessages::Kind::STAGE_UPDATE, su);
			stage->send_to_endpoint(updatemsg, sender);
		}
		else if (s == ZstMessages::Signal::LEAVING) {
			stage->destroy_endpoint(sender);
		}
		else if (s == ZstMessages::Signal::HEARTBEAT) {
			sender->set_heartbeat_active();
		}
	}

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

	switch (message_type) {
	case ZstMessages::Kind::STAGE_CREATE_ENDPOINT:
		stage->create_endpoint_handler(socket, msg);
		break;		
	case ZstMessages::Kind::STAGE_CREATE_PERFORMER:
		stage->create_performer_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_CREATE_PLUG:
		stage->create_plug_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_DESTROY_PLUG:
		stage->destroy_plug_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_CREATE_CABLE:
		stage->create_cable_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_DESTROY_CABLE:
		stage->destroy_cable_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_CREATE_ENTITY:
		stage->create_entity_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_DESTROY_ENTITY:
		stage->destroy_entity_handler(socket, msg);
	case ZstMessages::Kind::STAGE_REGISTER_ENTITY_TYPE:
		stage->register_entity_type_handler(socket, msg);
		break;
	case ZstMessages::Kind::ENDPOINT_HEARTBEAT:
		stage->endpoint_heartbeat_handler(socket, msg);
		break;
	default:
		cout << "ZST_STAGE: Didn't understand received message type of " << message_type << endl;
		break;
	}

	zmsg_destroy(&msg);
	return 0;
}


//--------------------------
//--------------------------

void ZstStage::reply_with_signal(zsock_t * socket, ZstMessages::Signal status, ZstEndpointRef * destination)
{
	ZstMessages::SignalAck signal_ack;
	signal_ack.sig = status;

	zmsg_t * signal_msg = ZstMessages::build_signal(status);

	if (destination != NULL) {
		zframe_t * empty = zframe_new_empty();
		zframe_t * identity = zframe_from(destination->client_assigned_uuid.c_str());

		zmsg_prepend(signal_msg, &empty);
		zmsg_prepend(signal_msg, &identity);
	}

	zmsg_send(&signal_msg, socket);
}


void ZstStage::send_to_endpoint(zmsg_t * msg, ZstEndpointRef * destination) {
	zframe_t * empty = zframe_new_empty();
	zframe_t * identity = zframe_from(destination->client_assigned_uuid.c_str());
	zmsg_prepend(msg, &empty);
	zmsg_prepend(msg, &identity);
	zmsg_send(&msg, m_performer_router);
}


// --------------
// Reply handlers

void ZstStage::create_endpoint_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::CreateEndpoint endpoint_args = ZstMessages::unpack_message_struct<ZstMessages::CreateEndpoint>(msg);
	cout << "ZST_STAGE: Registering new endpoint UUID " << endpoint_args.uuid << endl;

	ZstEndpointRef * endpointRef = create_endpoint(endpoint_args.uuid, endpoint_args.address);

	if (endpointRef == NULL) {
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_ENDPOINT_ALREADY_EXISTS);
		return;
	}

	ZstMessages::CreateEndpointAck ack_params;
	ack_params.assigned_uuid = endpointRef->client_assigned_uuid;

	zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::CreateEndpointAck>(ZstMessages::Kind::STAGE_CREATE_ENDPOINT_ACK, ack_params);
	zmsg_send(&ackmsg, socket);
}

void ZstStage::endpoint_heartbeat_handler(zsock_t * socket, zmsg_t * msg) {
	ZstMessages::Heartbeat heartbeat_args = ZstMessages::unpack_message_struct<ZstMessages::Heartbeat>(msg);
	cout << "ZST_STAGE: Received heartbeat from " << heartbeat_args.from << ". Timestamp: " << heartbeat_args.timestamp << endl;

	reply_with_signal(socket, ZstMessages::Signal::OK);
}


void ZstStage::create_performer_handler(zsock_t * socket, zmsg_t * msg) {
	ZstMessages::CreatePerformer performer_args = ZstMessages::unpack_message_struct<ZstMessages::CreatePerformer>(msg);
	cout << "ZST_STAGE: Registering new performer " << performer_args.name << endl;

	ZstEndpointRef * endpoint = get_endpoint_ref_by_UUID(performer_args.endpoint_uuid);
	if (endpoint == NULL) {
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_ENDPOINT_NOT_FOUND);
		return;
	}

	ZstPerformerRef * performerRef = endpoint->create_performer(performer_args.name);
	if (performerRef == NULL) {
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_PERFORMER_ALREADY_EXISTS);
		return;
	}

	reply_with_signal(socket, ZstMessages::Signal::OK);
	enqueue_stage_update(ZstEvent(performerRef->get_URI(), ZstEvent::EventType::PERFORMER_CREATED));
}


void ZstStage::create_plug_handler(zsock_t *socket, zmsg_t *msg) {
	ZstMessages::CreatePlug plug_args = ZstMessages::unpack_message_struct<ZstMessages::CreatePlug>(msg);

	ZstPerformerRef * performer = get_performer_ref_by_name(plug_args.address.performer_char());

	if (performer == NULL) {
		cout << "ZST_STAGE: Couldn't register plug. No performer registered to stage with name";
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_PERFORMER_NOT_FOUND);
		return;
	}

	cout << "ZST_STAGE: Registering new plug " << plug_args.address.to_char() << endl;

	ZstPlugRef * plug = performer->create_plug(plug_args.address, plug_args.dir);

	if (plug == NULL) {
        cout << "ZST_STAGE: Plug already exists! " << plug_args.address.to_char() << endl;
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_PLUG_ALREADY_EXISTS);
		return;
	}

	reply_with_signal(socket, ZstMessages::Signal::OK);
	enqueue_stage_update(ZstEvent(plug->get_URI(), ZstEvent::EventType::PLUG_CREATED));
}

void ZstStage::register_entity_type_handler(zsock_t * socket, zmsg_t * msg)
{
	throw exception("Register entity handler not implemented");
}

void ZstStage::create_entity_handler(zsock_t * socket, zmsg_t * msg)
{
	throw exception("Create entity handler not implemented");
}

void ZstStage::destroy_entity_handler(zsock_t * socket, zmsg_t * msg)
{
	throw exception("Destroy entity handler not implemented");
}

void ZstStage::destroy_plug_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::DestroyPlug plug_destroy_args = ZstMessages::unpack_message_struct<ZstMessages::DestroyPlug>(msg);
	cout << "ZST_STAGE: Received destroy plug request" << endl;

	ZstPerformerRef *performer = get_performer_ref_by_name(plug_destroy_args.address.performer_char());

	ZstPlugRef* plugRef = performer->get_plug_by_URI(plug_destroy_args.address);
	if (plugRef != NULL) {
        destroy_cable(plugRef->get_URI());
	}

	performer->destroy_plug(performer->get_plug_by_URI(plug_destroy_args.address));

	reply_with_signal(socket, ZstMessages::Signal::OK);
	enqueue_stage_update(ZstEvent(plug_destroy_args.address, ZstEvent::EventType::PLUG_DESTROYED));
}


// ----------------
// Router endpoints
void ZstStage::create_cable_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::CreateCable plug_args = ZstMessages::unpack_message_struct<ZstMessages::CreateCable>(msg);
	cout << "ZST_STAGE: Received connect cable request for " << plug_args.first.to_char() << " and " << plug_args.second.to_char() << endl;
	int connect_status = 0;
    
    ZstPerformerRef * performer_A = get_performer_ref_by_name(plug_args.first.performer_char());
    ZstPerformerRef * performer_B = get_performer_ref_by_name(plug_args.second.performer_char());
    
    if (performer_A && performer_B) {
        ZstPlugRef * plug_A = performer_A->get_plug_by_URI(plug_args.first);
        ZstPlugRef * plug_B = performer_B->get_plug_by_URI(plug_args.second);
        
        if (performer_A && performer_B){
            if (plug_A->get_direction() == PlugDirection::OUT_JACK && plug_B->get_direction() == PlugDirection::IN_JACK) {
                connect_status = connect_cable(plug_A, plug_B);
            }
            else if (plug_A->get_direction() == PlugDirection::IN_JACK && plug_B->get_direction() == PlugDirection::OUT_JACK) {
                connect_status = connect_cable(plug_B, plug_A);
            }
        }
    } else {
        cout << "ZST_STAGE: Missing performer!" << endl;
    }
   
	if (!connect_status) {
		cout << "ZST_STAGE: Bad cable connect request" << endl;
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST);
		return;
	}

	reply_with_signal(socket, ZstMessages::Signal::OK);
	enqueue_stage_update(ZstEvent(plug_args.first, plug_args.second, ZstEvent::EventType::CABLE_CREATED));
}

void ZstStage::destroy_cable_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::CreateCable connection_destroy_args = ZstMessages::unpack_message_struct<ZstMessages::CreateCable>(msg);
	cout << "ZST_STAGE: Received destroy cable connection request" << endl;

	if (!destroy_cable(connection_destroy_args.first, connection_destroy_args.second)) {
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST);
		return;
	}

	reply_with_signal(socket, ZstMessages::Signal::OK);
	enqueue_stage_update(ZstEvent(connection_destroy_args.first, connection_destroy_args.second, ZstEvent::EventType::CABLE_DESTROYED));
}

int ZstStage::connect_cable(ZstPlugRef * output_plug, ZstPlugRef * input_plug)
{
	if (get_cable_by_URI(output_plug->get_URI(), input_plug->get_URI()) != NULL) {
		cout << "ZST_STAGE: Cable already exists for " << output_plug->get_URI().to_char() << " and " << input_plug->get_URI().to_char() << endl;
		return 0;
	}
	m_cables.push_back(new ZstCable(output_plug->get_URI(), input_plug->get_URI()));
	
	//Create request for the performer who owns the input plug
	ZstMessages::PerformerConnection perf_args;
	perf_args.output_plug = output_plug->get_URI();
	perf_args.input_plug = input_plug->get_URI();
    
    ZstPerformerRef * output_performer = get_performer_ref_by_name(output_plug->get_URI().performer_char());
    ZstPerformerRef * input_performer = get_performer_ref_by_name(input_plug->get_URI().performer_char());
    assert(output_performer && input_performer);
    
	perf_args.endpoint = get_performer_endpoint(output_performer)->endpoint_address;

	zmsg_t *connectMsg = ZstMessages::build_message<ZstMessages::PerformerConnection>(ZstMessages::Kind::PERFORMER_REGISTER_CONNECTION, perf_args);
	zframe_t * empty = zframe_new_empty();
	zframe_t * identity = zframe_from(get_performer_endpoint(input_performer)->client_assigned_uuid.c_str());
	zmsg_prepend(connectMsg, &empty);
	zmsg_prepend(connectMsg, &identity);

	cout << "ZST_STAGE: Sending cable connection request to " << get_performer_endpoint(input_performer)->client_assigned_uuid.c_str() << endl;
	zmsg_send(&connectMsg, m_performer_router);
	return 1;
}

int ZstStage::destroy_cable(const ZstURI & uri){
    vector<ZstCable*> cables = get_cables_by_URI(uri);
    for (auto cable : cables) {
        if(!destroy_cable(cable)){
            cout << "ZST_STAGE: Couldn't remove cable" << endl;
			return 0;
        }
    }
	return 1;
}

int ZstStage::destroy_cable(ZstURI output_plug, ZstURI input_plug) {
    return destroy_cable(get_cable_by_URI(output_plug, input_plug));
}

int ZstStage::destroy_cable(ZstCable * cable) {
	if (cable != NULL) {
		cout << "ZST_STAGE: Destroying cable " << cable->get_output().to_char() << " " << cable->get_input().to_char() << endl;
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
        
        //let performers know this cable has left
        enqueue_stage_update(ZstEvent(out_uri, in_uri, ZstEvent::EventType::CABLE_DESTROYED));
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

ZstCable * ZstStage::get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB) {

	auto it = find_if(m_cables.begin(), m_cables.end(), [&uriA, &uriB](ZstCable * current) {
		return current->is_attached(uriA, uriB);
	});

	if (it != m_cables.end()) {
		return (*it);
	}
	return NULL;
}


void ZstStage::enqueue_stage_update(ZstEvent e)
{
	m_stage_updates.push(e);
}

vector<ZstEventWire> ZstStage::create_snapshot() {
	
	vector<ZstEventWire> stage_snapshot;
    vector<ZstPlugRef*> plugs = get_all_plug_refs();
	vector<ZstPerformerRef*> performers = get_all_performer_refs();

	for (auto performer : performers) {
		stage_snapshot.push_back(ZstEventWire(performer->get_URI(), ZstEvent::PERFORMER_CREATED));
	}

    for (auto plug : plugs) {
        stage_snapshot.push_back(ZstEventWire(plug->get_URI(), ZstEvent::PLUG_CREATED));
    }

    for (auto cable : m_cables) {
        stage_snapshot.push_back(ZstEventWire(cable->get_output(), cable->get_input(), ZstEvent::CABLE_CREATED));
    }

	return stage_snapshot;
}

int ZstStage::stage_update_timer_func(zloop_t * loop, int timer_id, void * arg)
{
	ZstStage *stage = (ZstStage*)arg;

	if (stage->m_stage_updates.size()) {
		vector<ZstEventWire> updates;
		while (stage->m_stage_updates.size()) {
			updates.push_back(ZstEventWire(stage->m_stage_updates.pop()));
		}

		ZstMessages::StageUpdates su;
		su.updates = updates;

		zmsg_t *updatemsg = ZstMessages::build_message<ZstMessages::StageUpdates>(ZstMessages::Kind::STAGE_UPDATE, su);
		zmsg_send(&updatemsg, stage->m_graph_update_pub);
	}
	return 0;
}

int ZstStage::stage_heartbeat_timer_func(zloop_t * loop, int timer_id, void * arg)
{
	ZstStage * stage = (ZstStage*)arg;
	map<string, ZstEndpointRef*> endpoints = stage->m_endpoint_refs;

	for (auto endpoint : endpoints) {
		if (endpoint.second->get_active_heartbeat()) {
			endpoint.second->clear_active_hearbeat();
		}
		else {
			cout << "ZST_STAGE: Endpoint " << endpoint.second->client_assigned_uuid << " missed a heartbeat. " << (MAX_MISSED_HEARTBEATS - endpoint.second->get_missed_heartbeats()) << " remaining" << endl;
			endpoint.second->set_heartbeat_inactive();
		}

		if (endpoint.second->get_missed_heartbeats() > MAX_MISSED_HEARTBEATS) {
			stage->destroy_endpoint(endpoint.second);
		}
	}
	return 0;
}
