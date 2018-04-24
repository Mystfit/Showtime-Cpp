#include <adaptors/ZstEventAdaptor.hpp>
#include <adaptors/ZstPlugAdaptors.hpp>
#include <adaptors/ZstSynchronisableAdaptor.hpp>

// ----------------------------
// Plug adaptor stub functions
// ----------------------------

void ZstOutputPlugAdaptor::on_plug_fire(ZstOutputPlug * plug) {}


// --------------------------------------
// Synchronisable adaptor stub functions
// --------------------------------------

void ZstSynchronisableAdaptor::notify_event_ready(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_activated(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_deactivated(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_destroyed(ZstSynchronisable * synchronisable) {}
