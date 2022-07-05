namespace showtime {
	%ignore ZstSynchronisable::synchronisable_events;
	
	%extend ZstSynchronisable {
		GEN_ADAPTOR_WRAPPERS(synchronisable_events, add_synchronisable_adaptor, ZstSynchronisableAdaptor)
	}
}