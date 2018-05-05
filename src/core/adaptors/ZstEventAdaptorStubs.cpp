#include <adaptors/ZstEventAdaptor.hpp>
#include <adaptors/ZstPlugAdaptors.hpp>
#include <adaptors/ZstSynchronisableAdaptor.hpp>
#include <adaptors/ZstSessionAdaptor.hpp>
#include <adaptors/ZstHierarchyAdaptor.hpp>

// ----------------------------
// Plug adaptor stub functions
// ----------------------------

void ZstOutputPlugAdaptor::on_plug_fire(ZstOutputPlug * plug) {}


// --------------------------------------
// Synchronisable adaptor stub functions
// --------------------------------------

void ZstSynchronisableAdaptor::synchronisable_has_event(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_activated(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_deactivated(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_destroyed(ZstSynchronisable * synchronisable) {}


// -----------------------
// Session adaptors
// -----------------------

void ZstSessionAdaptor::on_connected_to_stage() {}
void ZstSessionAdaptor::on_disconnected_from_stage() {};

void ZstSessionAdaptor::on_cable_created(ZstCable * cable) {}
void ZstSessionAdaptor::on_cable_destroyed(ZstCable * cable) {}


// -----------------------
// Hierarchy adaptors
// -----------------------

void ZstHierarchyAdaptor::on_performer_arriving(ZstPerformer * performer) {}
void ZstHierarchyAdaptor::on_performer_leaving(ZstPerformer * performer) {}

void ZstHierarchyAdaptor::on_entity_arriving(ZstEntityBase * entity) {}
void ZstHierarchyAdaptor::on_entity_leaving(ZstEntityBase * entity) {}

void ZstHierarchyAdaptor::on_plug_arriving(ZstPlug * plug) {}
void ZstHierarchyAdaptor::on_plug_leaving(ZstPlug * plug) {}

