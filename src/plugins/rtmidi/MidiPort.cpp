#include "MidiPort.h"
#include <showtime/ZstLogging.h>
#include <showtime/ZstConstants.h>

using namespace showtime;

MidiPort::MidiPort(const char* name) :
	showtime::ZstComponent(MIDIIN_COMPONENT_TYPE, name),
	m_set_in_port(std::make_unique<showtime::ZstInputPlug>("set_midi_in_port", ZstValueType::StrList)),
	m_set_out_port(std::make_unique<showtime::ZstInputPlug>("set_midi_out_port", ZstValueType::StrList)),
	m_received_note(std::make_unique<showtime::ZstInputPlug>("received_note", ZstValueType::IntList)),
	m_received_cc(std::make_unique<showtime::ZstInputPlug>("received_cc", ZstValueType::IntList)),
	m_received_sysex(std::make_unique<showtime::ZstInputPlug>("received_sysex", ZstValueType::IntList)),
	m_play_note(std::make_unique<showtime::ZstOutputPlug>("play_note", ZstValueType::IntList)),
	m_release_note(std::make_unique<showtime::ZstOutputPlug>("release_note", ZstValueType::IntList)),
	m_send_cc(std::make_unique<showtime::ZstOutputPlug>("send_cc", ZstValueType::IntList)),
	m_send_sysex(std::make_unique<showtime::ZstOutputPlug>("send_sysex", ZstValueType::IntList))
{
	try {
		m_midiin = std::make_unique<RtMidiIn>();
	}
	catch (RtMidiError& error) {
		Log::app(Log::Level::error, "Midi In construction failed: {}", error.getMessage().c_str());
	}

	try {
		m_midiout = std::make_unique<RtMidiOut>();
	}
	catch (RtMidiError& error) {
		Log::app(Log::Level::error, "Midi Out construction failed: {}", error.getMessage().c_str());
	}
}

void MidiPort::on_registered()
{
	add_child(m_set_in_port.get());
	add_child(m_set_out_port.get());
	add_child(m_received_note.get());
	add_child(m_received_cc.get());
	add_child(m_received_sysex.get());
	add_child(m_play_note.get());
	add_child(m_release_note.get());
	add_child(m_send_cc.get());
	add_child(m_send_sysex.get());
}

void MidiPort::compute(showtime::ZstInputPlug* plug)
{
}
