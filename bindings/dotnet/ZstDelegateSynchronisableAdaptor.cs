public class ZstSynchronisableEvents
{
    public delegate void ZstSynchronisableDlg(ZstSynchronisable synchronisable);

    public ZstSynchronisableDlg on_synchronisable_activated_events;
    public ZstSynchronisableDlg on_synchronisable_deactivated_events;
    public ZstSynchronisableDlg on_synchronisable_destroyed_events;
    public ZstSynchronisableDlg on_synchronisable_updated_events;
}


public sealed class ZstDelegateSynchronisableAdaptor : ZstSynchronisableAdaptor
{
    private ZstSynchronisableEvents m_events;

    public ZstDelegateSynchronisableAdaptor(ZstSynchronisableEvents events)
    {
        m_events = events;
    }

    public override void on_synchronisable_activated(ZstSynchronisable synchronisable){
        m_events?.on_synchronisable_activated_events(synchronisable);
    }

    public override void on_synchronisable_deactivated(ZstSynchronisable synchronisable){
        m_events?.on_synchronisable_deactivated_events(synchronisable);
    }

    public override void on_synchronisable_destroyed(ZstSynchronisable synchronisable){
        m_events?.on_synchronisable_destroyed_events(synchronisable);
    }

    public override void on_synchronisable_updated(ZstSynchronisable synchronisable){
        m_events?.on_synchronisable_updated_events(synchronisable);
    }
}
