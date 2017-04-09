package md5aab1eae5a23215e87b7ffc7728e5b85b;


public class LocalBinder
	extends android.os.Binder
	implements
		mono.android.IGCUserPeer
{
/** @hide */
	public static final String __md_methods;
	static {
		__md_methods = 
			"";
		mono.android.Runtime.register ("Main.LocalBinder, Main, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null", LocalBinder.class, __md_methods);
	}


	public LocalBinder () throws java.lang.Throwable
	{
		super ();
		if (getClass () == LocalBinder.class)
			mono.android.TypeManager.Activate ("Main.LocalBinder, Main, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null", "", this, new java.lang.Object[] {  });
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
