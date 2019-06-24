public class ZstSessionEvents 
{
    public delegate void ConnectionWatcherDlg();
    public delegate void ServerBeaconDlg(ZstServerAddress server);
    public delegate void CableDlg(ZstCable cable);

    public ConnectionWatcherDlg on_connected_to_stage_events;
    public ConnectionWatcherDlg on_disconnected_from_stage_events;
    public ServerBeaconDlg on_server_discovered_events;

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

    public override void on_connected_to_stage(){
        m_events.on_connected_to_stage_events();
    }

    public override void on_disconnected_from_stage(){
        m_events.on_disconnected_from_stage_events();
    }

    public override void on_server_discovered(ZstServerAddress server){
        m_events.on_server_discovered_events(server);
    }

    public override void on_cable_created(ZstCable cable){
        m_events.on_cable_created_events(cable);
    }

    public override void on_cable_destroyed(ZstCable cable){
        m_events.on_cable_destroyed_events(cable);
    }
}
