namespace showtime {
	%rename(get_creatable_bundle) ZstEntityFactory::get_creatables;

	%extend ZstEntityFactory {
		%insert("python") %{
			def get_creatables(self):
				bundle = ZstURIBundle()
				self.get_creatable_bundle(bundle)
				return bundle_to_list(bundle)
		%}

	GEN_ADAPTOR_WRAPPERS(factory_events, add_factory_adaptor, ZstFactoryAdaptor)
	
	}
}