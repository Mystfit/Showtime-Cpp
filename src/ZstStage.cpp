#include <sstream>
#include "ZstStage.h"
#include "ZstURI.h"
#include "ZstUtils.hpp"
#include "ZstEventWire.h"
#include "ZstEntityWire.h"

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
	zsock_destroy(&m_performer_requests);
	zsock_destroy(&m_performer_router);
	zsock_destroy(&m_graph_update_pub);
}

void ZstStage::init()
{
	ZstActor::init();
    
	std::stringstream addr;
	addr << "@tcp://*:" << STAGE_REP_PORT;
	m_performer_requests = zsock_new_rep(addr.str().c_str());
	addr.str("");
	zsock_set_linger(m_performer_requests, 0);
	attach_pipe_listener(m_performer_requests, s_handle_performer_requests, this);

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


std::vector<ZstPlugRef*> ZstStage::get_all_plug_refs(){
    return get_all_plug_refs(NULL);
}

std::vector<ZstPlugRef*> ZstStage::get_all_plug_refs(ZstEndpointRef * endpoint)
{
    vector<ZstPlugRef*> plugs;

    if(endpoint != NULL){
        plugs = endpoint->get_plug_refs();
    } else {
		vector<ZstPlugRef*> endpoint_plugs;
		for (auto endpoint : m_endpoint_refs) {
			endpoint_plugs = endpoint.second->get_plug_refs();
			plugs.insert(plugs.end(), endpoint_plugs.begin(), endpoint_plugs.end());
		}
    }
	
	return plugs;
}

ZstEndpointRef * ZstStage::get_plug_endpoint(ZstPlugRef * plug)
{
	ZstEndpointRef * result = NULL;
	for (auto endpoint : m_endpoint_refs) {
		if (endpoint.second->get_plug_by_URI(plug->get_URI()) != NULL) {
			result = endpoint.second;
			break;
		}
	}
	return result;
}

std::vector<ZstProxy*> ZstStage::get_all_entity_proxies()
{
	return get_all_entity_proxies(NULL);
}

std::vector<ZstProxy*> ZstStage::get_all_entity_proxies(ZstEndpointRef * endpoint)
{
	vector<ZstProxy*> all_entities;

	if (endpoint) {
		all_entities = endpoint->get_entity_proxies();
	}
	else {
		for (auto endpnt_iter : m_endpoint_refs) {
			vector<ZstProxy*> endpoint_performers = endpnt_iter.second->get_entity_proxies();
			all_entities.insert(all_entities.end(), endpoint_performers.begin(), endpoint_performers.end());
		}
	}
    
	return all_entities;
}

ZstEndpointRef * ZstStage::get_entity_endpoint(ZstEntityBase * entity)
{
	ZstEndpointRef * result = NULL;
    for (auto endpoint : m_endpoint_refs) {
		if (endpoint.second->get_entity_proxy_by_URI(entity->URI()) != NULL) {
			result = endpoint.second;
			break;
		}
	}
	return result;
}

ZstEndpointRef * ZstStage::create_endpoint(std::string starting_uuid, std::string endpoint)
{
	string new_uuid = "ENDPNT_" + (string)zuuid_str(zuuid_new());
	ZstEndpointRef * endpointRef = new ZstEndpointRef(starting_uuid, new_uuid, endpoint);
	m_endpoint_refs[new_uuid] = endpointRef;
	return endpointRef;
}

ZstEndpointRef * ZstStage::get_endpoint_ref_by_UUID(const char * uuid)
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
    char * sender_s = zmsg_popstr(msg);
	ZstEndpointRef * sender = stage->get_endpoint_ref_by_UUID(sender_s);
    zstr_free(&sender_s);

	if (sender == NULL) {
		//TODO: Can't return error to caller if the endpoint doesn't exist! Will need to implement timeouts
		//stage->reply_with_signal(socket, Signal::ENDPOINT_NOT_FOUND);
		return 0;
	}

	//Pop off empty frame
	zframe_t * empty = zmsg_pop(msg);
    zframe_destroy(&empty);

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
    } else if(message_type == ZstMessages::Kind::STAGE_CREATE_ENTITY) {
        //TODO:Create entity proxy on stage
        ZstEntityWire entity = ZstMessages::unpack_message_struct<ZstEntityWire>(msg);
        stage->create_entity_handler(entity, sender);
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
    
    ZstMessages::Signal result;
    bool send_reply = true;

	switch (message_type) {
	case ZstMessages::Kind::STAGE_CREATE_ENDPOINT:
        result = stage->create_endpoint_handler(socket, msg);
        send_reply = false;
		break;
	case ZstMessages::Kind::STAGE_CREATE_PLUG:
        result = stage->create_plug_handler(msg);
		break;
	case ZstMessages::Kind::STAGE_DESTROY_PLUG:
		result = stage->destroy_plug_handler(msg);
		break;
	case ZstMessages::Kind::STAGE_CREATE_CABLE:
		result =stage->create_cable_handler(msg);
		break;
	case ZstMessages::Kind::STAGE_DESTROY_CABLE:
		result = stage->destroy_cable_handler(msg);
		break;
	case ZstMessages::Kind::STAGE_DESTROY_ENTITY:
		result = stage->destroy_entity_handler(msg);
		break;
    case ZstMessages::Kind::STAGE_RUN_ENTITY_TEMPLATE:
        result = stage->create_entity_from_template_handler(msg);
        break;
	case ZstMessages::Kind::ENDPOINT_HEARTBEAT:
		result = stage->endpoint_heartbeat_handler(msg);
		break;
	default:
		cout << "ZST_STAGE: Didn't understand received message type of " << message_type << endl;
            result = ZstMessages::Signal::ERR_STAGE_MSG_TYPE_UNKNOWN;
		break;
	}
    
    if(send_reply)
        stage->reply_with_signal(socket, result);

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

ZstMessages::Signal ZstStage::create_endpoint_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::CreateEndpoint endpoint_args = ZstMessages::unpack_message_struct<ZstMessages::CreateEndpoint>(msg);
	cout << "ZST_STAGE: Registering new endpoint UUID " << endpoint_args.uuid << endl;

	ZstEndpointRef * endpointRef = create_endpoint(endpoint_args.uuid, endpoint_args.address);

	if (endpointRef == NULL) {
		return ZstMessages::Signal::ERR_STAGE_ENDPOINT_ALREADY_EXISTS;
	}

	ZstMessages::CreateEndpointAck ack_params;
	ack_params.assigned_uuid = endpointRef->client_assigned_uuid;

	zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::CreateEndpointAck>(ZstMessages::Kind::STAGE_CREATE_ENDPOINT_ACK, ack_params);
	zmsg_send(&ackmsg, socket);
    
    return ZstMessages::Signal::OK;
}

ZstMessages::Signal ZstStage::endpoint_heartbeat_handler(zmsg_t * msg) {
	ZstMessages::Heartbeat heartbeat_args = ZstMessages::unpack_message_struct<ZstMessages::Heartbeat>(msg);
	cout << "ZST_STAGE: Received heartbeat from " << heartbeat_args.from << ". Timestamp: " << heartbeat_args.timestamp << endl;
    return ZstMessages::Signal::OK;
}

ZstMessages::Signal ZstStage::create_plug_handler(zmsg_t *msg) {
	ZstMessages::CreatePlug plug_args = ZstMessages::unpack_message_struct<ZstMessages::CreatePlug>(msg);

	int end_index = static_cast<int>(plug_args.address.size()) - 2;
    ZstURI entity_URI = plug_args.address.range(0, end_index);
	ZstProxy* entity = get_entity_proxy_by_URI(entity_URI);

	if (entity == NULL) {
        cout << "ZST_STAGE: Couldn't register plug. No entity registered to stage with name " << entity_URI.path() << endl;
        return ZstMessages::Signal::ERR_STAGE_ENTITY_NOT_FOUND;
	}

	cout << "ZST_STAGE: Registering new plug " << plug_args.address.path() << endl;

	ZstEndpointRef * endpoint = get_entity_endpoint(entity);
	ZstPlugRef * plug = endpoint->create_plug(plug_args.address, plug_args.dir);

	if (plug == NULL) {
        cout << "ZST_STAGE: Plug already exists! " << plug_args.address.path() << endl;
		return ZstMessages::Signal::ERR_STAGE_PLUG_ALREADY_EXISTS;
	}
    
    ZstEvent e = ZstEvent(ZstEvent::EventType::PLUG_CREATED);
    e.add_parameter(plug->get_URI().path());
	enqueue_stage_update(e);
    
    return ZstMessages::Signal::OK;
}

ZstMessages::Signal ZstStage::create_entity_handler(ZstEntityWire & entity, ZstEndpointRef * endpoint)
{
	cout << "ZST_STAGE: Creating new entity " << entity.URI().path() << endl;
	if (endpoint == NULL) {
		//reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_ENDPOINT_NOT_FOUND;);
		return ZstMessages::Signal::ERR_STAGE_ENDPOINT_NOT_FOUND;
	}

	if (entity.URI().size() > 1) {

		int end_index = static_cast<int>(entity.URI().size()) - 2;
		ZstURI parent_uri = entity.URI().range(0, end_index);
		ZstProxy* entity_parent = get_entity_proxy_by_URI(parent_uri);
		if (entity_parent == NULL) {
			cout << "ZST_STAGE: Could not register entity " << entity.URI().path() << ", parent not found" << endl;
			return ZstMessages::Signal::ERR_STAGE_ENTITY_NOT_FOUND;
		}
	}

	ZstProxy * entity_proxy = endpoint->register_entity(entity);
	if (entity_proxy == NULL) {
		cout << "ZST_STAGE: Could not register entity " << entity.URI().path() << ", it already exists" << endl;
		return ZstMessages::Signal::ERR_STAGE_ENTITY_ALREADY_EXISTS;
	}
    
    ZstEvent e = ZstEvent(ZstEvent::EventType::ENTITY_CREATED);
    e.add_parameter(entity.URI().path());
	enqueue_stage_update(e);
    
    return ZstMessages::Signal::OK;
}

ZstMessages::Signal ZstStage::destroy_entity_handler(zmsg_t * msg)
{
	ZstMessages::DestroyURI entity_destroy_args = ZstMessages::unpack_message_struct<ZstMessages::DestroyURI>(msg);
	cout << "ZST_STAGE: Received destroy entity request for " << entity_destroy_args .address.path() << endl;

	ZstProxy *entity = get_entity_proxy_by_URI(entity_destroy_args.address);
	if (!entity) {
		return ZstMessages::Signal::ERR_STAGE_ENTITY_NOT_FOUND;
	}
	ZstEndpointRef* endpoint = get_entity_endpoint(entity);
	if (!endpoint) {
		return ZstMessages::Signal::ERR_STAGE_ENDPOINT_NOT_FOUND;
	}

	endpoint->destroy_entity(entity);
    
    ZstEvent e = ZstEvent(ZstEvent::EventType::ENTITY_DESTROYED);
    e.add_parameter(entity_destroy_args.address.path());
	enqueue_stage_update(e);
    
    return ZstMessages::Signal::OK;
}

ZstMessages::Signal ZstStage::destroy_plug_handler(zmsg_t * msg)
{
	ZstMessages::DestroyURI plug_destroy_args = ZstMessages::unpack_message_struct<ZstMessages::DestroyURI>(msg);
	cout << "ZST_STAGE: Received destroy plug request for " << plug_destroy_args.address.path() << endl;

	int end_index = static_cast<int>(plug_destroy_args.address.size()) - 2;
	ZstProxy *entity = get_entity_proxy_by_URI(plug_destroy_args.address.range(0, end_index));
	if (!entity) {
		return ZstMessages::Signal::ERR_STAGE_ENTITY_NOT_FOUND;
	}
	ZstEndpointRef* endpoint = get_entity_endpoint(entity);
	if (!endpoint) {
		return ZstMessages::Signal::ERR_STAGE_ENDPOINT_NOT_FOUND;
	}

	ZstPlugRef* plugRef = get_plug_by_URI(plug_destroy_args.address);
	if (plugRef != NULL) {
        destroy_cable(plugRef->get_URI());
	}

	endpoint->destroy_plug(plug_destroy_args.address);
    
    ZstEvent e = ZstEvent(ZstEvent::EventType::PLUG_DESTROYED);
    e.add_parameter(plug_destroy_args.address.path());
	enqueue_stage_update(e);
    
    return ZstMessages::Signal::OK;
}


// ----------------
// Router endpoints
ZstMessages::Signal ZstStage::create_cable_handler(zmsg_t * msg)
{
	ZstMessages::CreateCable plug_args = ZstMessages::unpack_message_struct<ZstMessages::CreateCable>(msg);
	cout << "ZST_STAGE: Received connect cable request for " << plug_args.first.path() << " and " << plug_args.second.path() << endl;
	int connect_status = 0;

	ZstPlugRef * plug_A = get_plug_by_URI(plug_args.first);
	ZstPlugRef * plug_B = get_plug_by_URI(plug_args.second);

    if (plug_A && plug_B) {
        if (plug_A->get_direction() == PlugDirection::OUT_JACK && plug_B->get_direction() == PlugDirection::IN_JACK) {
            connect_status = connect_cable(plug_A, plug_B);
        }
        else if (plug_A->get_direction() == PlugDirection::IN_JACK && plug_B->get_direction() == PlugDirection::OUT_JACK) {
            connect_status = connect_cable(plug_B, plug_A);
        }
    } else {
        cout << "ZST_STAGE: Missing plugs for cable connection!" << endl;
        return ZstMessages::Signal::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
    }
   
	if (!connect_status) {
		cout << "ZST_STAGE: Bad cable connect request" << endl;
		return ZstMessages::Signal::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
	}

    ZstEvent e = ZstEvent(ZstEvent::EventType::CABLE_CREATED);
    e.add_parameter(plug_args.first.path());
    e.add_parameter(plug_args.second.path());
    enqueue_stage_update(e);
    
    return ZstMessages::Signal::OK;
}

ZstMessages::Signal ZstStage::create_entity_from_template_handler(zmsg_t * msg)
{
    ZstURIWire template_path = ZstMessages::unpack_message_struct<ZstURIWire>(msg);
    cout << "ZST_STAGE: Received entity template creation request for " << template_path.path() << endl;
    
    ZstProxy * entity_template = get_entity_proxy_by_URI(template_path);
    ZstEndpointRef * endpoint = get_entity_endpoint(entity_template);
    
    return ZstMessages::Signal::OK;
}


ZstMessages::Signal ZstStage::destroy_cable_handler(zmsg_t * msg)
{
	ZstMessages::CreateCable connection_destroy_args = ZstMessages::unpack_message_struct<ZstMessages::CreateCable>(msg);
	cout << "ZST_STAGE: Received destroy cable connection request" << endl;

	if (!destroy_cable(connection_destroy_args.first, connection_destroy_args.second)) {
		return ZstMessages::Signal::ERR_STAGE_BAD_CABLE_DISCONNECT_REQUEST;
	}
    
    ZstEvent e = ZstEvent(ZstEvent::EventType::CABLE_DESTROYED);
    e.add_parameter(connection_destroy_args.first.path());
    e.add_parameter(connection_destroy_args.second.path());
    enqueue_stage_update(e);
    
    return ZstMessages::Signal::OK;
}


ZstProxy * ZstStage::get_entity_proxy_by_URI(const ZstURI & uri)
{
	ZstProxy * result = NULL;
	vector<ZstProxy*> entities = get_all_entity_proxies(NULL);
	for (auto entity_iter : entities) {
		if (ZstURI::equal(entity_iter->URI(), uri)) {
			result = entity_iter;
			break;
		}
	}
	return result;
}

ZstPlugRef * ZstStage::get_plug_by_URI(ZstURI uri)
{
	ZstPlugRef* result = NULL;
	vector<ZstPlugRef*> plugs = get_all_plug_refs();
	for (auto plug : plugs) {
		if (ZstURI::equal(plug->get_URI(), uri)) {
			result = plug;
			break;
		}
	}
	return result;
}

int ZstStage::connect_cable(ZstPlugRef * output_plug, ZstPlugRef * input_plug)
{
	if (get_cable_by_URI(output_plug->get_URI(), input_plug->get_URI()) != NULL) {
		cout << "ZST_STAGE: Cable already exists for " << output_plug->get_URI().path() << " and " << input_plug->get_URI().path() << endl;
		return 0;
	}
	m_cables.push_back(new ZstCable(output_plug->get_URI(), input_plug->get_URI()));
	
	//Create request for the entity who owns the input plug
	ZstMessages::PerformerConnection perf_args;
	perf_args.output_plug = output_plug->get_URI();
	perf_args.input_plug = input_plug->get_URI();
    
	ZstEndpointRef * output_endpoint = get_plug_endpoint(output_plug);
	ZstEndpointRef * input_endpoint = get_plug_endpoint(input_plug);
    
	perf_args.endpoint = output_endpoint->endpoint_address;

	zmsg_t *connectMsg = ZstMessages::build_message<ZstMessages::PerformerConnection>(ZstMessages::Kind::PERFORMER_REGISTER_CONNECTION, perf_args);
	zframe_t * empty = zframe_new_empty();
	zframe_t * identity = zframe_from(input_endpoint->client_assigned_uuid.c_str());
	zmsg_prepend(connectMsg, &empty);
	zmsg_prepend(connectMsg, &identity);

	cout << "ZST_STAGE: Sending cable connection request to " << input_endpoint->client_assigned_uuid.c_str() << endl;
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
        
        //let performers know this cable has left
        ZstEvent e = ZstEvent(ZstEvent::EventType::CABLE_DESTROYED);
        e.add_parameter(out_uri.path());
        e.add_parameter(in_uri.path());
        enqueue_stage_update(e);
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
	vector<ZstProxy*> entities = get_all_entity_proxies(NULL);
    ZstEvent e;

	for (auto entity : entities) {
        e = ZstEventWire(ZstEvent::ENTITY_CREATED);
        e.add_parameter(entity->URI().path());
		stage_snapshot.push_back(e);
	}

    for (auto plug : plugs) {
        e = ZstEventWire(ZstEvent::PLUG_CREATED);
        e.add_parameter(plug->get_URI().path());
        stage_snapshot.push_back(e);
    }

    for (auto cable : m_cables) {
        e = ZstEventWire(ZstEvent::CABLE_CREATED);
        e.add_parameter(cable->get_output().path());
        e.add_parameter(cable->get_input().path());
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

		ZstMessages::StageUpdates su = { updates };

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
