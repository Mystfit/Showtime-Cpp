namespace showtime {
  %typemap(cscode) ZstSynchronisable %{
    private ZstSynchronisableEvents m_synchronisable_events = null;
    private ZstDelegateSynchronisableAdaptor m_synchronisable_adaptor = null;

    public ZstSynchronisableEvents synchronisable_events() {
      if(m_synchronisable_events == null){
        m_synchronisable_events = new ZstSynchronisableEvents();
        m_synchronisable_adaptor = new ZstDelegateSynchronisableAdaptor(m_synchronisable_events);
        this.add_adaptor(m_synchronisable_adaptor);
      }
      return m_synchronisable_events;
    }
  %}

  %rename(synchronisable_event_dispatcher) ZstSynchronisable::synchronisable_events;
  %ignore ZstSynchronisable::synchronisable_events;
}
