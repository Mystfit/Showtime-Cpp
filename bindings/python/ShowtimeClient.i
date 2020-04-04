namespace showtime {
	%rename(get_discovered_server_bundle) ShowtimeClient::get_discovered_servers;
	%rename(get_performer_bundle) ShowtimeClient::get_performers;

	%extend ShowtimeClient {
		%insert("python") %{
			def get_discovered_servers(self):
				bundle = ZstServerAddressBundle()
				self.get_discovered_server_bundle(bundle)
				return bundle_to_list(bundle)

			def get_performers(self):
				bundle = ZstEntityBundle()
				self.get_performer_bundle(bundle)
				return entity_bundle_to_list(bundle)
		%}
	}
}
