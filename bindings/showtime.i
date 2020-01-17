%include "std_string.i"

%ignore std::stringstream;
%ignore std::enable_shared_from_this;
%ignore std::enable_shared_from_this<ShowtimeClient>;
%ignore std::enable_shared_from_this<ShowtimeServer>;
%ignore inheritable_enable_shared_from_this;

%include "ZstSerialisable.i"
%include "ZstLogging.i"
%include "ZstURI.i"

%include "ZstSynchronisable.i"
%include "ZstEntityBase.i"
%include "ZstPlug.i"
%include "ZstCableAddress.i"
%include "ZstCable.i"

%include "ZstAdaptorPointers.i"
%include "ZstComponent.i"
%include "ZstPerformer.i"
%include "ZstEntityFactory.i"
%include "ZstServerAddress.i"
%include "ZstAdaptors.i"

%rename("%(strip:[zst_])s") "";

%include <Showtime.h>
%include <ShowtimeServer.h>

