def entity_bundle_to_list(bundle):
    list = []
    for entity in bundle.items():
        if entity.entity_type() == ZstEntityType_COMPONENT: 
            list.append(cast_to_component(entity))
        elif entity.entity_type() == ZstEntityType_PERFORMER: 
            list.append(cast_to_performer(entity))
        elif entity.entity_type() == ZstEntityType_PLUG: 
            plug = cast_to_input_plug(entity)
            if not plug:
                plug = cast_to_output_plug(entity)
            list.append(plug)
        elif entity.entity_type() == ZstEntityType_FACTORY: 
            list.append(cast_to_factory(entity))
    return list

def bundle_to_list(bundle):
    list = []
    for item in bundle.items():
        list.append(item)
    return list


# Callable set for passing arguments to python callback functions
class CallbackList(set):
    def fire(self, *args, **kwargs):
        for listener in self:
            listener(*args, **kwargs)

# Meta-class generator for adaptor callback wrappers
def generate_event_wrapper(adaptor):
    callback_names = [func for func in dir(adaptor) if func.startswith("on_")]
    callback_lists = dict()
    attrs = dict()
    
    def generate_callback(name):
        cb_name = name.replace('on_', '')
        def callback(self, *args):
            getattr(self, cb_name).fire(*args)
        return callback

    def generate_init(adaptor, callbacks):
        def init(self):
            adaptor.__init__(self)
            for cb in callbacks:
                setattr(self, cb.replace('on_', ''), CallbackList())
        return init

    attrs["__init__"] = generate_init(adaptor, callback_names)
    for cb in callback_names:
        attrs[cb] = generate_callback(cb)
        wrapper_name = adaptor.__name__.replace('Zst', '').replace('Adaptor', 'EventHandler')
    
    return type(wrapper_name, (adaptor,), attrs)
