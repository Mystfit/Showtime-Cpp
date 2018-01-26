#include <ZstEvents.h>
#include <ZstSynchronisable.h>
#include <entities/ZstEntityBase.h>
#include <entities/ZstComponent.h>
#include <entities/ZstPerformer.h>
#include <entities/ZstPlug.h>
#include <ZstCable.h>


ZstEvent::ZstEvent() : m_num_calls(0) 
{
}

int ZstEvent::num_calls() const 
{
	return m_num_calls; 

}

void ZstEvent::reset_num_calls() 
{ 
	m_num_calls = 0; 
}

void ZstEvent::increment_calls() 
{
	m_num_calls++; 
}

void ZstSynchronisableEvent::cast_run(ZstSynchronisable * target)
{
	this->increment_calls();
	this->run(target);
}

void ZstEntityEvent::cast_run(ZstSynchronisable * target)
{
	ZstEntityBase * entity = dynamic_cast<ZstEntityBase*>(target);
	if (entity) {
		this->increment_calls();
		this->run(entity);
	}
}

void ZstComponentEvent::cast_run(ZstSynchronisable * target)
{
	ZstComponent * component = dynamic_cast<ZstComponent*>(target);
	if (component) {
		this->increment_calls();
		this->run(component);
	}
}

void ZstComponentTypeEvent::cast_run(ZstSynchronisable * target)
{
	ZstComponent * component = dynamic_cast<ZstComponent*>(target);
	if (component) {
		this->increment_calls();
		this->run(component);
	}
}

void ZstCableEvent::cast_run(ZstSynchronisable * target)
{
	ZstCable * cable = dynamic_cast<ZstCable*>(target);
	if (cable) {
		this->increment_calls();
		this->run(cable);
	}
}

void ZstPlugEvent::cast_run(ZstSynchronisable * target)
{
	ZstPlug * plug = dynamic_cast<ZstPlug*>(target);
	if (plug) {
		this->increment_calls();
		this->run(plug);
	}
}

void ZstPerformerEvent::cast_run(ZstPerformer * target)
{
	ZstPerformer * performer = dynamic_cast<ZstPerformer*>(target);
	if (performer) {
		this->increment_calls();
		this->run(performer);
	}
}
