#pragma once

#include <vector>
#include <ZstExports.h>
#include <ZstEvents.h>

class ZstINetworkInteractor;

class ZstSynchronisable {
public:
	ZST_EXPORT ZstSynchronisable();
	ZST_EXPORT ZstSynchronisable(const ZstSynchronisable & other);
	ZST_EXPORT virtual void attach_activation_event(ZstSynchronisableEvent * event);
	ZST_EXPORT virtual void attach_deactivation_event(ZstSynchronisableEvent * event);
	ZST_EXPORT virtual void detach_activation_event(ZstSynchronisableEvent * event);
	ZST_EXPORT virtual void detach_deactivation_event(ZstSynchronisableEvent * event);

	ZST_EXPORT virtual void process_events();
	ZST_EXPORT virtual void on_activated() = 0;
	ZST_EXPORT virtual void on_deactivated() = 0;
	ZST_EXPORT virtual bool is_activated();

	//Register graph sender so this entity can comunicate with the graph
	ZST_EXPORT virtual void set_network_interactor(ZstINetworkInteractor * network_interactor);

protected:
	ZST_EXPORT virtual void set_activated();
	ZST_EXPORT virtual void set_deactivated();

private:
	std::vector<ZstSynchronisableEvent*> m_activation_events;
	std::vector<ZstSynchronisableEvent*> m_deactivation_events;

	bool m_activation_queued;
	bool m_deactivation_queued;
	bool m_is_activated;

	ZstINetworkInteractor * m_network_interactor;
};