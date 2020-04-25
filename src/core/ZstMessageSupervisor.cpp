#include "ZstMessageSupervisor.hpp"
#include "ZstLogging.h"
#include "ZstExceptions.h"
#include "ZstEventDispatcher.hpp"

using namespace moodycamel;

namespace showtime {

void ZstMessageSupervisor::destroy()
{
}

ZstMessageFuture ZstMessageSupervisor::register_response(ZstMsgID id)
{
	std::lock_guard<std::mutex> lock();
	m_response_promises.emplace(id, ZstMessagePromise());
	return m_response_promises[id].get_future();
}

void ZstMessageSupervisor::enqueue_resolved_promise(ZstMsgID id)
{
	std::lock_guard<std::mutex> lock();
	m_dead_promises.enqueue(id);
}

void ZstMessageSupervisor::cleanup_response_messages()
{
	ZstMsgID id;
	while (m_dead_promises.try_dequeue(id)) {
		remove_response_promise(id);
	}
}

void ZstMessageSupervisor::take_message_ownership(std::shared_ptr<ZstMessage>& msg, ZstEventCallback cleanup_func)
{
	std::lock_guard<std::mutex> lock();
	m_owned_messages[msg->id()] = { msg, cleanup_func };
}

void ZstMessageSupervisor::release_owned_message(std::shared_ptr<ZstMessage>& msg)
{
	auto msg_it = m_owned_messages.find(msg->id());
	if (msg_it != m_owned_messages.end()) {
		// Call the cleanup callback now that the response has been processed fully
		msg_it->second.second(ZstEventStatus::SUCCESS);
		m_owned_messages.erase(msg_it);
	}
}

void ZstMessageSupervisor::remove_response_promise(ZstMsgID id)
{
	//Clear completed promise when finished
	auto promise = m_response_promises.find(id);
	if (promise != m_response_promises.end()) {
		std::lock_guard<std::mutex> lock();
		m_response_promises.erase(id);
	}
}

std::promise< std::shared_ptr<ZstMessage> >& ZstMessageSupervisor::get_response_promise(std::shared_ptr<ZstMessage> message)
{
	auto promise = m_response_promises.find(message->id());
	if (promise != m_response_promises.end()) {
		//message->set_has_promise();
		//promise->second.set_value(message);
		return promise->second;
	}
	throw std::out_of_range("Could not find promise for message ID");
}

}
