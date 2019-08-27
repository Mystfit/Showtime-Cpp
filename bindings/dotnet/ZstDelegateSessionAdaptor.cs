public class ZstSessionEvents 
{
    public delegate void CableDlg(ZstCable cable);

    public CableDlg on_cable_created_events;
    public CableDlg on_cable_destroyed_events;
}

public sealed class ZstDelegateSessionAdaptor : ZstSessionAdaptor
{
    private ZstSessionEvents m_events;

    public ZstDelegateSessionAdaptor(ZstSessionEvents events)
    {
        m_events = events;
    }

    public override void on_cable_created(ZstCable cable){
        m_events?.on_cable_created_events(cable);
    }

    public override void on_cable_destroyed(ZstCable cable){
        m_events?.on_cable_destroyed_events(cable);
    }
}
