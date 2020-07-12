namespace showtime {
	%ignore ZstCableAddress::ZstCableAddress(ZstCableAddress &&);
	%ignore ZstCableAddress::operator=;
	%ignore ZstCableAddress::operator==;
	%ignore ZstCableAddress::operator!=;
	%ignore ZstCableAddress::operator<;
	%ignore ZstCableAddressHash;
	%ignore ZstCableAddressEq;

	class Cable;
	%ignore ZstCableAddress::ZstCableAddress(Cable);
}

%include <showtime/ZstCableAddress.h>
