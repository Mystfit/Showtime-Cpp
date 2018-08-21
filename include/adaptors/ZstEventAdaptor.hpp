#pragma once
#include <ZstLogging.h>

class ZstEventAdaptor {
public:
	ZstEventAdaptor() : m_is_target_dispatcher_active(true) 
	{
	};

	bool is_target_dispatcher_active() 
	{ 
		return m_is_target_dispatcher_active; 
	}

	void set_target_dispatcher_inactive() 
	{ 
		m_is_target_dispatcher_active = false; 
	};

private:
	bool m_is_target_dispatcher_active;
};
