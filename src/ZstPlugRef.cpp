#include "ZstPlugRef.h"

using namespace std;

ZstPlugRef::ZstPlugRef(ZstURI uri) : m_URI(uri)
{
}

ZstPlugRef::~ZstPlugRef()
{
}

ZstURI ZstPlugRef::get_URI()
{
	return m_URI;
}

