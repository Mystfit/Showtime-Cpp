#include <RtMidi.h>
#include <showtime/ZstExports.h>
#include <showtime/entities/ZstComponent.h>
#include <showtime/entities/ZstPlug.h>


#define MIDIIN_COMPONENT_TYPE "midiport"

class MidiPort : public showtime::ZstComponent {
public:
	ZST_PLUGIN_EXPORT MidiPort(const char* name);
	ZST_PLUGIN_EXPORT virtual void on_registered() override;
	ZST_PLUGIN_EXPORT virtual void compute(showtime::ZstInputPlug* plug) override;

	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* set_in_port();
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* set_out_port();
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* play_note();
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* release_note();
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* send_cc();
	ZST_PLUGIN_EXPORT showtime::ZstInputPlug* send_sysex();

	ZST_PLUGIN_EXPORT showtime::ZstOutputPlug* received_note();
	ZST_PLUGIN_EXPORT showtime::ZstOutputPlug* received_cc();
	ZST_PLUGIN_EXPORT showtime::ZstOutputPlug* received_sysex();

private:
	std::unique_ptr<RtMidiIn> m_midiin;
	std::unique_ptr<RtMidiOut> m_midiout;

	// Config plugs
	std::unique_ptr<showtime::ZstInputPlug> m_set_in_port;
	std::unique_ptr<showtime::ZstInputPlug> m_set_out_port;

	// Input plugs
	std::unique_ptr<showtime::ZstInputPlug> m_received_note;
	std::unique_ptr<showtime::ZstInputPlug> m_received_cc;
	std::unique_ptr<showtime::ZstInputPlug> m_received_sysex;

	// Output plugs
	std::unique_ptr<showtime::ZstOutputPlug> m_play_note;
	std::unique_ptr<showtime::ZstOutputPlug> m_release_note;
	std::unique_ptr<showtime::ZstOutputPlug> m_send_cc;
	std::unique_ptr<showtime::ZstOutputPlug> m_send_sysex;
};
