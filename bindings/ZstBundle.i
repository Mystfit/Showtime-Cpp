namespace showtime {
	%ignore ZstBundleIterator;
	%ignore ZstBundleIterator::operator++;
	%ignore ZstBundleIterator::operator[];
	%ignore ZstCableBundleIterator;
	%ignore ZstEntityBundleIterator;
	%ignore ZstEntityBundleIterator::operator++;

	%ignore ZstBundle::begin;
	%ignore ZstBundle::end;
}

%include <showtime/ZstBundle.hpp>
%include "ZstBundleTemplates.i"
