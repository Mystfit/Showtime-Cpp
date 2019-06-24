// %feature("director") ZstSynchronisable;

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

%include <ZstSynchronisable.h>
