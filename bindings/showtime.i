%include "std_string.i"

%ignore std::stringstream;
%ignore std::enable_shared_from_this;
%ignore std::enable_shared_from_this<ShowtimeClient>;
%ignore std::enable_shared_from_this<ShowtimeServer>;
%ignore inheritable_enable_shared_from_this;

%include "flatbuffers_types.i"
%include "ZstSerialisable.i"
%include "ZstServerAddress.i"
%include "ZstURI.i"
%include "ZstCableAddress.i"

%include "ZstBundle.i"
%include "ZstBundleTemplates.i"

%include "ZstAdaptorPointers.i"
%include "ZstAdaptors.i"

%include "ZstLogging.i"

%include "ZstSynchronisable.i"
%include "ZstEntityBase.i"
%include "ZstPlug.i"
%include "ZstCable.i"

%include "ZstComponent.i"
%include "ZstPerformer.i"
%include "ZstEntityFactory.i"

%rename("%(strip:[zst_])s") "";

template <typename T>
class std::enable_shared_from_this{};
%template(ShowtimeClientShared) std::enable_shared_from_this<showtime::ShowtimeClient>;
%template(ShowtimeServerShared) std::enable_shared_from_this<showtime::ShowtimeServer>;

%ignore std::enable_shared_from_this<ShowtimeClient>;
%ignore std::enable_shared_from_this<ShowtimeServer>;

%include <showtime/ShowtimeClient.h>
%include <showtime/ShowtimeServer.h>
