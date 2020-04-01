namespace showtime {
	%extend ShowtimeClient {
		%insert("python") %{
			def get_discovered_servers(self):
				bundle = ZstServerAddressBundle()
				return bundle_to_list(self.get_discovered_servers(bundle))

			def get_performers(self):
				bundle = ZstEntityBundle()
				return entity_bundle_to_list(self.get_performers(bundle))
		%}
	}
}
