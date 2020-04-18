namespace showtime {
    %rename(get_child_bundle) ZstEntityBase::get_child_entities;
    %rename(get_cable_bundle) ZstEntityBase::get_child_cables;

    %extend ZstEntityBase {
        %insert("python") %{
            def get_child_entities(self, include_parent=False, recursive=False):
                bundle = ZstEntityBundle()
                self.get_child_bundle(bundle, include_parent, recursive)
                return entity_bundle_to_list(bundle)

            def get_child_cables(self):
                bundle = ZstCableBundle()
                self.get_child_cables(bundle)
                return bundle_to_list(bundle)
        %}

        GEN_ADAPTOR_WRAPPERS(entity_events, add_entity_adaptor, ZstEntityAdaptor)
    }
}