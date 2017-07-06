#include "Showtime.h"
#include "ZstEndpoint.h"
#include "ZstMessages.h"
#include "ZstPerformer.h"
#include "ZstPlug.h"
#include "ZstEvent.h"

using namespace std;

RuntimeLanguage Showtime::_runtime_language = RuntimeLanguage::NATIVE_RUNTIME;

Showtime::Showtime(){
}

Showtime::~Showtime(){
}

ZstEndpoint & Showtime::endpoint()
{
	static ZstEndpoint endpoint_singleton;
	return endpoint_singleton;
}

void Showtime::destroy() {
	Showtime::endpoint().destroy();
}

void Showtime::init()
{
	Showtime::endpoint().init();
}

void Showtime::join(const char * stage_address){
	cout << "ZST: Connecting to stage located at " << stage_address << endl;
	Showtime::endpoint().register_endpoint_to_stage(stage_address);
}

void Showtime::leave()
{
	return Showtime::endpoint().leave_stage();
}

void Showtime::poll_once()
{
	while (Showtime::event_queue_size() > 0) {
		ZstPerformer * performer = NULL;
		ZstEvent e = Showtime::pop_event();
        cout << "ZST: Received event " << e.get_update_type() << endl;

		switch (e.get_update_type()) {
		case ZstEvent::EventType::PLUG_HIT:
			cout << "Poll once: Plug hit" << endl;

			performer = Showtime::endpoint().get_performer_by_URI(e.get_first());
			if (performer != NULL) {
				ZstPlug * plug = performer->get_plug_by_URI(e.get_first());
				if (plug != NULL) {
					cout << "Poll once: Plug running callbacks" << endl;

					plug->run_recv_callbacks();
				}
			}
			break;
		default:
			cout << "Poll once: Stage event" << endl;
			Showtime::endpoint().run_stage_event_callbacks(e);
		}
	}
}

void Showtime::attach_stage_event_callback(ZstEventCallback * callback) {
	Showtime::endpoint().attach_stage_event_callback(callback);
}

void Showtime::remove_stage_event_callback(ZstEventCallback * callback) {
	Showtime::endpoint().remove_stage_event_callback(callback);
}

void Showtime::set_runtime_language(RuntimeLanguage runtime)
{
	_runtime_language = runtime;
}

RuntimeLanguage Showtime::get_runtime_language()
{
	return _runtime_language;
}


// -------------
// API Functions
// -------------

int Showtime::ping_stage(){
	return Showtime::endpoint().ping_stage();
}

ZstPerformer * Showtime::create_performer(const char * name)
{
	return Showtime::endpoint().create_performer(ZstURI(name, "", "", ZstURI::Direction::NONE));
}

ZstIntPlug * Showtime::create_int_plug(ZstURI * uri)
{
	return Showtime::endpoint().create_int_plug(uri);
}

ZstPerformer * Showtime::get_performer_by_URI(const ZstURI * uri)
{
	return Showtime::endpoint().get_performer_by_URI(*uri);
}

ZstEvent Showtime::pop_event()
{
	return Showtime::endpoint().pop_plug_event();
}

int Showtime::event_queue_size()
{
	return Showtime::endpoint().plug_event_queue_size();
}

int Showtime::destroy_plug(ZstPlug * plug)
{
	return Showtime::endpoint().destroy_plug(plug);
}

int Showtime::connect_cable(const ZstURI * a, const ZstURI * b)
{
	return Showtime::endpoint().connect_cable(a, b);
}

int Showtime::destroy_cable(const ZstURI * a, const ZstURI * b)
{
	return Showtime::endpoint().destroy_cable(a, b);
}
