namespace showtime {
	%template(ZstEntityBundle) ZstBundle<showtime::ZstEntityBase*>;
	%template(ZstCableBundle) ZstBundle<showtime::ZstCable*>;
	%template(ZstURIBundle) ZstBundle<showtime::ZstURI>;
	%template(ZstEntityFactoryBundle) ZstBundle<showtime::ZstEntityFactory*>;
	%template(ZstServerAddressBundle) ZstBundle<showtime::ZstServerAddress>;
}
