#include <boost/uuid/uuid_io.hpp>
#include <showtime/ZstLogging.h>
#include <showtime/ZstExceptions.h>

#include "ZstMessageSupervisor.hpp"
#include "ZstEventDispatcher.hpp"

using namespace moodycamel;

namespace showtime {

void ZstMessageSupervisor::destroy()
{
}

ZstMessageFuture ZstMessageSupervisor::register_response(ZstMsgID id)
{
	std::lock_guard<std::mutex> lock(m_mtx);
	m_response_promises.emplace(id, ZstMessagePromise());
	return m_response_promises[id].get_future();
}

void ZstMessageSupervisor::enqueue_resolved_promise(ZstMsgID id)
{
	std::lock_guard<std::mutex> lock(m_mtx);
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
	std::lock_guard<std::mutex> lock(m_mtx);
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
		std::lock_guard<std::mutex> lock(m_mtx);
		m_response_promises.erase(id);
	}
}

void ZstMessageSupervisor::process_message_response(std::shared_ptr<ZstMessage>& message, ZstEventCallback on_complete)
{
	if (!m_response_promises.size()) {
		throw std::out_of_range("No promises available");
		return;
	}
	auto promise = m_response_promises.find(message->id());
	if (promise != m_response_promises.end()) {
		// Flag message as a response message so it won't get cleaned up until the response has finished processing
		message->set_has_promise();
		this->take_message_ownership(message, on_complete);
		try {
			promise->second.set_value(message);
		}
		catch (std::future_error e) {
			Log::net(Log::Level::error, "Promise error! {}", e.what());
		}
		return;
	}
	throw std::out_of_range("Could not find promise for message ID");
}

}
