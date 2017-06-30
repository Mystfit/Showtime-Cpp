#include "ZstStage.h"
#include "ZstURIWire.h"

using namespace std;

ZstStage::ZstStage()
{
	m_stage_identity = ZstURI("stage", "", "", ZstURI::Direction::NONE);
}

ZstStage::~ZstStage()
{
	ZstActor::~ZstActor();

	//Close stage pipes
	zsock_destroy(&m_performer_requests);
	zsock_destroy(&m_performer_router);
	zsock_destroy(&m_graph_update_pub);
}

void ZstStage::init()
{
	ZstActor::init();
	m_performer_requests = zsock_new_rep("@tcp://*:6000");
	zsock_set_linger(m_performer_requests, 0);
	attach_pipe_listener(m_performer_requests, s_handle_performer_requests, this);

	m_performer_router = zsock_new(ZMQ_ROUTER);
	zsock_set_linger(m_performer_router, 0);
	attach_pipe_listener(m_performer_router, s_handle_router, this);
	zsock_bind(m_performer_router, "tcp://*:6001");

	m_graph_update_pub = zsock_new_pub("@tcp://*:6002");
	zsock_set_linger(m_graph_update_pub, 0);

	int m_update_timer_id = attach_timer(stage_update_timer_func, 50, this);

	start();
}

ZstStage* ZstStage::create_stage()
{
	ZstStage* stage = new ZstStage();
	stage->init();
	return stage;
}

std::vector<ZstPlugRef*> ZstStage::get_all_plug_refs()
{
	vector<ZstPlugRef*> plugs;
	for (map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter)
	{
		vector<ZstPerformerRef*> performers = get_all_performer_refs();
		if (performers.size() > 0) {
			for (vector<ZstPerformerRef*>::iterator perf_iter = performers.begin(); perf_iter != performers.end(); ++perf_iter) {
				plugs.insert(plugs.end(), (*perf_iter)->get_plug_refs().begin(), (*perf_iter)->get_plug_refs().end());
			}
		}	
	}
	return plugs;
}

std::vector<ZstPerformerRef*> ZstStage::get_all_performer_refs()
{
	vector<ZstPerformerRef*> all_performers;
	for (map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter) {
		vector<ZstPerformerRef*> endpoint_performers = endpnt_iter->second->get_performer_refs();
		all_performers.insert(all_performers.end(), endpoint_performers.begin(), endpoint_performers.end());
	}

	return all_performers;
}

ZstPerformerRef * ZstStage::get_performer_ref_by_name(const char * performer_name) {

	ZstPerformerRef * performer = NULL;
	for (map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter) {
		performer = endpnt_iter->second->get_performer_by_name(string(performer_name));
		if (performer != NULL)
			break;
	}
	return performer;
}

ZstEndpointRef * ZstStage::get_performer_endpoint(ZstPerformerRef * performer)
{
	ZstEndpointRef * endpoint = NULL;
	for (map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter) {
		if (endpnt_iter->second->get_performer_by_name(performer->get_URI().performer()) != NULL) {
			endpoint = endpnt_iter->second;
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
	for (std::map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter)
	{
		if ((endpnt_iter->second) == endpoint)
		{
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
	case ZstMessages::Kind::STAGE_REGISTER_ENDPOINT:
		stage->register_endpoint_handler(socket, msg);
		break;		
	case ZstMessages::Kind::STAGE_REGISTER_PERFORMER:
		stage->register_performer_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_REGISTER_PLUG:
		stage->register_plug_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_DESTROY_PLUG:
		stage->destroy_plug_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_REGISTER_CABLE:
		stage->connect_cable_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_DESTROY_CABLE:
		stage->disconnect_cable_handler(socket, msg);
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

void ZstStage::register_endpoint_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::RegisterEndpoint endpoint_args = ZstMessages::unpack_message_struct<ZstMessages::RegisterEndpoint>(msg);
	cout << "ZST_STAGE: Registering new endpoint UUID " << endpoint_args.uuid << endl;

	ZstEndpointRef * endpointRef = create_endpoint(endpoint_args.uuid, endpoint_args.address);

	if (endpointRef == NULL) {
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_ENDPOINT_ALREADY_EXISTS);
		return;
	}

	ZstMessages::RegisterEndpointAck ack_params;
	ack_params.assigned_uuid = endpointRef->client_assigned_uuid;

	zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::RegisterEndpointAck>(ZstMessages::Kind::STAGE_REGISTER_ENDPOINT_ACK, ack_params);
	zmsg_send(&ackmsg, socket);
}

void ZstStage::endpoint_heartbeat_handler(zsock_t * socket, zmsg_t * msg) {
	ZstMessages::Heartbeat heartbeat_args = ZstMessages::unpack_message_struct<ZstMessages::Heartbeat>(msg);
	cout << "ZST_STAGE: Received heartbeat from " << heartbeat_args.from << ". Timestamp: " << heartbeat_args.timestamp << endl;

	reply_with_signal(socket, ZstMessages::Signal::OK);
}


void ZstStage::register_performer_handler(zsock_t * socket, zmsg_t * msg) {
	ZstMessages::RegisterPerformer performer_args = ZstMessages::unpack_message_struct<ZstMessages::RegisterPerformer>(msg);
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
	enqueue_stage_update(ZstEvent(performerRef->get_URI(), ZstEvent::EventType::CREATED));
}


void ZstStage::register_plug_handler(zsock_t *socket, zmsg_t *msg) {
	ZstMessages::RegisterPlug plug_args = ZstMessages::unpack_message_struct<ZstMessages::RegisterPlug>(msg);

	ZstPerformerRef * performer = get_performer_ref_by_name(plug_args.address.performer_char());

	if (performer == NULL) {
		cout << "ZST_STAGE: Couldn't register plug. No performer registered to stage with name";
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_PERFORMER_NOT_FOUND);
		return;
	}

	cout << "ZST_STAGE: Registering new plug " << plug_args.address.to_char() << endl;

	ZstPlugRef * plug = performer->create_plug(plug_args.address);

	if (plug == NULL) {
        cout << "ZST_STAGE: Plug already exists! " << plug_args.address.to_char() << endl;
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_PLUG_ALREADY_EXISTS);
		return;
	}

	reply_with_signal(socket, ZstMessages::Signal::OK);
	enqueue_stage_update(ZstEvent(plug->get_URI(), ZstEvent::EventType::CREATED));
}


void ZstStage::destroy_plug_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::DestroyPlug plug_destroy_args = ZstMessages::unpack_message_struct<ZstMessages::DestroyPlug>(msg);
	cout << "ZST_STAGE: Received destroy plug request" << endl;

	ZstPerformerRef *performer = get_performer_ref_by_name(plug_destroy_args.address.performer_char());

	ZstPlugRef* plugRef = performer->get_plug_by_URI(plug_destroy_args.address);
	if (plugRef != NULL) {

		//Need to remove all cables attached to this plug
		vector<ZstCable*> connections = get_cables_by_URI(plugRef->get_URI());
		for (vector<ZstCable*>::iterator cable_iter = connections.begin(); cable_iter != connections.end(); ++cable_iter) {

			ZstURI out_URI = (*cable_iter)->get_output();
			ZstURI in_URI = (*cable_iter)->get_input();

			if (destroy_cable(out_URI, in_URI)) {
				enqueue_stage_update(ZstEvent(out_URI, in_URI, ZstEvent::EventType::CABLE_DESTROYED));
			}
		}
	}

	performer->destroy_plug(performer->get_plug_by_URI(plug_destroy_args.address));

	reply_with_signal(socket, ZstMessages::Signal::OK);
	enqueue_stage_update(ZstEvent(plug_destroy_args.address, ZstEvent::EventType::DESTROYED));
}


// ----------------
// Router endpoints
void ZstStage::connect_cable_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::PlugConnection plug_args = ZstMessages::unpack_message_struct<ZstMessages::PlugConnection>(msg);
	cout << "ZST_STAGE: Received connect cable request for " << plug_args.first.to_char() << " and " << plug_args.second.to_char() << endl;
	int connect_status = 0;
	if (plug_args.first.direction() == ZstURI::Direction::OUT_JACK && plug_args.second.direction() == ZstURI::Direction::IN_JACK) {
		connect_status = connect_cable(plug_args.first, plug_args.second);
	}
	else if (plug_args.first.direction() == ZstURI::Direction::IN_JACK && plug_args.second.direction() == ZstURI::Direction::OUT_JACK) {
		connect_status = connect_cable(plug_args.second, plug_args.first);
	}

	if (!connect_status) {
		cout << "ZST_STAGE: Bad cable connect request" << endl;
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_BAD_PLUG_CONNECT_REQUEST);
		return;
	}

	reply_with_signal(socket, ZstMessages::Signal::OK);
	enqueue_stage_update(ZstEvent(plug_args.first, plug_args.second, ZstEvent::EventType::CABLE_CREATED));
}

void ZstStage::disconnect_cable_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::PlugConnection connection_destroy_args = ZstMessages::unpack_message_struct<ZstMessages::PlugConnection>(msg);
	cout << "ZST_STAGE: Received destroy cable connection request" << endl;

	if (!destroy_cable(connection_destroy_args.first, connection_destroy_args.second)) {
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_BAD_PLUG_DISCONNECT_REQUEST);
		return;
	}

	reply_with_signal(socket, ZstMessages::Signal::OK);
	enqueue_stage_update(ZstEvent(connection_destroy_args.first, connection_destroy_args.second, ZstEvent::EventType::CABLE_DESTROYED));
}

int ZstStage::connect_cable(ZstURI output_plug, ZstURI input_plug)
{
    ZstPerformerRef * output_performer = get_performer_ref_by_name(output_plug.performer_char());
    ZstPerformerRef * input_performer = get_performer_ref_by_name(input_plug.performer_char());
    
	if (!(output_performer && input_performer)) {
		cout << "ZST_STAGE: Cable registration. Missing performer." << endl;
		return 0;
	}

	if (get_cable_by_URI(output_plug, input_plug) != NULL) {
		cout << "ZST_STAGE: Cable already exists for " << output_plug.to_char() << " and " << input_plug.to_char() << endl;
		return 0;
	}
	m_cables.push_back(new ZstCable(output_plug, input_plug));
	
	//Create request for the performer who owns the input plug
	ZstMessages::PerformerConnection perf_args;
	perf_args.output_plug = output_plug;
	perf_args.input_plug = input_plug;
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

int ZstStage::destroy_cable(ZstURI output_plug, ZstURI input_plug) {
	cout << "ZST_STAGE: Disconnecting cable " << output_plug.to_char() << " and " << input_plug.to_char() << endl;
	ZstCable * cable = get_cable_by_URI(output_plug, input_plug);

	if (cable != NULL) {
		for (vector<ZstCable*>::iterator cable_iter = m_cables.begin(); cable_iter != m_cables.end(); ++cable_iter){
			if ((*cable_iter) == cable){
				m_cables.erase(cable_iter);
				break;
			}
		}
		delete cable;
		return 1;
	}
	return 0;
}


vector<ZstCable*> ZstStage::get_cables_by_URI(const ZstURI & uri) {

	vector<ZstCable*> cables;
	auto it = find_if(m_cables.begin(), m_cables.end(), [&uri](ZstCable* current) {
		return *current == uri;
	});
	
	for (it; it != m_cables.end(); ++it) {
		cables.push_back((*it));
	}

	return cables;
}

ZstCable * ZstStage::get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB) {

	auto it = find_if(m_cables.begin(), m_cables.end(), [&uriA, &uriB](ZstCable * current) {
		return (*current == uriA) && (*current == uriB);
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

vector<ZstEvent> ZstStage::create_snapshot() {
	
	vector<ZstEvent> stage_snapshot;

	for (map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter)
	{
		vector<ZstPlugRef*> plugs = get_all_plug_refs();

		//Pull addressess from plug refs
		for (vector<ZstPlugRef*>::iterator plug_iter = plugs.begin(); plug_iter != plugs.end(); ++plug_iter) {
			stage_snapshot.push_back(ZstEvent((*plug_iter)->get_URI(), ZstEvent::CREATED));

			for (vector<ZstCable*>::iterator cable_iter = m_cables.begin(); cable_iter != m_cables.end(); ++cable_iter) {
				stage_snapshot.push_back(ZstEvent((*cable_iter)->get_output(), (*cable_iter)->get_input(), ZstEvent::CABLE_CREATED));
			}
		}
	}

	return stage_snapshot;
}

int ZstStage::stage_update_timer_func(zloop_t * loop, int timer_id, void * arg)
{
	ZstStage *stage = (ZstStage*)arg;

	if (stage->m_stage_updates.size()) {
		vector<ZstEvent> updates;
		while (stage->m_stage_updates.size()) {
			updates.push_back(stage->m_stage_updates.pop());
		}

		ZstMessages::StageUpdates su;
		su.updates = updates;

		zmsg_t *updatemsg = ZstMessages::build_message<ZstMessages::StageUpdates>(ZstMessages::Kind::STAGE_UPDATE, su);
		zmsg_send(&updatemsg, stage->m_graph_update_pub);
	}
	return 0;
}
