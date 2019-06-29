using System.Collections;
using System.Collections.Generic;
using UnityEngine;


public class TransformableComponent : MonoBehaviour
{
    public ZstComponent component;
    private ZstObservedTransformAdaptor transform_observer;

    public float UpdateRate = 0.2f;

    public void Init(string name)
    {
        //Create component
        ZstTransformableComponent transform_comp = new ZstTransformableComponent(name);

        //Hold on to component
        this.component = transform_comp;

        //Set starting position of plug to the current transform position
        WriteTransformPlug(transform);

        //Start the network update loop
        StartCoroutine("NetworkUpdate");
    }

    public ZstOutputPlug OutputTransform()
    {
        ZstURI plug_path = component.URI().add(new ZstURI("out_transform"));
        ZstEntityBase entity = showtime.find_entity(plug_path);
        ZstOutputPlug plug = showtime.cast_to_output_plug(entity);
        return plug;
    }

    public ZstInputPlug InputTransform()
    {
        ZstURI plug_path = component.URI().add(new ZstURI("in_transform"));
        ZstEntityBase entity = showtime.find_entity(plug_path);
        ZstInputPlug plug = showtime.cast_to_input_plug(entity);
        return plug;
    }

    public void WrapProxyComponent(ZstComponent component)
    {
        if (component == null) return;

        Debug.Log($"Wrapping proxy component {component.URI().ToString()}");

        //Hold on to proxy component reference
        this.component = component;

        //Find the output transform plug. Since this is a proxy component, we use the Showtime API to locate it
        ZstOutputPlug transform_plug = showtime.cast_to_output_plug(component.get_child_by_URI(component.URI().add(new ZstURI("out_transform"))));

        Debug.Log($"Observing {transform_plug}");

        //Let the position plug know that we care about its values
        showtime.observe_entity(transform_plug);

        //Create ObservedPositionAdaptor to watch for updates on the position plug
        transform_observer = new ZstObservedTransformAdaptor
        {
            on_plug_updated = ReceivePlugUpdate
        };
        transform_plug.add_adaptor(transform_observer);

        //Set the starting position of the proxy to the plug value
        this.ReadTransformPlug(transform_plug);
    }

    public void ReceivePlugUpdate(ZstPlug plug)
    {
        this.ReadTransformPlug(plug);
    }

    public void ReceiveTransform(Vector3 pos, Quaternion rot, Vector3 scale)
    {
        transform.localPosition = pos;
        transform.localRotation = rot;
        transform.localScale = scale;
    }

    void OnDestroy()
    {
        if (!component?.is_proxy() ?? false)
        {
            StopCoroutine("NetworkUpdate");
            showtime.deactivate_entity(component);
        }
    }

    public void WriteTransformPlug(Transform t)
    {
        ZstTransformableComponent comp = this.component as ZstTransformableComponent;
        comp.out_transform.append_float(t.localPosition.x);
        comp.out_transform.append_float(t.localPosition.y);
        comp.out_transform.append_float(t.localPosition.z);
        comp.out_transform.append_float(t.localRotation.x);
        comp.out_transform.append_float(t.localRotation.y);
        comp.out_transform.append_float(t.localRotation.z);
        comp.out_transform.append_float(t.localRotation.w);
        comp.out_transform.append_float(t.localScale.x);
        comp.out_transform.append_float(t.localScale.y);
        comp.out_transform.append_float(t.localScale.z);
    }

    public void ReadTransformPlug(ZstPlug plug)
    {
        transform.localPosition = new Vector3(plug.float_at(0), plug.float_at(1), plug.float_at(2));
        transform.localRotation = new Quaternion(plug.float_at(3), plug.float_at(4), plug.float_at(5), plug.float_at(6));
        transform.localScale = new Vector3(plug.float_at(7), plug.float_at(8), plug.float_at(9));
    }

    IEnumerator NetworkUpdate()
    {
        while (true)
        {
            yield return new WaitForSeconds(this.UpdateRate);

            if (!this.component?.is_proxy() ?? false && this.transform.hasChanged)
            {
                this.WriteTransformPlug(transform);
                (this.component as ZstTransformableComponent).out_transform.fire();
            }
        }
    }
}


//------------------



public class ZstTransformableComponent : ZstComponent
{
    //Plugs
    public ZstOutputPlug out_transform;
    public ZstInputPlug in_transform;

    //Delegates
    public delegate void PlugUpdateDlg(ZstPlug plug);
    public PlugUpdateDlg on_plug_updated;

    public ZstTransformableComponent(string name) : base(typeof(ZstTransformableComponent).ToString(), name)
    {
        out_transform = create_output_plug("out_transform", ZstValueType.ZST_FLOAT);
        in_transform = create_input_plug("in_transform", ZstValueType.ZST_FLOAT);
    }

    public override void compute(ZstInputPlug plug)
    {
        if (plug == in_transform)
        {
            on_plug_updated.Invoke(in_transform);
        }
    }
}


public class ZstObservedTransformAdaptor : ZstSynchronisableAdaptor
{
    public delegate void PlugUpdateDlg(ZstPlug plug);
    public PlugUpdateDlg on_plug_updated;

    public override void on_synchronisable_updated(ZstSynchronisable synchronisable)
    {
        ZstOutputPlug plug = showtime.cast_to_output_plug(synchronisable);
        on_plug_updated.Invoke(plug);
    }
}
