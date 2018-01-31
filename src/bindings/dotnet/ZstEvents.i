%feature("director") ZstSynchronisableEvent;
%feature("director") ZstActivationEvent;
%feature("director") ZstDeactivationEvent;
%feature("director") ZstEntityEvent;
%feature("director") ZstComponentEvent;
%feature("director") ZstCableEvent;
%feature("director") ZstPlugEvent;
%feature("director") ZstInputPlugEvent;
%feature("director") ZstPerformerEvent;

%rename("run_with_entity") ZstEntityEvent::run;
%rename("run_with_component") ZstComponentEvent::run;
%rename("run_with_cable") ZstCableEvent::run;
%rename("run_with_plug") ZstPlugEvent::run;
%rename("run_with_input_plug") ZstInputPlugEvent::run;
%rename("run_with_performer") ZstPerformerEvent::run;

%include <ZstEvents.h> 