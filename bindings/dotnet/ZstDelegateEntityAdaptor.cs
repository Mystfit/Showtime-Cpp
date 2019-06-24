public class ZstEntityEvents
{
    public delegate void EntityDlg(ZstEntityBase entity);
    public delegate void CableDlg(ZstCable cable);

    public EntityDlg on_publish_entity_update_events;
    public EntityDlg on_register_entity_events;
    public CableDlg on_disconnect_cable_events; 
}

public sealed class ZstDelegateEntityAdaptor : ZstEntityAdaptor
{
    private ZstEntityEvents m_events;

    public ZstDelegateEntityAdaptor(ZstEntityEvents events)
    {
        m_events = events;
    }

    public override void on_publish_entity_update(ZstEntityBase entity){
        m_events.on_publish_entity_update_events(entity);
    }

    public override void on_register_entity(ZstEntityBase entity){
        m_events.on_register_entity_events(entity);
    }

    public override void on_disconnect_cable(ZstCable cable){
        m_events.on_disconnect_cable_events(cable);
    }
}