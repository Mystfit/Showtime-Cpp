#include "ZstClientHierarchy.h"
#include <boost/assign.hpp>

ZstClientHierarchy::ZstClientHierarchy() :
	m_root(NULL)
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
	m_root->add_adaptor(static_cast<ZstSynchronisableAdaptor*>(this));
}

void ZstClientHierarchy::destroy()
{
	stage_events().remove_all_adaptors();

	//TODO: Delete other clientsa
	//Reset local performer
	size_t index = 0;

	for (auto entity : ZstEntityBundleScoped(m_root)) {
		destroy_entity_complete(entity);
	}

	//Process events to make sure events are dispatched properly
	process_events();
	delete m_root;

	ZstHierarchy::destroy();
}

void ZstClientHierarchy::process_events()
{
	ZstHierarchy::process_events();
	stage_events().process_events();
}

void ZstClientHierarchy::flush_events()
{
	ZstHierarchy::flush_events();
	stage_events().flush();
}

void ZstClientHierarchy::on_receive_msg(ZstMessage * msg)
{
	switch (msg->kind()) {
	case ZstMsgKind::CREATE_PLUG:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstPlug>());
		break;
	case ZstMsgKind::CREATE_PERFORMER:
		add_performer(msg->unpack_payload_serialisable<ZstPerformer>());
		break;
	case ZstMsgKind::CREATE_COMPONENT:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstComponent>());
		break;
	case ZstMsgKind::CREATE_CONTAINER:
		add_proxy_entity(msg->unpack_payload_serialisable<ZstContainer>());
		break;
	case ZstMsgKind::DESTROY_ENTITY:
		destroy_entity_complete(find_entity(ZstURI(msg->get_arg(ZstMsgArg::PATH))));
		break;
	default:
		break;
	}
}

void ZstClientHierarchy::publish_entity_update(ZstEntityBase * entity)
{
	performance_events().invoke([entity](ZstTransportAdaptor * adaptor) {
		ZstOutputPlug * plug = dynamic_cast<ZstOutputPlug*>(entity);
		if (plug) {
			ZstMsgArgs args = { { ZstMsgArg::PATH, plug->URI().path() } };
			if (!plug->is_reliable())
				args[ZstMsgArg::UNRELIABLE] = "";
			adaptor->send_message(ZstMsgKind::PERFORMANCE_MSG, args, *plug->raw_value());
		}
	});
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
	stage_events().invoke([this, entity, sendtype](ZstTransportAdaptor * adaptor)
	{
		adaptor->send_message(ZstMessage::entity_kind(*entity), sendtype, *entity, [this, entity](ZstMessageReceipt response) {
			if (response.status != ZstMsgKind::OK) {
				ZstLog::net(LogLevel::error, "Activate entity {} failed with status {}", entity->URI().path(), ZstMsgNames[response.status]);
				return;
			}
			this->activate_entity_complete(entity);
		});
	});

	if (sendtype == ZstTransportSendType::SYNC_REPLY)
		process_events();
}


void ZstClientHierarchy::destroy_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	ZstHierarchy::destroy_entity(entity, sendtype);

	//If the entity is local, let the stage know it's leaving
	if (!entity->is_proxy()) {
		stage_events().invoke([this, sendtype, entity](ZstTransportAdaptor * adaptor) {
			adaptor->send_message(ZstMsgKind::DESTROY_ENTITY, sendtype, {{ZstMsgArg::PATH, entity->URI().path()}}, [this, entity](ZstMessageReceipt response) {
				if (response.status != ZstMsgKind::OK) {
					ZstLog::net(LogLevel::error, "Destroy entity failed with status {}", ZstMsgNames[response.status]);
					return;
				}
				this->destroy_entity_complete(entity);
			});
		});
	}
	else {
		destroy_entity_complete(entity);
	}

	if (sendtype != ZstTransportSendType::ASYNC_REPLY) {
		process_events();
	}
}

void ZstClientHierarchy::destroy_entity_complete(ZstEntityBase * entity)
{
	if (!entity) {
		ZstLog::net(LogLevel::warn, "destroy_entity_complete(): Entity not found");
		return;
	}

	if (entity->URI() == this->get_local_performer()->URI()) {
		ZstLog::net(LogLevel::debug, "Destroyed entity is our own client, ignore.");
		return;
	}
	ZstHierarchy::destroy_entity_complete(entity);
}


ZstEntityBase * ZstClientHierarchy::find_entity(const ZstURI & path)
{
	if (m_root->URI() == path) {
		return m_root;
	}
	return ZstHierarchy::find_entity(path);
}

bool ZstClientHierarchy::path_is_local(const ZstURI & path) {
	return path.contains(m_root->URI());
}

ZstMsgKind ZstClientHierarchy::add_proxy_entity(const ZstEntityBase & entity) {

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

void ZstClientHierarchy::add_performer(const ZstPerformer & performer)
{
	if (performer.URI() == m_root->URI()) {
		//If we received ourselves as a performer, then we are now activated and can be added to the entity lookup map
		for (auto c : ZstEntityBundleScoped(m_root, true)) {
			add_entity_to_lookup(c);
		}
		ZstLog::net(LogLevel::debug, "Received self {} as performer. Caching in entity lookup", m_root->URI().path());
		return;
	}

	ZstHierarchy::add_performer(performer);
}
