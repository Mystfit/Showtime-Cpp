%ignore ZstBundleIterator;
%ignore ZstCableBundleIterator;
%ignore ZstEntityBundleIterator;
%ignore ZstBundle::begin;
%ignore ZstBundle::end;
%ignore ZstBundle::operator!=;
%ignore ZstBundle::operator++;
%ignore ZstBundle::operator[];

%include <ZstBundle.hpp>
%include "ZstBundleTemplates.i"
