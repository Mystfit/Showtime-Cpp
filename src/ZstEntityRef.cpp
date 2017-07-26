#include "ZstEntityRef.h"

using namespace std;

ZstEntityRef::ZstEntityRef(ZstURI uri, std::string entity_type) : m_URI(uri), m_entity_type(entity_type)
{
}

ZstURI ZstEntityRef::get_URI()
{
	return m_URI;
}

string ZstEntityRef::get_entity_type()
{
	return m_entity_type;
}

ZstEntityRef::~ZstEntityRef()
{
}
