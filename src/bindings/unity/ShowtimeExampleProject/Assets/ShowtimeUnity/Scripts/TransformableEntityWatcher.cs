using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TransformableEntityWatcher : MonoBehaviour {

    public GameObject transformable_prefab;
    private TransformableEntityWatcherAdaptor adaptor;

    // Use this for initialization
    void Start () {
        adaptor = new TransformableEntityWatcherAdaptor();
        adaptor.component_arrive_dlg = TransformableArriving;
        adaptor.component_leave_dlg = TransformableLeaving;

        //Library must be initialised
        showtime.add_hierarchy_adaptor(adaptor);
    }

    // Update is called once per frame
    void Update () {
		
	}

    void TransformableArriving(ZstComponent component)
    {
        GameObject transformable = GameObject.Instantiate(transformable_prefab);
        transformable.name = component.URI().path();
        transformable.GetComponent<Rigidbody>().isKinematic = true;
        TransformableComponent visual_component = transformable.AddComponent<TransformableComponent>();
        visual_component.WrapProxyComponent(component);
    }

    void TransformableLeaving(ZstComponent component)
    {
        GameObject transformable = GameObject.Find(component.URI().path());
        if(transformable != null)
        {
            GameObject.Destroy(transformable);
        }
    }
}


class TransformableEntityWatcherAdaptor : ZstHierarchyAdaptor
{
    public delegate void TransformableComponentDlg(ZstComponent transformable);
    public TransformableComponentDlg component_arrive_dlg;
    public TransformableComponentDlg component_leave_dlg;

    public override void on_entity_arriving(ZstEntityBase entity)
    {
        ZstComponent component = showtime.cast_to_component(entity);
        if(component?.component_type() == typeof(ZstTransformableComponent).ToString())
        {
            component_arrive_dlg(component);
        }
    }

    public override void on_entity_leaving(ZstEntityBase entity)
    {
        ZstComponent component = showtime.cast_to_component(entity);
        if (component?.component_type() == typeof(ZstTransformableComponent).ToString())
        {
            component_leave_dlg(component);
        }
    }
}
