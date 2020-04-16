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

        # Python callback events
        #def connection_events(self):
        #    if not hasattr(self, "_connection_events"):
        #        import events
        #        self._connection_events = events.ConnectionEventHandler()
        #        self.add_connection_adaptor(self._connection_events)
        #    return self._connection_events
        %}

        GEN_ADAPTOR_WRAPPERS(connection_events, add_connection_adaptor, ZstConnectionAdaptor)
        GEN_ADAPTOR_WRAPPERS(hierarchy_events, add_hierarchy_adaptor, ZstHierarchyAdaptor)
        GEN_ADAPTOR_WRAPPERS(session_events, add_session_adaptor, ZstSessionAdaptor)
    }
}
