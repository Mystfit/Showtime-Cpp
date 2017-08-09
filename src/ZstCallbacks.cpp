#include "entities/ZstComponent.h"
#include "ZstCallbacks.h"

// -----------------------
// Filter compute callback
// -----------------------
void ZstComputeCallback::set_target_filter(ZstComponent * component)
{
    m_component = component;
}

void ZstComputeCallback::run(ZstInputPlug * plug)
{
    m_component->compute(plug);
}
