namespace showtime {
	%rename(equal) ZstCable::operator==;
	%ignore ZstSerialisable;
	%ignore ZstCable::operator!=;
	%ignore ZstCable::write;
	%ignore ZstCable::read;
	%ignore ZstCableEq;
	%ignore ZstCableHash;
	%ignore ZstCableCompare;

	%template(ZstSerialisableCable) ZstSerialisable<Cable, CableData>;
}

%include <ZstCable.h>
