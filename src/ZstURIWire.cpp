#include "ZstURIWire.h"

using namespace std;

ZstURIWire::ZstURIWire() : ZstURI()
{
}

ZstURIWire::ZstURIWire(const ZstURIWire & copy) : ZstURI(copy)
{
}

ZstURIWire::ZstURIWire(const ZstURI & uri) : ZstURI(uri)
{
}

ZstURIWire::~ZstURIWire()
{
}
