%include "std_string.i"
%include "std_shared_ptr.i"

%ignore std::stringstream;
%ignore ZstLog::LoggerInfo;
%ignore std::enable_shared_from_this;
%ignore inheritable_enable_shared_from_this;

%include "ZstLogging.i"
%include "ZstURI.i"
%include "ZstAdaptorPointers.i"

%include "ZstSynchronisable.i"
%include "ZstSerialisable.i"
%include "ZstEntityBase.i"
%include "ZstPlug.i"
%include "ZstCable.i"

%include "ZstAdaptors.i"
%include "ZstComponent.i"
%include "ZstPerformer.i"
%include "ZstEntityFactory.i"

%include "ZstServerAddress.i"
%include "ZstCableAddress.i"

%rename("%(strip:[zst_])s") "";

%include <Showtime.h>
%include <ShowtimeServer.h>

