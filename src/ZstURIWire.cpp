#include "ZstURIWire.h"

using namespace std;

ZstURIWire::ZstURIWire()
{
}

ZstURIWire::ZstURIWire(const ZstURIWire & copy) : ZstURI(copy)
{
}

ZstURIWire::ZstURIWire(ZstURI uri) : ZstURI(uri)
{
}
