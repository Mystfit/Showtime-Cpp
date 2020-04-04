namespace showtime {
	%rename(get_factory_bundle) ZstPerformer::get_factories;

	%extend ZstPerformer {
		%insert("python") %{
			def get_factories(self):
				bundle = ZstEntityFactoryBundle()
				self.get_factory_bundle(bundle)
				return entity_bundle_to_list(bundle)
		%}
	}

	%feature("nodirector") ZstPerformer::get_child_entities;
	%feature("nodirector") ZstPerformer::get_child_cables;
}