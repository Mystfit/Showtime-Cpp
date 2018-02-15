%include <ZstExports.h>
%include <ZstConstants.h>
%include "ZstLogging.i"

%include "ZstURI.i"
%include "ZstSynchronisable.i"

%ignore ZstSerialisable::read;
%ignore ZstSerialisable::write;
%include "ZstEntityBase.i"
%include "ZstPlug.i"
%include "ZstCable.i"

%include "ZstComponent.i"
%include "ZstContainer.i"
%include "ZstPerformer.i"
%include "ZstEvents.i"

%rename("%(strip:[zst_])s") "";
%include <Showtime.h>

//Entities
%include "AddFilter.i"
