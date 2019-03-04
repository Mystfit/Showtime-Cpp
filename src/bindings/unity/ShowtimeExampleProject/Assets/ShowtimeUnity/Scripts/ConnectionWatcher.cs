public class ConnectionWatcher : ZstSessionAdaptor
{
    public delegate void ConnectionWatcherDlg();
    public ConnectionWatcherDlg on_connected;
    public ConnectionWatcherDlg on_disconnected;
    
    public override void on_connected_to_stage()
    {
        on_connected();
    }

    public override void on_disconnected_from_stage()
    {
        on_disconnected();
    }
}
