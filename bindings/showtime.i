%include "std_string.i"
%ignore std::stringstream;
%ignore ZstLog::LoggerInfo;

%include "ZstLogging.i"
%include "ZstURI.i"

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

