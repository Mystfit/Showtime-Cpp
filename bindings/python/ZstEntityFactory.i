namespace showtime {
	%extend ZstEntityFactory {
		%insert("python") %{
			def get_creatables(self):
				bundle = ZstURIBundle()
				self.get_creatables(bundle)
				return bundle_to_list(bundle)
		%}
	}
}