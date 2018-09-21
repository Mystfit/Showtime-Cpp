#include "ZstBoostEventWakeup.hpp"

ZstBoostEventWakeup::ZstBoostEventWakeup()
{
	m_event_waker = std::make_shared<boost::fibers::condition_variable>();
}

void ZstBoostEventWakeup::wake()
{
	m_event_waker->notify_one();
}

void ZstBoostEventWakeup::wait()
{
	std::unique_lock< boost::fibers::mutex > lk(m_event_mtx);
	m_event_waker->wait(lk);
}
