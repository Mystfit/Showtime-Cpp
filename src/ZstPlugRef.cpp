#include "ZstPlugRef.h"

using namespace std;

ZstPlugRef::ZstPlugRef(ZstURI address) : m_address(address)
{
}

ZstPlugRef::~ZstPlugRef()
{
}

ZstURI ZstPlugRef::get_URI()
{
	return m_address;
}

const std::vector<ZstURI> ZstPlugRef::get_output_connections() const
{
	return m_connections;
}

void ZstPlugRef::add_output_connection(ZstURI uri)
{
	return m_connections.push_back(uri);
}

void ZstPlugRef::remove_output_connection(ZstURI uri)
{
    throw runtime_error("TODO: Implement plug discconection on stage");
	return;
}
