%include "ZstAdaptorPointers.i"

namespace showtime {
	%feature("director") ZstSynchronisable;
	%ignore ZstSynchronisable::enqueue_activation;
	%ignore ZstSynchronisable::enqueue_deactivation;
	%ignore ZstSynchronisable::set_activating;
	%ignore ZstSynchronisable::set_activated;
	%ignore ZstSynchronisable::set_deactivating;
	%ignore ZstSynchronisable::set_deactivated;
	%ignore ZstSynchronisable::set_activation_status;
	%ignore ZstSynchronisable::set_error;
	%ignore ZstSynchronisable::process_events;
	%ignore ZstSynchronisable::dispatch_destroyed;
	%ignore ZstSynchronisable::set_proxy;
}

%feature("nodirector") showtime::ZstSynchronisable::add_adaptor;
%feature("nodirector") showtime::ZstSynchronisable::remove_adaptor;
%rename(add_synchronisable_adaptor) showtime::ZstSynchronisable::add_adaptor(std::shared_ptr<ZstSynchronisableAdaptor>);
%rename(remove_synchronisable_adaptor) showtime::ZstSynchronisable::remove_adaptor(std::shared_ptr<ZstSynchronisableAdaptor>);

%include <ZstSynchronisable.h>
