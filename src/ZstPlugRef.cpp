#include "ZstPlugRef.h"

using namespace std;

ZstPlugRef::ZstPlugRef(ZstURI address) : m_address(address)
{
}

ZstPlugRef::~ZstPlugRef()
{
}

ZstURI ZstPlugRef::get_address()
{
	return m_address;
}
