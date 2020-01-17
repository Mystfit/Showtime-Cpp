namespace showtime {
	%template(ZstEntityBundle) ZstBundle<ZstEntityBase*>;
	%template(ZstCableBundle) ZstBundle<ZstCable*>;
	%template(ZstURIBundle) ZstBundle<ZstURI>;
	%template(ZstEntityFactoryBundle) ZstBundle<ZstEntityFactory*>;
	%template(ZstServerAddressBundle) ZstBundle<ZstServerAddress>;
}
