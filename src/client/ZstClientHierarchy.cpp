#include "ZstClientHierarchy.h"
#include <boost/assign.hpp>

ZstClientHierarchy::ZstClientHierarchy() :
	m_root(NULL),
	m_stage_events("hierarchy syncronisables with events")
{
}

ZstClientHierarchy::~ZstClientHierarchy()
{
}

void ZstClientHierarchy::init(std::string name)
{
	ZstHierarchy::init();

	//Create a root entity to hold our local entity hierarchy
	//Sets the name of our performer and the address of our graph output
	m_root = new ZstPerformer(name.c_str());
    ZstSynchronisable::add_adaptor(m_root, this);
}

void ZstClientHierarchy::destroy()
{
	ZstHierarchy::destroy();
	m_stage_events.remove_all_adaptors();

	//TODO: Delete other clients
	delete m_root;
}

void ZstClientHierarchy::process_events()
{
	ZstHierarchy::process_events();
	m_stage_events.process_events();
}

void ZstClientHierarchy::flush_events()
{
	ZstHierarchy::flush_events();
	m_stage_events.flush();
}

void ZstClientHierarchy::on_receive_msg(ZstMessage * msg)
{
	switch (msg->kind()) {
	case ZstMsgKind::CREATE_PLUG:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstPlug>(0));
		break;
	case ZstMsgKind::CREATE_PERFORMER:
		add_performer(msg->unpack_payload_serialisable<ZstPerformer>(0));
		break;
	case ZstMsgKind::CREATE_COMPONENT:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstComponent>(0));
		break;
	case ZstMsgKind::CREATE_CONTAINER:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstContainer>(0));
		break;
	case ZstMsgKind::DESTROY_ENTITY:
		destroy_entity_complete(find_entity(ZstURI(msg->get_arg(ZstMsgArg::PATH))));
		break;
	default:
		break;
	}
}

void ZstClientHierarchy::activate_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	//If the entity doesn't have a parent, put it under the root container
	if (!entity->parent()) {
		ZstLog::net(LogLevel::debug, "No parent set for {}, adding to {}", entity->URI().path(), m_root->URI().path());
		m_root->add_child(entity);
	}
	 
	ZstHierarchy::activate_entity(entity, sendtype);
	
	//Build message
	m_stage_events.invoke([this, entity, sendtype](ZstTransportAdaptor * adaptor)
	{
		adaptor->send_message(ZstMessage::entity_kind(*entity), sendtype, *entity, [this, entity](ZstMessageReceipt response) {
			this->activate_entity_complete(response, entity);
		});
	});

	if (sendtype == ZstTransportSendType::SYNC_REPLY)
		process_events();
}


void ZstClientHierarchy::activate_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
{
	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::error, "Activate entity {} failed with status {}", entity->URI().path(), ZstMsgNames[response.status]);
		return;
	}

	switch (response.status) {
	case ZstMsgKind::OK:
		synchronisable_enqueue_activation(entity);
		break;
	case ZstMsgKind::ERR_ENTITY_ALREADY_EXISTS:
		synchronisable_set_error(entity, ZstSyncError::ENTITY_ALREADY_EXISTS);
		break;
	case ZstMsgKind::ERR_ENTITY_NOT_FOUND:
		synchronisable_set_error(entity, ZstSyncError::PERFORMER_NOT_FOUND);
		break;
	default:
		break;
	}

	ZstLog::net(LogLevel::debug, "Activate entity {} complete with status {}", entity->URI().path(), ZstMsgNames[response.status]);
}


void ZstClientHierarchy::destroy_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	ZstHierarchy::destroy_entity(entity, sendtype);

	//If the entity is local, let the stage know it's leaving
	if (!entity->is_proxy()) {
		m_stage_events.invoke([this, sendtype, entity](ZstTransportAdaptor * adaptor) {
			adaptor->send_message(ZstMsgKind::DESTROY_ENTITY, sendtype, {{ZstMsgArg::PATH, entity->URI().path()}}, [this, entity](ZstMessageReceipt response) {
				if (response.status != ZstMsgKind::OK) {
					ZstLog::net(LogLevel::error, "Destroy entity failed with status {}", ZstMsgNames[response.status]);
					return;
				}
				this->destroy_entity_complete(entity);
                ZstSynchronisable::remove_adaptor(entity, this);
			});
		});
	}
	else {
		destroy_entity_complete(entity);
	}

	if (sendtype != ZstTransportSendType::ASYNC_REPLY) {
		process_events();
        ZstSynchronisable::remove_adaptor(entity, this);
	}
}

void ZstClientHierarchy::destroy_entity_complete(ZstEntityBase * entity)
{
	//Pre-emptively disconnect all cables inside the entity
	ZstCableBundle * bundle = entity->acquire_cable_bundle();
	bundle->disconnect_all();
	entity->release_cable_bundle(bundle);

	ZstHierarchy::destroy_entity_complete(entity);
}


ZstEntityBase * ZstClientHierarchy::find_entity(const ZstURI & path)
{
	if (path_is_local(path)) {
		//Path points to local performer
		if (m_root->URI() == path) {
			return m_root;
		}

		//Path points to child in local performer
		return m_root->find_child_by_URI(path);
	}
	else {
		//Path is somewhere else in the hierarchy
		//TODO: Should the local performer be also placed in the remote performer list too?
		return ZstHierarchy::find_entity(path);
	}

	return NULL;
}

bool ZstClientHierarchy::path_is_local(const ZstURI & path) {
	return path.contains(m_root->URI());
}

ZstMsgKind ZstClientHierarchy::add_proxy_entity(ZstEntityBase & entity) {

	// Don't need to activate local entities, they will auto-activate when the stage responds with an OK
	// Also, we can't rely on the proxy flag here as it won't have been set yet
	if (path_is_local(entity.URI())) {
		ZstLog::net(LogLevel::debug, "Received local entity {}. Ignoring", entity.URI().path());
		return ZstMsgKind::EMPTY;
	}
	return ZstHierarchy::add_proxy_entity(entity);
}

ZstPerformer * ZstClientHierarchy::get_local_performer() const
{
	return m_root;
}

ZstEventDispatcher<ZstTransportAdaptor*> & ZstClientHierarchy::stage_events()
{
	return m_stage_events;
}

void ZstClientHierarchy::add_performer(ZstPerformer & performer)
{
	if (performer.URI() == m_root->URI()) {
		ZstLog::net(LogLevel::debug, "Received self {} as performer. Ignoring", m_root->URI().path());
		return;
	}

	ZstHierarchy::add_performer(performer);
}
