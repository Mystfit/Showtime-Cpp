#include "ZstValueWire.h"

ZstValueWire::ZstValueWire() : ZstValue()
{
}

ZstValueWire::ZstValueWire(const ZstValueWire & copy) : ZstValue(copy)
{
}

ZstValueWire::ZstValueWire(const ZstValue & copy) : ZstValue(copy)
{
}

ZstValueWire::ZstValueWire(ZstValueType t) : ZstValue(t)
{
}
