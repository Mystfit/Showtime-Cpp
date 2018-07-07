#include "ZstStageHierarchy.h"

ZstStageHierarchy::~ZstStageHierarchy()
{
}

void ZstStageHierarchy::destroy() {
	for (auto p : m_clients) {
		destroy_client(p.second);
	}
}

void ZstStageHierarchy::on_receive_msg(ZstStageMessage * msg)
{
	ZstStageMessage * response = NULL;
	ZstPerformerStageProxy * sender = m_session->hierarchy()->find_entity(msg->sender());

	//Check client hasn't finished joining yet
	if (!sender)
		return;

	switch (msg->kind()) {
	case ZstMsgKind::CLIENT_LEAVING:
	{
		response = destroy_client_handler(sender);
		break;
	}

	case ZstMsgKind::CLIENT_JOIN:
	{
		response = create_client_handler(sender_identity, msg);
		sender = get_client_from_socket_id(sender_identity);
		break;
	}
	case ZstMsgKind::CREATE_COMPONENT:
	{
		response = create_entity_handler<ZstComponent>(msg, sender);
		break;
	}
	case ZstMsgKind::CREATE_CONTAINER:
	{
		response = create_entity_handler<ZstContainer>(msg, sender);
		break;
	}
	case ZstMsgKind::CREATE_PLUG:
	{
		response = create_entity_handler<ZstPlug>(msg, sender);
		break;
	}

	case ZstMsgKind::CREATE_ENTITY_FROM_TEMPLATE:
	{
		response = create_entity_from_template_handler(msg);
		break;
	}
	case ZstMsgKind::DESTROY_ENTITY:
	{
		response = destroy_entity_handler(msg);
		break;
	}
	default:
		break;
	}

	if (response) {
		//Copy ID of the original message so we can match this message to a promise on the client
		//once upon a time there was a null pointer, it pointed to nothing.
		response->copy_id(msg);
		m_dispatch->send_to_address(response, sender);
	}
}

void ZstStageHierarchy::destroy_client(ZstPerformer * performer)
{
	//Nothing to do
	if (performer == NULL) {
		return;
	}

	ZstLog::net(LogLevel::notification, "Performer {} leaving", performer->URI().path());

	std::vector<ZstCable*> cables = get_cables_in_entity(performer);
	for (auto c : cables) {
		destroy_cable(c);
	}

	//Remove client and call all destructors in its hierarchy
	ZstClientMap::iterator client_it = m_clients.find(performer->URI());
	if (client_it != m_clients.end()) {
		m_clients.erase(client_it);
	}

	ZstStageMessage * leave_msg = msg_pool()->get_msg()->init_message(ZstMsgKind::DESTROY_ENTITY);
	leave_msg->append_str(performer->URI().path(), performer->URI().full_size());
	publish_stage_update(leave_msg);
	delete performer;
}

ZstStageMessage * ZstStageHierarchy::create_client_handler(std::string sender_identity, ZstStageMessage * msg)
{
	ZstStageMessage * response = msg_pool()->get_msg();

	//Copy the id of the message so the sender will eventually match the response to a message promise
	ZstPerformer client = msg->unpack_payload_serialisable<ZstPerformer>(0);

	ZstLog::net(LogLevel::notification, "Registering new client {}", client.URI().path());

	//Only one client with this UUID at a time
	if (get_client(client.URI())) {
		ZstLog::net(LogLevel::warn, "Client already exists ", client.URI().path());
		return response->init_message(ZstMsgKind::ERR_STAGE_PERFORMER_ALREADY_EXISTS);
	}

	std::string ip_address = std::string((char*)msg->payload_at(1).data(), msg->payload_at(1).size());

	//Copy streamable so we have a local ptr for the client
	ZstPerformerStageProxy * client_proxy = new ZstPerformerStageProxy(client, ip_address);
	assert(client_proxy);

	//Save our new client
	m_clients[client_proxy->URI()] = client_proxy;
	m_client_socket_index[std::string(sender_identity)] = client_proxy;

	//Update rest of network
	publish_stage_update(msg_pool()->get_msg()->init_entity_message(client_proxy));

	return response->init_message(ZstMsgKind::OK);
}

ZstStageMessage * ZstStageHierarchy::destroy_client_handler(ZstPerformer * performer)
{
	destroy_client(performer);
	return msg_pool()->get_msg()->init_message(ZstMsgKind::OK);
}

template ZstStageMessage * ZstStageHierarchy::create_entity_handler<ZstPlug>(ZstStageMessage * msg, ZstPerformer * performer);
template ZstStageMessage * ZstStageHierarchy::create_entity_handler<ZstComponent>(ZstStageMessage * msg, ZstPerformer * performer);
template ZstStageMessage * ZstStageHierarchy::create_entity_handler<ZstContainer>(ZstStageMessage * msg, ZstPerformer * performer);
template <typename T>
ZstStageMessage * ZstStageHierarchy::create_entity_handler(ZstStageMessage * msg, ZstPerformer * performer) {

	ZstStageMessage * response = msg_pool()->get_msg();

	if (!performer) {
		return response->init_message(ZstMsgKind::ERR_STAGE_PERFORMER_NOT_FOUND);
	}

	ZstLog::net(LogLevel::notification, "Found performer, unpacking entity");

	T entity = msg->unpack_payload_serialisable<T>(0);

	//Make sure this entity doesn't already exist
	if (performer->find_child_by_URI(entity.URI())) {
		//Entity already exists
		ZstLog::net(LogLevel::warn, "Entity already exists! {}", entity.URI().path());
		return response->init_message(ZstMsgKind::ERR_STAGE_ENTITY_ALREADY_EXISTS);
	}

	ZstLog::net(LogLevel::notification, "Entity {} doesn't exist yet, registering it now", entity.URI().path());

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
		ZstLog::net(LogLevel::warn, "Couldn't register entity {}. No parent found at {}", entity.URI().path(), parent_path.path());
		return response->init_message(ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND);
	}

	ZstLog::net(LogLevel::notification, "Found parent of entity. ");

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

	ZstLog::net(LogLevel::notification, "Registering new entity {}", entity_proxy->URI().path());

	//Update rest of network
	publish_stage_update(msg_pool()->get_msg()->init_entity_message(entity_proxy));

	return response->init_message(ZstMsgKind::OK);
}

ZstStageMessage * ZstStageHierarchy::destroy_entity_handler(ZstStageMessage * msg)
{
	ZstStageMessage * response = msg_pool()->get_msg();

	//Unpack entity to destroy from message
	ZstURI entity_path = ZstURI((char*)msg->payload_at(0).data(), msg->payload_at(0).size());

	ZstLog::net(LogLevel::notification, "Destroying entity {}", entity_path.path());

	//Find owner of entity
	ZstPerformer * owning_performer = get_client(entity_path.first());

	if (!owning_performer) {
		ZstLog::net(LogLevel::notification, "Could not find performer for destroyed entity {}", entity_path.path());
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
		ZstLog::net(LogLevel::notification, "Removing cables linked to leaving entity");
		destroy_cable(c);
	}

	//Update rest of network
	ZstStageMessage * stage_update_msg = msg_pool()->get_msg()->init_message(msg->kind());
	stage_update_msg->append_str(entity_path.path(), entity_path.full_size());
	publish_stage_update(stage_update_msg);

	delete entity;
	return response->init_message(ZstMsgKind::OK);
}


ZstStageMessage * ZstStageHierarchy::create_entity_template_handler(ZstStageMessage * msg)
{
	//TODO: Implement this
	throw std::logic_error("Creating entity types not implemented yet");
	return msg_pool()->get_msg()->init_message(ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND);
}


ZstStageMessage * ZstStageHierarchy::create_entity_from_template_handler(ZstStageMessage * msg)
{
	throw std::logic_error("Creating entity types not implemented yet");
	return msg_pool()->get_msg()->init_message(ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND);
}

