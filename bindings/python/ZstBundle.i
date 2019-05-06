%ignore ZstBundleIterator;
%ignore ZstCableBundleIterator;
%ignore ZstEntityBundleIterator;
%ignore ZstBundle::begin;
%ignore ZstBundle::end;

%include <ZstBundle.hpp>

%template(ZstEntityBundle) ZstBundle<ZstEntityBase*>;
%template(ZstCableBundle) ZstBundle<ZstCable*>;
