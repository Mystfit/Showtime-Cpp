#include "ZstPlugRef.h"

using namespace std;

ZstPlugRef::ZstPlugRef(PlugAddress address)
{
	m_address = address;
}

ZstPlugRef::~ZstPlugRef()
{
}

PlugAddress ZstPlugRef::get_address()
{
	return m_address;
}
