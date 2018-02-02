%feature("director") ZstContainer;
%ignore ZstContainer::read;
%ignore ZstContainer::write;
%ignore ZstContainer::set_parent;
%nodefaultctor;

%include <entities/ZstContainer.h>
%clearnodefaultctor;
