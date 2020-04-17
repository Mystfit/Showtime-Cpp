// Helper C func for casting to a component
%typemap(out) showtime::ZstComponent * showtime::ZstEntityBase::as_component {
    const std::string lookup_typename = "ZstComponent*";
    swig_type_info * const outtype = SWIG_TypeQuery(lookup_typename.c_str());
    $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), outtype, $owner);
}

// Global python code
%pythoncode "api_extensions.py"

// Swig helper macro for generating python adaptor wrappers
%define GEN_ADAPTOR_WRAPPERS(EVENT_NAME, ADAPTOR_REG_FUNC, CTYPE)
%pythoncode {
    def EVENT_NAME(self):
        adaptor_name = "_" + "CTYPE" + "_events"
        if not hasattr(self, adaptor_name):
            adaptor = generate_event_wrapper(CTYPE)()
            self.ADAPTOR_REG_FUNC(adaptor)
            setattr(self, adaptor_name, adaptor)
        return getattr(self, adaptor_name)
}
%enddef
