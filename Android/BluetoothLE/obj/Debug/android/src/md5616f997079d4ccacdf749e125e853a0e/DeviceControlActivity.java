package md5616f997079d4ccacdf749e125e853a0e;


public class DeviceControlActivity
	extends android.app.Activity
	implements
		mono.android.IGCUserPeer
{
/** @hide */
	public static final String __md_methods;
	static {
		__md_methods = 
			"";
		mono.android.Runtime.register ("BluetoothLE.DeviceControlActivity, BluetoothLE, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null", DeviceControlActivity.class, __md_methods);
	}


	public DeviceControlActivity () throws java.lang.Throwable
	{
		super ();
		if (getClass () == DeviceControlActivity.class)
			mono.android.TypeManager.Activate ("BluetoothLE.DeviceControlActivity, BluetoothLE, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null", "", this, new java.lang.Object[] {  });
	}

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
