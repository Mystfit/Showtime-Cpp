%pragma(csharp) modulecode=%{
  private static ZstSessionEvents s_session_events = null;
  private static ZstHierarchyEvents s_hierarchy_events = null;

  private static ZstDelegateSessionAdaptor s_session_adaptor = null;
  private static ZstDelegateHierarchyAdaptor s_hierarchy_adaptor = null;

  public static ZstSessionEvents session_events() {
    if(s_session_events == null){
      s_session_events = new ZstSessionEvents();
      s_session_adaptor = new ZstDelegateSessionAdaptor(s_session_events);
      add_session_adaptor(s_session_adaptor);
    }
    return s_session_events;
  }

  public static ZstHierarchyEvents hierarchy_events() {
    if(s_hierarchy_events == null){
      s_hierarchy_events = new ZstHierarchyEvents();
      s_hierarchy_adaptor = new ZstDelegateHierarchyAdaptor(s_hierarchy_events);
      add_hierarchy_adaptor(s_hierarchy_adaptor);
    }
    return s_hierarchy_events;
  }

  public static ZstEntityBundle get_performers() {
    ZstEntityBundle bundle = new ZstEntityBundle();
    showtime.get_performers(bundle);
    return bundle;
  }
%}
