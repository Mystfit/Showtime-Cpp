public class ZstHierarchyEvents
{
    public delegate void PerformerDlg(ZstPerformer performer);
    public delegate void EntityDlg(ZstEntityBase entity);
    public delegate void FactoryDlg(ZstEntityFactory factory);

    public PerformerDlg on_performer_arriving_events;
    public PerformerDlg on_performer_leaving_events;

    public EntityDlg on_entity_arriving_events;
    public EntityDlg on_entity_leaving_events;

    public FactoryDlg on_factory_arriving_events;
    public FactoryDlg on_factory_leaving_events;
}


public sealed class ZstDelegateHierarchyAdaptor : ZstHierarchyAdaptor
{
    private readonly ZstHierarchyEvents m_events;

    public ZstDelegateHierarchyAdaptor(ZstHierarchyEvents events)
    {
        m_events = events;
    }

    public override void on_performer_arriving(ZstPerformer performer)
    {
        m_events?.on_performer_arriving_events?.Invoke(performer);
    }

    public override void on_performer_leaving(ZstPerformer performer)
    {
        m_events?.on_performer_arriving_events?.Invoke(performer);
    }

    public override void on_entity_arriving(ZstEntityBase entity)
    {
        m_events?.on_entity_arriving_events?.Invoke(entity);
    }

    public override void on_entity_leaving(ZstEntityBase entity)
    {
        m_events?.on_entity_leaving_events?.Invoke(entity);
    }

    public override void on_factory_arriving(ZstEntityFactory factory)
    {
        m_events?.on_factory_arriving_events?.Invoke(factory);
    }

    public override void on_factory_leaving(ZstEntityFactory factory)
    {
        m_events?.on_factory_leaving_events?.Invoke(factory);
    }
}
