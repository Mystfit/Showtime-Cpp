namespace showtime {
  %typemap(cscode) ZstEntityBase %{
    private ZstEntityEvents m_entity_events = null;
    private ZstDelegateEntityAdaptor m_entity_adaptor = null;

    public ZstEntityEvents entity_events() {
      if(m_entity_events == null){
        m_entity_events = new ZstEntityEvents();
        m_entity_adaptor = new ZstDelegateEntityAdaptor(m_entity_events);
        this.add_adaptor(m_entity_adaptor);
      }
      return m_entity_events;
    }
  %}

  %rename(entity_event_dispatcher) ZstEntityBase::entity_events;
  %ignore ZstEntityBase::entity_events;

  %rename(session_event_dispatcher) ZstEntityBase::session_events;
  %ignore ZstEntityBase::session_events;
}