using Android.App;
using Android.Bluetooth;
using Android.Bluetooth.LE;
using Android.Content;
using Android.Widget;
using Android.OS;
using Java.Lang;
using Java.Util;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Timers;

namespace Bluetooth
{
    public static class Constants
    {
        #region TI UUID Information

        // All the custom serivice identifiers (UUID's) are in the form 
        // F000XXXX-0451-4000-B000-000000000000 where each service has
        // a different 16-bit XXXX-part identifier, and the rest of the UUID is the
        // Texas Instruments 128-bit UUID namespace used for demo purposes.
        //
        // The services can be identified from the 16-bit part of the UUID as
        // follows:
        //
        //      0x1110 - LED Service
        //          Service declaration
        //      0x1111 - LED0 State
        //          Read state or write 01 or 00.
        //      0x1112 - LED1 State
        //          Read state or write 01 or 00.
        //      0x1120 - Button Service
        //          Service Declaration
        //      0x1121 - BUTTON0 State
        //          Read state or subscribe to notifications
        //      0x1122 - BUTTON1 State
        //          Read state or subscribe to notifications
        //      0x1130 - Data Service
        //          Service Declaration
        //      0x1131 - String char*
        //          Read/Write a long string
        //      0x1132 - Stream char
        //          Send or recieve WriteNoRsp/Notification

        #endregion

        private static string UUID_service = "1131";
        private static string UUID_str = "F000" + UUID_service + "-0451-4000-B000-000000000000";
        public static UUID UUID = UUID.FromString(UUID_str);
        public static string NAME = "Android Phone";
        public const int ACTION_FOUND = 1;
    }

    [Activity(Label = "Bluetooth Testing", MainLauncher = true, Icon = "@drawable/icon")]
    public class MainActivity : Activity
    {
        #region Transmission Data Information

        // B0: Battery Level
        //
        // B1: Pressure Data 1
        //
        // B2: Pressure Data 2
        //      [5:0]: Data bits
        //      [7:6]: Status bits
        //
        // B3: Status Flags
        //      [0]: Pressure/Sensor Fault
        //      [1]: Battery Warning
        //      [2]: Battery Critical
        //      [3]: Status Bit 0
        //      [4]: Status Bit 1
        //      [5]: C02
        //      [6]: Flat Tire Warning
        //      [7]: unimplemented
        
        #endregion

        // static int data = 0x0000;

        private LinearLayout layout;
        public TextView debugText;

        BluetoothAdapter mBluetoothAdapter;

        protected override void OnCreate(Bundle bundle)
        {
            base.OnCreate(bundle);

            layout = new LinearLayout(this);
            layout.Orientation = Orientation.Vertical;
            layout.SetGravity(Android.Views.GravityFlags.CenterHorizontal);
            SetContentView(layout);

            debugText = new TextView(this);
            layout.AddView(debugText);
            debugText.Text = "Debugging...\n";

            #region Bluetooth Setup

            #region Check For Bluetooth Support

            // Check if device supports Bluetooth.
            mBluetoothAdapter = BluetoothAdapter.DefaultAdapter;
            if (mBluetoothAdapter == null)
            {
                // Device does not support Bluetooth.
                debugText.Text += "Device does not support Bluetooth.\n";
            }
            else
            {
                debugText.Text += "Device supports Bluetooth.\n";
            }

            #endregion

            #region Enable Discoverability

            // Note: enabling discoverablity automatically enables Bluetooth if not previously enabled.

            debugText.Text += "Requesting permission to make device discoverable... \n";

            Intent discoverabileIntent = new Intent(BluetoothAdapter.ActionRequestDiscoverable);
            discoverabileIntent.PutExtra(BluetoothAdapter.ExtraDiscoverableDuration, 300);
            StartActivity(discoverabileIntent);

            #endregion

            #endregion

            #region Load Paired Devices

            debugText.Text += "Loading paired devices...\n";

            ICollection<BluetoothDevice> pairedDevices = mBluetoothAdapter.BondedDevices;

            if (pairedDevices.Count > 0)
            {
                // There are pre-existing paired devices.
                // Get the name and address of each device.

                foreach (BluetoothDevice device in pairedDevices)
                {
                    string deviceName = device.Name;
                    string deviceAddress = device.Address;

                    debugText.Text = debugText.Text + deviceName + " (" + deviceAddress + ")\n";
                }
            }
            else
            {
                debugText.Text += "No paired devices were found.\n";
            }

            #endregion

            #region Find Devices

            BluetoothDeviceReceiver mReceiver = new BluetoothDeviceReceiver();
            mReceiver.debugText = debugText;
            mReceiver.mBluetoothAdapter = mBluetoothAdapter;

            IntentFilter filter_ActionFound = new IntentFilter(BluetoothDevice.ActionFound);
            IntentFilter filter_ActionDiscoveryStarted = new IntentFilter(BluetoothAdapter.ActionDiscoveryStarted);
            IntentFilter filter_ActionDiscoveryFinished = new IntentFilter(BluetoothAdapter.ActionDiscoveryFinished);

            RegisterReceiver(mReceiver, filter_ActionFound);
            RegisterReceiver(mReceiver, filter_ActionDiscoveryStarted);
            RegisterReceiver(mReceiver, filter_ActionDiscoveryFinished);

            // Start searching for devices.
            debugText.Text += "Searching for discoverable devices...\n";
            mBluetoothAdapter.StartDiscovery();

            #endregion
        }
    }

    public class BluetoothDeviceReceiver : BroadcastReceiver
    {
        public TextView debugText { get; set; }
        public BluetoothAdapter mBluetoothAdapter { get; set; }

        private string btAddress = "CC:78:AB:83:3C:06";
        private bool found = false;

        public override void OnReceive(Context context, Intent intent)
        {
            string action = intent.Action;

            if (action == BluetoothDevice.ActionFound)
            {
                // Discovery has found a device. Get the BluetoothDevice object and its info from the Intent.
                BluetoothDevice device = (BluetoothDevice)intent.GetParcelableExtra(BluetoothDevice.ExtraDevice);

                string deviceName = device.Name;
                string deviceAddress = device.Address;

                if (deviceAddress == btAddress)
                {
                    found = true;

                    debugText.Text += "Device was found.\n";
                    debugText.Text = debugText.Text + deviceName + " (" + deviceAddress + ")\n";

                    #region Connect Device (Server)

                    /*
                    debugText.Text += "Attempting to connect with device as a server...\n";
                    AcceptThread AcceptThread = new AcceptThread(mBluetoothAdapter);
                    AcceptThread.debugText = debugText;
                    AcceptThread.Accept();
                    */

                    #endregion

                    #region Connect Device (Client)

                    debugText.Text += "Attempting to connect with device as a client...\n";
                    ConnectThread ConnectThread = new ConnectThread(device);
                    ConnectThread.mBluetoothAdapter = mBluetoothAdapter;
                    ConnectThread.debugText = debugText;
                    ConnectThread.Connect();

                    #endregion
                }
            }

            if (action == BluetoothAdapter.ActionDiscoveryStarted)
            {
                debugText.Text += "Discovery started...\n";
            }

            if (action == BluetoothAdapter.ActionDiscoveryFinished)
            {
                debugText.Text += "Discovery finished.\n";

                if (!found)
                {
                    debugText.Text += "Device was not found.\n";
                }
            }
        }
    }

    public class AcceptThread : Thread
    {
        public TextView debugText { get; set; }

        private static BluetoothServerSocket mmServerSocket;

        public AcceptThread(BluetoothAdapter mBluetoothAdapter)
        {
            // Use a temporary object that is later assigned to mmServerSocket because mmServerSocket is static.
            BluetoothServerSocket tmp = null;

            try
            {
                // MY_UUID is the app's UUID string, also used by the client code.
                // tmp = mBluetoothAdapter.ListenUsingRfcommWithServiceRecord(Constants.NAME, Constants.UUID);
            }
            catch { }

            mmServerSocket = tmp;
        }

        public void Accept()
        {
            BluetoothSocket socket = null;

            // Keep listening until exception occurs or a socket is returned.
            debugText.Text += "Listening for socket...\n";

            ;

            while (true)
            {
                ;

                try
                {
                    ;
                    
                    socket = mmServerSocket.Accept();
                }
                catch
                {
                    debugText.Text += "Exception has occured.\n";
                    break;
                }

                if (socket != null)
                {
                    // A connection was accepted. Perform work associated with the connection in a separate thread.
                    debugText.Text += "Connection was accepted.\n";

                    // manageMyConnectedSocket(mmSocket);

                    debugText.Text += "Closing the socket...\n";
                    mmServerSocket.Close();

                    break;
                }
            }
        }

        // Closes the connect socket and causes the thread to finish.
        public void Cancel()
        {
            try
            {
                debugText.Text += "Closing the connect socket...\n";
                mmServerSocket.Close();
            }
            catch
            {
                debugText.Text += "Unable to close the connect socket.\n";
            }
        }
    }

    public class ConnectThread : Thread
    {
        public TextView debugText { get; set; }
        public BluetoothAdapter mBluetoothAdapter { get; set; }

        private static BluetoothSocket mmSocket;
        private static BluetoothDevice mmDevice;

        public ConnectThread(BluetoothDevice device)
        {
            // Use a temporary object that is later assigned to mmSocket because mmSocket is static. 
            BluetoothSocket tmp = null;
            mmDevice = device;

            try
            {
                // Get a BluetoothSocket to connect with the given BluetoothDevice.
                // MY_UUID is the app's UUID string, also used in the server code.
                tmp = device.CreateRfcommSocketToServiceRecord(Constants.UUID);
            }
            catch { };

            mmSocket = tmp;
        }

        public void Connect()
        {
            // Cancel discovery because it otherwise slows down the connection.
            debugText.Text += "Cancelling discovery...\n";
            mBluetoothAdapter.CancelDiscovery();

            try
            {
                // Connect to the remote device through the socket. This call blocks until it succeeds or throws an exception.
                debugText.Text += "Attempting to connect to device through the socket...\n";

                mmSocket.ConnectAsync();
            }
            catch (IOException e)
            {
                // Unable to connect; close the socket and return.
                debugText.Text += "Unable to connect.\n";

                try
                {
                    debugText.Text += "Closing the socket...\n";
                    mmSocket.Close();
                }
                catch
                {
                    debugText.Text += "Unable to close the socket.\n";
                }

                return;
            }

            // The connection attempt succeeded. Perform work associated with the connection in a separate thread.
            debugText.Text += "Connection succeeded.\n";

            // manageMyConnectedSocket(mmSocket);
        }

        // Closes the client socket and causes the thread to finish.
        public void Cancel()
        {
            try
            {
                debugText.Text += "Closing the socket...\n";
                mmSocket.Close();
            }
            catch
            {
                debugText.Text += "Unable to close the socket.\n";
            }
        }
    }
}