#pragma once
 
#include "zst_instrument.h" 

Showtime::ZstInstrument::ZstInstrument()
{
}

DLL_EXPORT Showtime::ZstInstrument* Showtime::ZstInstrument::create_node()
{
	return new ZstInstrument();
}