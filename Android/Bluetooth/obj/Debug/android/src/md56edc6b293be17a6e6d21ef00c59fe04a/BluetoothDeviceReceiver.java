package md56edc6b293be17a6e6d21ef00c59fe04a;


public class BluetoothDeviceReceiver
	extends android.content.BroadcastReceiver
	implements
		mono.android.IGCUserPeer
{
/** @hide */
	public static final String __md_methods;
	static {
		__md_methods = 
			"n_onReceive:(Landroid/content/Context;Landroid/content/Intent;)V:GetOnReceive_Landroid_content_Context_Landroid_content_Intent_Handler\n" +
			"";
		mono.android.Runtime.register ("Bluetooth.BluetoothDeviceReceiver, Bluetooth, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null", BluetoothDeviceReceiver.class, __md_methods);
	}


	public BluetoothDeviceReceiver () throws java.lang.Throwable
	{
		super ();
		if (getClass () == BluetoothDeviceReceiver.class)
			mono.android.TypeManager.Activate ("Bluetooth.BluetoothDeviceReceiver, Bluetooth, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null", "", this, new java.lang.Object[] {  });
	}


	public void onReceive (android.content.Context p0, android.content.Intent p1)
	{
		n_onReceive (p0, p1);
	}

	private native void n_onReceive (android.content.Context p0, android.content.Intent p1);

	private java.util.ArrayList refList;
	public void monodroidAddReference (java.lang.Object obj)
	{
		if (refList == null)
			refList = new java.util.ArrayList ();
		refList.add (obj);
	}

	public void monodroidClearReferences ()
	{
		if (refList != null)
			refList.clear ();
	}
}
