namespace showtime {
	%rename(get_child_bundle) ZstEntityBase::get_child_entities;
	%rename(get_cable_bundle) ZstEntityBase::get_child_cables;

	%extend ZstEntityBase {
		%insert("python") %{
			def get_child_entities(self, include_parent=True, recursive=True):
				bundle = ZstEntityBundle()
				return entity_bundle_to_list(self.get_child_bundle(bundle, include_parent))

			def get_child_cables(self):
				bundle = ZstCableBundle()
				return bundle_to_list(self.get_child_cables(bundle, include_parent))
		%}
	}
}