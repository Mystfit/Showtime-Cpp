namespace showtime {
	%extend ZstPerformer {
		%insert("python") %{
			def get_factories(self):
				bundle = ZstEntityFactoryBundle()
				self.get_factories(bundle)
				return entity_bundle_to_list(bundle)
		%}
	}

	%feature("nodirector") ZstPerformer::get_child_entities;
	%feature("nodirector") ZstPerformer::get_child_cables;
}