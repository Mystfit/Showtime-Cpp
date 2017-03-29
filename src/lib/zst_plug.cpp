#pragma once
 
#include "zst_plug.h"

using namespace Showtime;

ZstPlug::ZstPlug()
{
}

DLL_EXPORT ZstPlug * ZstPlug::create_plug(string name, string ownerName, PlugMode mode, const string args[], function<void(string)> callback)
{
	//Initialize plug here
	return new ZstPlug();
}

DLL_EXPORT string Showtime::ZstPlug::get_name()
{
	return m_name;
}
