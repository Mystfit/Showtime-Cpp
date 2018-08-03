using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ZstTransformableComponent : ZstComponent
{
    public ZstOutputPlug position;

    public ZstTransformableComponent(string name) : base(typeof(ZstTransformableComponent).ToString(), name)
    {
        position = create_output_plug("position", ZstValueType.ZST_FLOAT);
    }

    public override void compute(ZstInputPlug plug)
    {
    }
}


public class TransformableComponent : MonoBehaviour
{
    private ZstComponent component;
    private ZstOutputPlug position_plug;
    private Vector3 last_position;
    private ObservedPositionAdaptor position_observer;

    public void Init(string name)
    {
        ZstTransformableComponent comp = new ZstTransformableComponent(name);

        //Set starting position of plug to the current transform position
        comp.position.append_float(transform.position.x);
        comp.position.append_float(transform.position.y);
        comp.position.append_float(transform.position.z);

        //Activate entity and hold on to references
        showtime.activate_entity(comp);
        this.position_plug = comp.position;
        this.component = comp;
    }

    public void Init(ZstComponent component)
    {
        //Hold on to proxy component reference
        this.component = component;

        //Create shortcut to position plug
        this.position_plug = showtime.cast_to_output_plug(component.get_plug_by_URI(component.URI().add(new ZstURI("position"))));

        //Let the position plug know that we care about its values
        showtime.observe_entity(this.position_plug);

        //Create ObservedPositionAdaptor to watch for updates on the position plug
        position_observer = new ObservedPositionAdaptor();
        position_observer.position_updated = ReceivePosition;
        this.position_plug.add_adaptor(position_observer);

        //Set the starting position of the proxy to the plug value
        ReceivePosition(new Vector3(position_plug.float_at(0), position_plug.float_at(1), position_plug.float_at(2)));
    }

    public void ReceivePosition(Vector3 position)
    {
        transform.position = position;
    }

    public void SendPosition(Vector3 position)
    {
        position_plug.append_float(position.x);
        position_plug.append_float(position.y);
        position_plug.append_float(position.z);
        position_plug.fire();
    }

    public void OnDestroy()
    {
        if (component != null)
        {
            if (!component.is_proxy())
            {
                showtime.deactivate_entity(component);
            }
        }
    }

    // Update is called once per frame
    void FixedUpdate()
    {
        if (component != null)
        {
            if (!component.is_proxy() && last_position != transform.position)
            {
                SendPosition(transform.position);
            }
        }
    }
}

public class ObservedPositionAdaptor : ZstSynchronisableAdaptor
{
    public delegate void TransformablePlugDlg(Vector3 position);
    public TransformablePlugDlg position_updated;

    public override void on_synchronisable_updated(ZstSynchronisable synchronisable)
    {
        ZstOutputPlug p = showtime.cast_to_output_plug(synchronisable);
        position_updated(new Vector3(p.float_at(0), p.float_at(1), p.float_at(2)));
    }
}