#include "ZstStage.h"

using namespace std;

ZstStage::ZstStage()
{
	m_section_router = zsock_new_router("@tcp://*:6000");
	zsock_set_linger(m_section_router, 0);

    m_graph_update_pub = zsock_new_pub("@tcp://*:6001");
	zsock_set_linger(m_graph_update_pub, 0);
	
	//Need to start the stage actor before we attach a listener

	attach_pipe_listener(m_section_router, s_handle_router, this);
	start();
}

ZstStage::~ZstStage()
{
	ZstActor::~ZstActor();

	//Close performer pipes
	for (map<std::string, ZstPerformerRef>::iterator perfIter = m_performer_refs.begin(); perfIter != m_performer_refs.end(); ++perfIter) {
		zsock_destroy(&perfIter->second.pipe);
	}

	//Close stage pipes
	zsock_destroy(&m_section_router);
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
    
    //Get identity
    zframe_t *identity = zmsg_pop(msg);
    
    //Remove spacer
    zmsg_pop(msg);
    
    //Get message type
    ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
    
    switch (message_type) {
        case ZstMessages::Kind::STAGE_REGISTER_PERFORMER:
            stage->register_section_handler(socket, identity, msg);
            break;
        case ZstMessages::Kind::PERFORMER_HEARTBEAT:
            stage->section_heartbeat_handler(socket, identity, msg);
            break;
        case ZstMessages::Kind::STAGE_REGISTER_PLUG:
            stage->register_plug_handler(socket, identity, msg);
            break;
        case ZstMessages::Kind::STAGE_LIST_PLUGS:
            stage->list_plugs_handler(socket, identity, msg);
            break;
		case ZstMessages::Kind::STAGE_DESTROY_PLUG:
			stage->destroy_plug_handler(socket, identity, msg);
			break;
        default:
            cout << "Didn't understand received message type of " << message_type << endl;
            break;
    }

	return 0;
}


int ZstStage::s_handle_section_pipe(zloop_t * loop, zsock_t * socket, void * arg)
{
    ZstStage *stage = (ZstStage*)arg;
    
    //Receive waiting message
    zmsg_t *msg = zmsg_recv(socket);
    
    //Get message type
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
    
    cout << "STAGE: Message from section pipe" << endl;
    
    return 0;
}


void ZstStage::register_section_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg){
	ZstMessages::RegisterPerformer section_args = ZstMessages::unpack_message_struct<ZstMessages::RegisterPerformer>(msg);
    
    if(m_performer_refs.count(section_args.name) > 0){
        cout << "STAGE: Section already registered!" << endl;
        return;
    }
    cout << "STAGE: Registering new section " << section_args.name << endl;

    //Create new pair socket to handle messages to/from to section
    ZstPerformerRef sectionRef;
    sectionRef.name = section_args.name;
    sectionRef.pipe = zsock_new_pair("@tcp://127.0.0.1:*");
    
    m_performer_refs[section_args.name] = sectionRef;
    char *last_endpoint = zsock_last_endpoint(sectionRef.pipe);
    
    //Register section pipe to poller
	attach_pipe_listener(sectionRef.pipe, s_handle_section_pipe, this);

    //Find endpoint port
	ZstMessages::RegisterPerformerAck ack_args;
    std::cmatch matches;
    regex endpoint_expr("(?:tcp?)(?::\/\/)([^:]*):([0-9]{5})?(.*)");
    std::regex_match ( last_endpoint, matches, endpoint_expr, std::regex_constants::match_default );
    ack_args.assigned_stage_port = std::atoi(matches.str(2).c_str());
    cout << "STAGE: New section port: " << ack_args.assigned_stage_port << endl;
    
    zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::RegisterPerformerAck>(ZstMessages::Kind::STAGE_REGISTER_PERFORMER_ACK, ack_args ,identity);
    zmsg_send(&ackmsg, socket);
    
    //Test new section pipe with a handshake
    zmsg_send(&ackmsg, sectionRef.pipe);
}

void ZstStage::register_plug_handler(zsock_t *socket, zframe_t *identity, zmsg_t *msg){
	ZstMessages::RegisterPlug plug_args = ZstMessages::unpack_message_struct<ZstMessages::RegisterPlug>(msg);
    
    if(m_performer_refs.empty()){
       cout << "STAGE: Couldn't register plug. No section registered to stage with name";
    }
    
    cout << "STAGE: Registering new plug " << plug_args.name << endl;
    ZstPlug::Address plugRef;
    plugRef.name = plug_args.name;
    plugRef.instrument = plug_args.instrument;
    plugRef.performer = plug_args.performer;
    
    m_performer_refs[plug_args.performer].plugs.push_back(plugRef);
    
    zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::OKAck>(ZstMessages::Kind::OK, ZstMessages::OKAck() ,identity);
    zmsg_send(&ackmsg, socket);
}

void ZstStage::section_heartbeat_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg){
	ZstMessages::Heartbeat heartbeat_args = ZstMessages::unpack_message_struct<ZstMessages::Heartbeat>(msg);
    cout << "STAGE: Received heartbeat from " << heartbeat_args.from << ". Timestamp: " << heartbeat_args.timestamp << endl;
    
    zmsg_t *ackmsg = ZstMessages::build_message(ZstMessages::Kind::OK, NULL ,identity);
    zmsg_send(&ackmsg, socket);
}

void ZstStage::list_plugs_handler(zsock_t *socket, zframe_t *identity, zmsg_t *msg){
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
        vector<ZstPlug::Address> plugs = m_performer_refs[plug_query_args.performer].plugs;
        if(plug_query_args.instrument.empty()){
            //Return all plugs from section
            response.plugs = m_performer_refs[plug_query_args.performer].plugs;
        } else {
            //Return only named instrument plugs from section
            for(vector<ZstPlug::Address>::iterator plugIter = plugs.begin(); plugIter != plugs.end(); ++plugIter){
                if(plugIter->instrument == plug_query_args.instrument){
                    response.plugs.push_back(*plugIter);
                }
            }
        }
    }

    zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::ListPlugsAck>(ZstMessages::Kind::STAGE_LIST_PLUGS_ACK, response, identity);
    zmsg_send(&ackmsg, socket);
}

void ZstStage::destroy_plug_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg)
{
	ZstMessages::DestroyPlug plug_destroy_args = ZstMessages::unpack_message_struct<ZstMessages::DestroyPlug>(msg);
	cout << "STAGE: Received destroy plug request" << endl;

	ZstPerformerRef &performer = get_performer_ref(plug_destroy_args.address.performer);

	performer.plugs.erase(std::remove(performer.plugs.begin(), performer.plugs.end(), plug_destroy_args.address), performer.plugs.end());
	
	zmsg_t *ackmsg = ZstMessages::build_message<ZstMessages::OKAck>(ZstMessages::Kind::OK, ZstMessages::OKAck(), identity);
	zmsg_send(&ackmsg, socket);
}
