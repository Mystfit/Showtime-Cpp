namespace showtime {
	%extend ZstSynchronisable {
		GEN_ADAPTOR_WRAPPERS(synchronisable_events, add_synchronisable_adaptor, ZstSynchronisableAdaptor)
	}
}