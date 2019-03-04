#pragma once

#include "adaptors/ZstHierarchyAdaptor.hpp"
#include "adaptors/ZstSessionAdaptor.hpp"

class ZstModuleAdaptor : public ZstHierarchyAdaptor
{
public:
	//Pull base event adaptor functions forwards from common base class
	using ZstHierarchyAdaptor::is_target_dispatcher_active;
	using ZstHierarchyAdaptor::set_target_dispatcher_inactive;
};
