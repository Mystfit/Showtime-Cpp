#include "ZstCallbacks.h"
#include "ZstComponent.h"

class ComputeCallback : public ZstPlugDataEventCallback {
public:
    ZST_EXPORT void set_target_filter(ZstComponent * component);
    ZST_EXPORT void run(ZstInputPlug * plug) override;
private:
    ZstComponent * m_component;
};