#include "../ZstEventDispatcher.hpp"

ZstEventAdaptor::ZstEventAdaptor() : m_is_target_dispatcher_active(true)
{
};

ZstEventAdaptor::~ZstEventAdaptor() 
{
	std::lock_guard<std::recursive_mutex> lock(m_mtx);
	for (auto source : m_event_sources) {
		source->remove_adaptor(this);
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

void ZstEventAdaptor::add_event_source(ZstEventDispatcherBase* event_source)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtx);
	m_event_sources.push_back(event_source);
}

void ZstEventAdaptor::remove_event_source(ZstEventDispatcherBase* event_source)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtx);
	m_event_sources.erase(std::remove(m_event_sources.begin(), m_event_sources.end(), event_source), m_event_sources.end());
}
