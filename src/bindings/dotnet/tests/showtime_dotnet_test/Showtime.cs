//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (http://www.swig.org).
// Version 3.0.12
//
// Do not make changes to this file unless you know what you are doing--modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class Showtime : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal Showtime(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(Showtime obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~Showtime() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          showtime_dotnetPINVOKE.delete_Showtime(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public static SWIGTYPE_p_ZstEndpoint endpoint() {
    SWIGTYPE_p_ZstEndpoint ret = new SWIGTYPE_p_ZstEndpoint(showtime_dotnetPINVOKE.Showtime_endpoint(), false);
    return ret;
  }

  public static void destroy() {
    showtime_dotnetPINVOKE.Showtime_destroy();
  }

  public static void set_runtime_language(RuntimeLanguage runtime) {
    showtime_dotnetPINVOKE.Showtime_set_runtime_language((int)runtime);
  }

  public static RuntimeLanguage get_runtime_language() {
    RuntimeLanguage ret = (RuntimeLanguage)showtime_dotnetPINVOKE.Showtime_get_runtime_language();
    return ret;
  }

  public static void join(string stage_address) {
    showtime_dotnetPINVOKE.Showtime_join(stage_address);
  }

  public static SWIGTYPE_p_std__chrono__milliseconds ping_stage() {
    SWIGTYPE_p_std__chrono__milliseconds ret = new SWIGTYPE_p_std__chrono__milliseconds(showtime_dotnetPINVOKE.Showtime_ping_stage(), true);
    return ret;
  }

  public static SWIGTYPE_p_ZstPerformer create_performer(string name) {
    global::System.IntPtr cPtr = showtime_dotnetPINVOKE.Showtime_create_performer(name);
    SWIGTYPE_p_ZstPerformer ret = (cPtr == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_ZstPerformer(cPtr, false);
    return ret;
  }

  public static SWIGTYPE_p_ZstPerformer get_performer_by_name(string performer) {
    global::System.IntPtr cPtr = showtime_dotnetPINVOKE.Showtime_get_performer_by_name(performer);
    SWIGTYPE_p_ZstPerformer ret = (cPtr == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_ZstPerformer(cPtr, false);
    return ret;
  }

  public static PlugEvent pop_plug_event() {
    PlugEvent ret = new PlugEvent(showtime_dotnetPINVOKE.Showtime_pop_plug_event(), true);
    return ret;
  }

  public static int plug_event_queue_size() {
    int ret = showtime_dotnetPINVOKE.Showtime_plug_event_queue_size();
    return ret;
  }

  public static ZstIntPlug create_int_plug(ZstURI uri) {
    global::System.IntPtr cPtr = showtime_dotnetPINVOKE.Showtime_create_int_plug(ZstURI.getCPtr(uri));
    ZstIntPlug ret = (cPtr == global::System.IntPtr.Zero) ? null : new ZstIntPlug(cPtr, false);
    return ret;
  }

  public static void destroy_plug(ZstPlug plug) {
    showtime_dotnetPINVOKE.Showtime_destroy_plug(ZstPlug.getCPtr(plug));
  }

  public static SWIGTYPE_p_std__vectorT_ZstURI_t get_all_plug_addresses(string section, string instrument) {
    SWIGTYPE_p_std__vectorT_ZstURI_t ret = new SWIGTYPE_p_std__vectorT_ZstURI_t(showtime_dotnetPINVOKE.Showtime_get_all_plug_addresses__SWIG_0(section, instrument), true);
    return ret;
  }

  public static SWIGTYPE_p_std__vectorT_ZstURI_t get_all_plug_addresses(string section) {
    SWIGTYPE_p_std__vectorT_ZstURI_t ret = new SWIGTYPE_p_std__vectorT_ZstURI_t(showtime_dotnetPINVOKE.Showtime_get_all_plug_addresses__SWIG_1(section), true);
    return ret;
  }

  public static SWIGTYPE_p_std__vectorT_ZstURI_t get_all_plug_addresses() {
    SWIGTYPE_p_std__vectorT_ZstURI_t ret = new SWIGTYPE_p_std__vectorT_ZstURI_t(showtime_dotnetPINVOKE.Showtime_get_all_plug_addresses__SWIG_2(), true);
    return ret;
  }

  public static void connect_plugs(ZstURI a, ZstURI b) {
    showtime_dotnetPINVOKE.Showtime_connect_plugs(ZstURI.getCPtr(a), ZstURI.getCPtr(b));
  }

}