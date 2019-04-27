#include "ZstMessageSupervisor.hpp"
#include "ZstLogging.h"

ZstMessageSupervisor::ZstMessageSupervisor(std::shared_ptr<cf::time_watcher> timeout_watcher, long timeout_duration) :
	m_timeout_watcher(timeout_watcher),
	m_timeout_duration(timeout_duration)
{
}

ZstMessageSupervisor::~ZstMessageSupervisor()
{
}

ZstMessageFuture ZstMessageSupervisor::register_response(ZstMsgID id)
{
	m_response_promises.emplace(id, promise<ZstMsgKind>());
	future<ZstMsgKind> future = m_response_promises[id].get_future();
	future = future.timeout(std::chrono::milliseconds(m_timeout_duration), ZstTimeoutException("Connect timeout"), *m_timeout_watcher);
	return future;
}

void ZstMessageSupervisor::enqueue_resolved_promise(ZstMsgID id)
{
	m_dead_promises.enqueue(id);
}

void ZstMessageSupervisor::cleanup_response_messages()
{
	ZstMsgID id;
	while (m_dead_promises.try_dequeue(id)) {
		remove_response_promise(id);
	}
}

void ZstMessageSupervisor::remove_response_promise(ZstMsgID id)
{
	//Clear completed promise when finished
	try {
		m_response_promises.erase(id);
	}
	catch (std::out_of_range e) {
		ZstLog::net(LogLevel::debug, "Promise already removed. {}", e.what());
	}
}

void ZstMessageSupervisor::process_response(ZstMsgID id, ZstMsgKind response)
{
	auto promise = m_response_promises.find(id);
	if (promise != m_response_promises.end()) {
		try {
			//ZstLog::net(LogLevel::warn, "Resolving promise {}", id);
			promise->second.set_value(response);
		}
		catch (cf::future_error e) {
			//ZstLog::net(LogLevel::warn, "Promise error {}", e.what());
		}
		enqueue_resolved_promise(id);
	}
}
