#include "ZstStage.h"

using namespace std;

ZstStage::ZstStage()
{
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

ZstStage::~ZstStage()
{
	ZstActor::~ZstActor();

	//Close stage pipes
    zsock_destroy(&m_performer_requests);
	zsock_destroy(&m_performer_router);
	zsock_destroy(&m_graph_update_pub);
}

ZstStage* ZstStage::create_stage()
{
    ZstStage* stage = new ZstStage();
    return stage;
}

ZstPerformerRef & ZstStage::get_performer_ref(string performer_name){
    map<string, ZstPerformerRef>::iterator performerRefIter = m_performer_refs.find(performer_name);
    
    if(performerRefIter == m_performer_refs.end())
        throw out_of_range("Could not find section");

    return performerRefIter->second;
}


int ZstStage::s_handle_router(zloop_t * loop, zsock_t * socket, void * arg)
{
    ZstStage *stage = (ZstStage*)arg;
    
    //Receive waiting message
    zmsg_t *msg = zmsg_recv(socket);
    zmsg_print(msg);
    
    //Get identity
    zframe_t *identity = zmsg_pop(msg);
    
    //Get message type
    ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
    switch (message_type) {
        default:
            break;
    }
}


int ZstStage::s_handle_performer_requests(zloop_t * loop, zsock_t * socket, void * arg)
{
    ZstStage *stage = (ZstStage*)arg;
    
    //Receive waiting message
	zmsg_t *msg = zmsg_recv(socket);
    
    //Get message type
    ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
    
    switch (message_type) {
        case ZstMessages::Kind::STAGE_REGISTER_PERFORMER:
            stage->register_performer_handler(socket, msg);
            break;
        case ZstMessages::Kind::PERFORMER_HEARTBEAT:
            stage->section_heartbeat_handler(socket, msg);
            break;
        case ZstMessages::Kind::STAGE_REGISTER_PLUG:
            stage->register_plug_handler(socket, msg);
            break;
        case ZstMessages::Kind::STAGE_LIST_PLUGS:
            stage->list_plugs_handler(socket, msg);
            break;
		case ZstMessages::Kind::STAGE_DESTROY_PLUG:
			stage->destroy_plug_handler(socket, msg);
			break;
		case ZstMessages::Kind::STAGE_REGISTER_CONNECTION:
			stage->connect_plugs_handler(socket, msg);
			break;
        default:
            cout << "Didn't understand received message type of " << message_type << endl;
            break;
    }

	return 0;
}


void ZstStage::register_performer_handler(zsock_t * socket, zmsg_t * msg){
	ZstMessages::RegisterPerformer performer_args = ZstMessages::unpack_message_struct<ZstMessages::RegisterPerformer>(msg);
    
    if(m_performer_refs.count(performer_args.name) > 0){
        cout << "STAGE: Performer already registered!" << endl;
        return;
    }
    cout << "STAGE: Registering new performer " << performer_args.name << endl;

    ZstPerformerRef performerRef;
    performerRef.name = performer_args.name;
    performerRef.endpoint = performer_args.endpoint;
    m_performer_refs[performer_args.name] = performerRef;
    
    zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::OKAck>(ZstMessages::Kind::OK, ZstMessages::OKAck());
    zmsg_send(&ackmsg, socket);
}


void ZstStage::register_plug_handler(zsock_t *socket, zmsg_t *msg){
	ZstMessages::RegisterPlug plug_args = ZstMessages::unpack_message_struct<ZstMessages::RegisterPlug>(msg);
    
    if(m_performer_refs.empty()){
       cout << "STAGE: Couldn't register plug. No section registered to stage with name";
    }
    
    cout << "STAGE: Registering new plug " << plug_args.name << endl;
    PlugAddress plugRef;
    plugRef.name = plug_args.name;
    plugRef.instrument = plug_args.instrument;
    plugRef.performer = plug_args.performer;
	plugRef.direction = plug_args.direction;
    
    m_performer_refs[plug_args.performer].plugs.push_back(plugRef);
    
    zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::OKAck>(ZstMessages::Kind::OK, ZstMessages::OKAck());
    zmsg_send(&ackmsg, socket);
}


void ZstStage::section_heartbeat_handler(zsock_t * socket, zmsg_t * msg){
	ZstMessages::Heartbeat heartbeat_args = ZstMessages::unpack_message_struct<ZstMessages::Heartbeat>(msg);
    cout << "STAGE: Received heartbeat from " << heartbeat_args.from << ". Timestamp: " << heartbeat_args.timestamp << endl;
    
    zmsg_t *ackmsg = ZstMessages::build_message(ZstMessages::Kind::OK, ZstMessages::OKAck());
    zmsg_send(&ackmsg, socket);
}


void ZstStage::list_plugs_handler(zsock_t *socket, zmsg_t *msg){
	ZstMessages::ListPlugs plug_query_args = ZstMessages::unpack_message_struct<ZstMessages::ListPlugs>(msg);
    cout << "STAGE: Received list plugs request" << endl;
    
	ZstMessages::ListPlugsAck response;
    
    //Filter plugs based on query args
    if(plug_query_args.performer.empty()){
        //Return all plugs
        for(map<string,ZstPerformerRef>::iterator performerIter = m_performer_refs.begin(); performerIter != m_performer_refs.end(); ++performerIter) {
            response.plugs.insert(response.plugs.end(), performerIter->second.plugs.begin(), performerIter->second.plugs.end());
        }
    } else {
        //Return plugs from named section
        vector<PlugAddress> plugs = m_performer_refs[plug_query_args.performer].plugs;
        if(plug_query_args.instrument.empty()){
            //Return all plugs from section
            response.plugs = m_performer_refs[plug_query_args.performer].plugs;
        } else {
            //Return only named instrument plugs from section
            for(vector<PlugAddress>::iterator plugIter = plugs.begin(); plugIter != plugs.end(); ++plugIter){
                if(plugIter->instrument == plug_query_args.instrument){
                    response.plugs.push_back(*plugIter);
                }
            }
        }
    }

    zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::ListPlugsAck>(ZstMessages::Kind::STAGE_LIST_PLUGS_ACK, response);
    zmsg_send(&ackmsg, socket);
}


void ZstStage::destroy_plug_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::DestroyPlug plug_destroy_args = ZstMessages::unpack_message_struct<ZstMessages::DestroyPlug>(msg);
	cout << "STAGE: Received destroy plug request" << endl;

	ZstPerformerRef &performer = get_performer_ref(plug_destroy_args.address.performer);

	performer.plugs.erase(std::remove(performer.plugs.begin(), performer.plugs.end(), plug_destroy_args.address), performer.plugs.end());
	
	zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::OKAck>(ZstMessages::Kind::OK, ZstMessages::OKAck());
	zmsg_send(&ackmsg, socket);
}


void ZstStage::connect_plugs_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::ConnectPlugs plug_args = ZstMessages::unpack_message_struct<ZstMessages::ConnectPlugs>(msg);
	cout << "STAGE: Received connect plug request" << endl;

	ZstPerformerRef &perfB = get_performer_ref(plug_args.second.performer);

	zmsg_t *ackmsg;
	PlugAddress input, output;

	if (plug_args.first.direction == PlugDirection::OUTPUT && plug_args.second.direction == PlugDirection::INPUT) {
		connect_plugs(get_performer_ref(plug_args.second.performer), get_performer_ref(plug_args.first.performer), plug_args.first);
	}
	else if (plug_args.first.direction == PlugDirection::INPUT && plug_args.second.direction == PlugDirection::OUTPUT){
		connect_plugs(get_performer_ref(plug_args.first.performer), get_performer_ref(plug_args.second.performer), plug_args.second);
	}
	else {
		ZstMessages::ErrorAck ack;
		ack.err = "Can't attach input->input or output->output";
		ackmsg = ZstMessages::build_message<ZstMessages::ErrorAck>(ZstMessages::Kind::ERR, ack);
		zmsg_send(&ackmsg, socket);
		return;
	}
	
	ackmsg = ZstMessages::build_message<ZstMessages::OKAck>(ZstMessages::Kind::OK, ZstMessages::OKAck());
	zmsg_send(&ackmsg, socket);
}


void ZstStage::connect_plugs(const ZstPerformerRef & input_performer, const ZstPerformerRef & output_performer, PlugAddress output_plug)
{
	//Need to get to the input performer and tell it to initiate a connection to the output performer
	//Will the performer need to return information back to the output?
	//The stage will dispatch a graph update, so will the client need to do this? Probably not

	ZstMessages::PerformerConnection perf_args;
    perf_args.endpoint = output_performer.endpoint;
	perf_args.output_plug = output_plug;

	zmsg_t *connectMsg = ZstMessages::build_message<ZstMessages::PerformerConnection>(ZstMessages::Kind::PERFORMER_REGISTER_CONNECTION, perf_args);
    zframe_t * empty = zframe_new_empty();
    zframe_t * identity = zframe_from(input_performer.name.c_str());
    zmsg_prepend(connectMsg, &empty);
    zmsg_prepend(connectMsg, &identity);
	zmsg_send(&connectMsg, m_performer_router);
}
