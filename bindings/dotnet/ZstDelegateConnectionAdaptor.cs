public class ZstConnectionEvents
{
    public delegate void ConnectionWatcherDlg(ShowtimeClient client, ZstServerAddress server);
    public delegate void ServerBeaconDlg(ShowtimeClient client, ZstServerAddress server);

    public ConnectionWatcherDlg on_connected_to_stage_events;
    public ConnectionWatcherDlg on_disconnected_from_stage_events;
    public ServerBeaconDlg on_server_discovered_events;
}

public sealed class ZstDelegateConnectionAdaptor : ZstConnectionAdaptor
{
    private readonly ZstConnectionEvents m_events;

    public ZstDelegateConnectionAdaptor(ZstConnectionEvents events)
    {
        m_events = events;
    }

    public override void on_connected_to_stage(ShowtimeClient client, ZstServerAddress server)
    {
        m_events?.on_connected_to_stage_events(client, server);
    }

    public override void on_disconnected_from_stage(ShowtimeClient client, ZstServerAddress server)
    {
        m_events?.on_disconnected_from_stage_events(client, server);
    }

    public override void on_server_discovered(ShowtimeClient client, ZstServerAddress server)
    {
        m_events?.on_server_discovered_events(client, server);
    }
}
