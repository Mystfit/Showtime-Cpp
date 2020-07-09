#include "../ZstEventDispatcher.hpp"
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {


//ZstEventAdaptor::ZstEventAdaptor()
//{
//};
//
//ZstEventAdaptor::~ZstEventAdaptor() 
//{
//	//auto sources = m_event_sources;
//	for (auto source : m_event_sources) {
//		if (auto src = source.lock())
//			src->prune_missing_adaptors();
//	}
//	//m_event_sources.clear();
//};
//
//void ZstEventAdaptor::add_event_source(std::weak_ptr<ZstEventDispatcherBase> event_source)
//{
//	std::lock_guard<std::recursive_mutex> lock(m_mtx);
//	m_event_sources.insert(event_source);
//}
//
//void ZstEventAdaptor::remove_event_source(std::weak_ptr<ZstEventDispatcherBase> event_source)
//{
//	std::lock_guard<std::recursive_mutex> lock(m_mtx);
//	m_event_sources.erase(event_source);
//}
//
//bool ZstEventAdaptor::contains_event_source(std::weak_ptr<ZstEventDispatcherBase> event_source)
//{
//	std::lock_guard<std::recursive_mutex> lock(m_mtx);
//	return (m_event_sources.find(event_source) != m_event_sources.end()) ? true : false;
//}
//
//void ZstEventAdaptor::prune_dispatchers()
//{
//	std::lock_guard<std::recursive_mutex> lock(m_mtx);
//	auto sources = m_event_sources;
//	for (auto src : sources) {
//		if (src.expired())
//			m_event_sources.erase(src);
//	}
//}

}
