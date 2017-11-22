#include "ZstPlugRef.h"

using namespace std;

ZstPlugRef::ZstPlugRef(ZstURI uri, PlugDirection dir) : m_URI(uri), m_direction(dir)
{
}

ZstPlugRef::~ZstPlugRef()
{
}

ZstURI ZstPlugRef::URI()
{
	return m_URI;
}

PlugDirection ZstPlugRef::get_direction(){
    return m_direction;
}
