namespace showtime {
	%ignore ZstBundleIterator;
	%ignore ZstCableBundleIterator;
	%ignore ZstEntityBundleIterator;
	%ignore ZstBundle::begin;
	%ignore ZstBundle::end;
}

%include <ZstBundle.hpp>

using namespace showtime;
%template(ZstEntityBundle) ZstBundle<ZstEntityBase*>;
%template(ZstCableBundle) ZstBundle<ZstCable*>;
