namespace showtime {
	%ignore ZstCableAddress::ZstCableAddress(ZstCableAddress &&);
	%ignore ZstCableAddress::operator=;
	%ignore ZstCableAddress::operator==;
	%ignore ZstCableAddress::operator!=;
	%ignore ZstCableAddress::operator<;
	%ignore ZstCableAddressHash;
	%ignore ZstCableAddressEq;
}

%include <ZstCableAddress.h>
