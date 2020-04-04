//Bundle template declarations
//%{
//  #include <ZstCableAddress.h>
//  #include <ZstServerAddress.h>
//  #include <ZstBundle.hpp>
//  #include <ZstCable.h>
//  #include <entities/ZstEntityBase.h>
//%}

namespace showtime {
	%template(ZstEntityBundle) ZstBundle<showtime::ZstEntityBase*>;
	%template(ZstCableBundle) ZstBundle<showtime::ZstCable*>;
	%template(ZstURIBundle) ZstBundle<showtime::ZstURI>;
	%template(ZstEntityFactoryBundle) ZstBundle<showtime::ZstEntityFactory*>;
	%template(ZstServerAddressBundle) ZstBundle<showtime::ZstServerAddress>;
}
