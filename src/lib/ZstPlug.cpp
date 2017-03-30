#pragma once
 
#include "ZstPlug.h"

using namespace Showtime;

ZstPlug::ZstPlug(string name, PlugMode mode)
{
	m_name = name;
	m_plug_mode = mode;
}

string ZstPlug::get_name()
{
	return m_name;
}

ZstPlug::PlugMode ZstPlug::get_mode()
{
	return m_plug_mode;
}
