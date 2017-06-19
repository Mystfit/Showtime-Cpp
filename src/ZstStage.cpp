#include "ZstStage.h"

using namespace std;

ZstStage::ZstStage()
{
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

		for (vector<ZstPerformerRef*>::iterator perf_iter = performers.begin(); perf_iter != performers.end(); ++perf_iter) {
			plugs.insert(plugs.end(), (*perf_iter)->get_plug_refs().begin(), (*perf_iter)->get_plug_refs().end());
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

ZstPerformerRef * ZstStage::get_performer_ref_by_name(string performer_name) {

	ZstPerformerRef * performer = NULL;
	for (map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter) {
		performer = endpnt_iter->second->get_performer_by_name(performer_name);
		if (performer != NULL)
			break;
	}
	return performer;
}

ZstEndpointRef * ZstStage::get_performer_endpoint(ZstPerformerRef * performer)
{
	ZstEndpointRef * endpoint = NULL;
	for (map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter) {
		if (endpnt_iter->second->get_performer_by_name(performer->name) != NULL) {
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
	zmsg_print(msg);

	//Get endpoint	
	ZstEndpointRef * sender = stage->get_endpoint_ref_by_UUID(zmsg_popstr(msg));

	if (sender == NULL) {
		//TODO: Can't return error to caller if the endpoint doesn't exist! Will need to implement timeouts
		//stage->reply_with_signal(socket, Signal::ENDPOINT_NOT_FOUND);
		return 0;
	}

	//Get message type
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
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
	case ZstMessages::Kind::STAGE_LIST_PLUGS:
		stage->list_plugs_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_LIST_PLUG_CONNECTIONS:
		stage->list_plug_connections_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_DESTROY_PLUG:
		stage->destroy_plug_handler(socket, msg);
		break;
	case ZstMessages::Kind::STAGE_REGISTER_CONNECTION:
		stage->connect_plugs_handler(socket, msg);
		break;
	case ZstMessages::Kind::ENDPOINT_HEARTBEAT:
		stage->endpoint_heartbeat_handler(socket, msg);
		break;
	default:
		cout << "Didn't understand received message type of " << message_type << endl;
		break;
	}

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

// --------------
// Reply handlers

void ZstStage::register_endpoint_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::RegisterEndpoint endpoint_args = ZstMessages::unpack_message_struct<ZstMessages::RegisterEndpoint>(msg);
	cout << "STAGE: Registering new endpoint UUID " << endpoint_args.uuid << endl;

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
	cout << "STAGE: Received heartbeat from " << heartbeat_args.from << ". Timestamp: " << heartbeat_args.timestamp << endl;

	reply_with_signal(socket, ZstMessages::Signal::OK);
}


void ZstStage::register_performer_handler(zsock_t * socket, zmsg_t * msg) {
	ZstMessages::RegisterPerformer performer_args = ZstMessages::unpack_message_struct<ZstMessages::RegisterPerformer>(msg);
	cout << "STAGE: Registering new performer " << performer_args.name << endl;

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
}


void ZstStage::register_plug_handler(zsock_t *socket, zmsg_t *msg) {
	ZstMessages::RegisterPlug plug_args = ZstMessages::unpack_message_struct<ZstMessages::RegisterPlug>(msg);

	ZstPerformerRef * performer = get_performer_ref_by_name(plug_args.address.performer());

	if (performer == NULL) {
		cout << "STAGE: Couldn't register plug. No performer registered to stage with name";
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_PERFORMER_NOT_FOUND);
		return;
	}

	cout << "STAGE: Registering new plug " << plug_args.address.to_str() << endl;

	ZstPlugRef * plug = performer->create_plug(plug_args.address);

	if (plug == NULL) {
        cout << "STAGE: Plug already exists! " << plug_args.address.to_str() << endl;
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_PLUG_ALREADY_EXISTS);
		return;
	}

	reply_with_signal(socket, ZstMessages::Signal::OK);
}


void ZstStage::list_plugs_handler(zsock_t *socket, zmsg_t *msg) {
	ZstMessages::ListPlugs plug_query_args = ZstMessages::unpack_message_struct<ZstMessages::ListPlugs>(msg);
	cout << "STAGE: Received list plugs request" << endl;

	ZstMessages::ListPlugsAck response;

	for (map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter)
	{
		vector<ZstPlugRef*> plugs;
		if (plug_query_args.performer.empty()) {
			plugs = get_all_plug_refs();
		}
		else {
			ZstPerformerRef* endpoint = endpnt_iter->second->get_performer_by_name(plug_query_args.performer);
			if (endpoint != NULL) {
				plugs = endpnt_iter->second->get_performer_by_name(plug_query_args.performer)->get_plug_refs();
			}
		}

		//Pull addressess from plug refs
		for (vector<ZstPlugRef*>::iterator plug_iter = plugs.begin(); plug_iter != plugs.end(); ++plug_iter) {
			response.plugs.push_back((*plug_iter)->get_URI());
		}
	}

	zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::ListPlugsAck>(ZstMessages::Kind::STAGE_LIST_PLUGS_ACK, response);
	zmsg_send(&ackmsg, socket);
}

void ZstStage::list_plug_connections_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::ListPlugs plug_query_args = ZstMessages::unpack_message_struct<ZstMessages::ListPlugs>(msg);
	cout << "STAGE: Received list plug connections request" << endl;

	ZstMessages::ListPlugConnectionsAck response;

	for (map<string, ZstEndpointRef*>::iterator endpnt_iter = m_endpoint_refs.begin(); endpnt_iter != m_endpoint_refs.end(); ++endpnt_iter)
	{
		//Build tuple of each plug connection

		vector<ZstPlugRef*> plugs;
		if (plug_query_args.performer.empty()) {
			plugs = get_all_plug_refs();
		}
		else {
			ZstPerformerRef* endpoint = endpnt_iter->second->get_performer_by_name(plug_query_args.performer);
			if (endpoint != NULL) {
				plugs = endpnt_iter->second->get_performer_by_name(plug_query_args.performer)->get_plug_refs();
			}
		}

		//Pull URI from plug refs per connection
		for (vector<ZstPlugRef*>::iterator plug_iter = plugs.begin(); plug_iter != plugs.end(); ++plug_iter) {
			if ((*plug_iter)->get_output_connections().size() > 0) {
				vector<ZstURI> connections = (*plug_iter)->get_output_connections();
				for (vector<ZstURI>::iterator out_plug_iter = connections.begin(); out_plug_iter != connections.end(); ++out_plug_iter) {
					response.plug_connections.push_back(pair<ZstURI, ZstURI>((*plug_iter)->get_URI(), (*out_plug_iter)));
				}
			}
		}
	}

	zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::ListPlugConnectionsAck>(ZstMessages::Kind::STAGE_LIST_PLUG_CONNECTIONS_ACK, response);
	zmsg_send(&ackmsg, socket);
}


void ZstStage::destroy_plug_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::DestroyPlug plug_destroy_args = ZstMessages::unpack_message_struct<ZstMessages::DestroyPlug>(msg);
	cout << "STAGE: Received destroy plug request" << endl;

	ZstPerformerRef *performer = get_performer_ref_by_name(plug_destroy_args.address.performer());

	performer->destroy_plug(performer->get_plug_by_URI(plug_destroy_args.address.to_str()));

	reply_with_signal(socket, ZstMessages::Signal::OK);
}


// ----------------
// Router endpoints
void ZstStage::connect_plugs_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::ConnectPlugs plug_args = ZstMessages::unpack_message_struct<ZstMessages::ConnectPlugs>(msg);
	cout << "STAGE: Received connect plug request for " << plug_args.first.to_str() << " and " << plug_args.second.to_str() << endl;
	int connect_status = 0;
	if (plug_args.first.direction() == ZstURI::Direction::OUT_JACK && plug_args.second.direction() == ZstURI::Direction::IN_JACK) {
		connect_status = connect_plugs(plug_args.first, plug_args.second);
	}
	else if (plug_args.first.direction() == ZstURI::Direction::IN_JACK && plug_args.second.direction() == ZstURI::Direction::OUT_JACK) {
		connect_status = connect_plugs(plug_args.second, plug_args.first);
	}
	else {
		connect_status = -1;
	}

	if (connect_status != 0) {
		cout << "STAGE: Bad plug connect request" << endl;
		reply_with_signal(socket, ZstMessages::Signal::ERR_STAGE_BAD_PLUG_CONNECT_REQUEST);
		return;
	}

	reply_with_signal(socket, ZstMessages::Signal::OK);
}

int ZstStage::connect_plugs(ZstURI output_plug, ZstURI input_plug)
{
    ZstPerformerRef * output_performer = get_performer_ref_by_name(output_plug.performer());
    ZstPerformerRef * input_performer = get_performer_ref_by_name(input_plug.performer());
    
	if (!(output_performer && input_performer)) {
		return -1;
	}

	output_performer->get_plug_by_URI(output_plug.to_str())->add_output_connection(input_plug);

	ZstMessages::PerformerConnection perf_args;
	perf_args.output_plug = output_plug;
	perf_args.input_plug = input_plug;
	perf_args.endpoint = get_performer_endpoint(output_performer)->endpoint_address;

	zmsg_t *connectMsg = ZstMessages::build_message<ZstMessages::PerformerConnection>(ZstMessages::Kind::PERFORMER_REGISTER_CONNECTION, perf_args);
	zframe_t * empty = zframe_new_empty();
	zframe_t * identity = zframe_from(get_performer_endpoint(input_performer)->client_assigned_uuid.c_str());
	zmsg_prepend(connectMsg, &empty);
	zmsg_prepend(connectMsg, &identity);

	cout << "STAGE: Sending plug connection request to " << get_performer_endpoint(input_performer)->client_assigned_uuid.c_str() << endl;
	zmsg_send(&connectMsg, m_performer_router);
	return 0;
}

