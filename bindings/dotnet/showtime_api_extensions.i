%typemap(cscode) ShowtimeClient %{
  private ZstSessionEvents m_session_events = null;
  private ZstHierarchyEvents m_hierarchy_events = null;
  private ZstConnectionEvents m_connection_events = null;

  private ZstDelegateSessionAdaptor m_session_adaptor = null;
  private ZstDelegateHierarchyAdaptor m_hierarchy_adaptor = null;
  private ZstDelegateConnectionAdaptor m_connection_adaptor = null;

  public ZstSessionEvents session_events() {
    if(m_session_events == null){
      m_session_events = new ZstSessionEvents();
      m_session_adaptor = new ZstDelegateSessionAdaptor(m_session_events);
      add_session_adaptor(m_session_adaptor);
    }
    return m_session_events;
  }

  public ZstHierarchyEvents hierarchy_events() {
    if(m_hierarchy_events == null){
      m_hierarchy_events = new ZstHierarchyEvents();
      m_hierarchy_adaptor = new ZstDelegateHierarchyAdaptor(m_hierarchy_events);
      add_hierarchy_adaptor(m_hierarchy_adaptor);
    }
    return m_hierarchy_events;
  }

  public ZstConnectionEvents connection_events() {
    if(m_connection_events == null){
      m_connection_events = new ZstConnectionEvents();
      m_connection_adaptor = new ZstDelegateConnectionAdaptor(m_connection_events);
      add_connection_adaptor(m_connection_adaptor);
    }
    return m_connection_events;
  }

  public ZstEntityBundle get_performers() {
    ZstEntityBundle bundle = new ZstEntityBundle();
    get_performers(bundle);
    return bundle;
  }
%}
