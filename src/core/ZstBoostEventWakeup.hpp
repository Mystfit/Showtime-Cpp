#pragma once
#include <memory>
#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <ZstExports.h>
#include "ZstEventWakeup.hpp"

class ZstBoostEventWakeup : public ZstEventWakeup {
public:
	ZST_EXPORT ZstBoostEventWakeup();
    ZST_EXPORT virtual ~ZstBoostEventWakeup();
	ZST_EXPORT virtual void wake() override;
	ZST_EXPORT virtual void wait() override;

private:
	std::shared_ptr<boost::fibers::condition_variable> m_event_waker;
	boost::fibers::mutex m_event_mtx;
};
