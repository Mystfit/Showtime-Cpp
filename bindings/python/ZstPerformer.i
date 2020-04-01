namespace showtime {
	%extend ZstPerformer {
		%insert("python") %{
			def get_factories(self):
				bundle = ZstEntityFactoryBundle()
				return entity_bundle_to_list(self.get_factories(bundle))
		%}
	}

	%feature("nodirector") ZstPerformer::get_child_entities;
	%feature("nodirector") ZstPerformer::get_child_cables;
}