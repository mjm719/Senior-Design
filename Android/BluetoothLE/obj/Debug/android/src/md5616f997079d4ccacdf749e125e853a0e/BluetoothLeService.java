package md5616f997079d4ccacdf749e125e853a0e;


public class BluetoothLeService
	extends android.app.Service
	implements
		mono.android.IGCUserPeer
{
/** @hide */
	public static final String __md_methods;
	static {
		__md_methods = 
			"n_onBind:(Landroid/content/Intent;)Landroid/os/IBinder;:GetOnBind_Landroid_content_Intent_Handler\n" +
			"";
		mono.android.Runtime.register ("BluetoothLE.BluetoothLeService, BluetoothLE, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null", BluetoothLeService.class, __md_methods);
	}


	public BluetoothLeService () throws java.lang.Throwable
	{
		super ();
		if (getClass () == BluetoothLeService.class)
			mono.android.TypeManager.Activate ("BluetoothLE.BluetoothLeService, BluetoothLE, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null", "", this, new java.lang.Object[] {  });
	}


	public android.os.IBinder onBind (android.content.Intent p0)
	{
		return n_onBind (p0);
	}

	private native android.os.IBinder n_onBind (android.content.Intent p0);

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
