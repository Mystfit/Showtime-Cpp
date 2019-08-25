#include "../ZstEventDispatcher.hpp"
#include "adaptors/ZstEventAdaptor.hpp"

ZstEventAdaptor::ZstEventAdaptor() : m_is_target_dispatcher_active(true)
{
};

ZstEventAdaptor::~ZstEventAdaptor() 
{
	auto sources = m_event_sources;
	for (auto source : sources) {
		source->remove_adaptor(shared_from_this());
	}
	m_event_sources.clear();
};

bool ZstEventAdaptor::is_target_dispatcher_active()
{
	return m_is_target_dispatcher_active;
}

void ZstEventAdaptor::set_target_dispatcher_inactive()
{
	std::lock_guard<std::recursive_mutex> lock(m_mtx);
	m_is_target_dispatcher_active = false;
};

void ZstEventAdaptor::add_event_source(std::shared_ptr<ZstEventDispatcherBase> event_source)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtx);
	m_event_sources.insert(event_source);
}

void ZstEventAdaptor::remove_event_source(std::shared_ptr<ZstEventDispatcherBase> event_source)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtx);
	m_event_sources.erase(event_source);
}
