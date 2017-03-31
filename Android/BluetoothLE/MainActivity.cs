using Android.App;
using Android.Bluetooth;
using Android.Bluetooth.LE;
using Android.Content;
using Android.OS;
using Android.Views;
using Android.Widget;
using Java.Lang;
using Java.Util;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Timers;

namespace BluetoothLE
{
    public static class Constants
    {
        public const int REQUEST_ENABLE_BT = 1;

        public const string ACTION_GATT_CONNECTED = 
            "com.example.bluetooth.le.ACTION_GATT_CONNECTED";
        public const string ACTION_GATT_DISCONNECTED = 
            "com.example.bluetooth.le.ACTION_GATT_DISCONNECTED";
        public const string ACTION_GATT_SERVICES_DISCOVERED =
            "com.example.bluetooth.le.ACTION_GATT_SERVICES_DISCOVERED";
        public const string ACTION_DATA_AVAILABLE =
            "com.example.bluetooth.le.ACTION_DATA_AVAILABLE";
        public const string EXTRA_DATA =
            "com.example.bluetooth.le.EXTRA_DATA";

        public const int STATE_DISCONNECTED = 0;
        public const int STATE_CONNECTING = 1;
        public const int STATE_CONNECTED = 2;
        public const int STATE_DISCONNECTING = 3;
    }

    [Activity(Label = "Bluetooth LE Testing", MainLauncher = true, Icon = "@drawable/icon")]
    public class MainActivity : Activity
    {
        private LinearLayout layout;
        public TextView debugText;

        private BluetoothAdapter mBluetoothAdapter;
        public static BluetoothManager mBluetoothManager;

        protected override void OnCreate(Bundle bundle)
        {
            base.OnCreate(bundle);

            layout = new LinearLayout(this);
            layout.Orientation = Orientation.Vertical;
            layout.SetGravity(Android.Views.GravityFlags.CenterHorizontal);
            SetContentView(layout);

            debugText = new TextView(this);
            layout.AddView(debugText);
            debugText.Text += "Debugging...\n";

            #region Bluetooth Setup

            // Initializes Bluetooth adapter.
            mBluetoothManager = (BluetoothManager) GetSystemService(Context.BluetoothService);
            mBluetoothAdapter = mBluetoothManager.Adapter;

            #region BLE Setup

            // Ensures Bluetooth is available on the device and it is enabled. If not,
            // displays a dialog requesting user permission to enable Bluetooth.

            if (mBluetoothAdapter == null || !mBluetoothAdapter.IsEnabled)
            {
                debugText.Text += "Bluetooth is not enabled on device.\n";
                debugText.Text += "Requesting permission to enable Bluetooth on device...\n";

                Intent enableBtIntent = new Intent(BluetoothAdapter.ActionRequestEnable);
                StartActivityForResult(enableBtIntent, Constants.REQUEST_ENABLE_BT);
            }
            else if (mBluetoothAdapter.IsEnabled)
            {
                debugText.Text += "Bluetooth is enabled on device.\n";
            }

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

            // BLE Discovery method has various missing classes/functions, so use the regular
            // Bluetooth Discovery implementation.

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
                    
                    debugText.Text += "Cancelling discovery...\n";
                    mBluetoothAdapter.CancelDiscovery();

                    
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

        public void ConnectLE(BluetoothDevice device)
        {
            //BluetoothGattCallback mGattCallback = new BluetoothGattCallback();
            //device.ConnectGatt(this, false, mGattCallback);
        }
    }

    public class BluetoothLeService : Service
    {
        // private static string TAG = BluetoothLeService.class.getSimpleName();

        private BluetoothManager mBluetoothManager;
        private BluetoothAdapter mBluetoothAdapter;
        private string mBluetoothDeviceAddress;
        private BluetoothGatt mBluetoothGatt;
        private int mConnectionState = Constants.STATE_DISCONNECTED;

        // public static UUID UUID_HEART_RATE_MEASUREMENT = UUID.FromString(SampleGattAttributes.HEART_RATE_MEASUREMENT);

        private GattCallback mGattCallback = new GattCallback();
    }

    public class GattCallback : BluetoothGattCallback
    {
        public int mConnetionState { get; set; }

        public override void OnConnectionStateChange(BluetoothGatt gatt, GattStatus status, ProfileState newState)
        {
            base.OnConnectionStateChange(gatt, status, newState);

            string intentAction;

            if (newState == ProfileState.Connected)
            {
                intentAction = Constants.ACTION_GATT_CONNECTED;
                mConnetionState = Constants.STATE_CONNECTED;
                broadcastUpdate(intentAction);
                // Connected to GATT server.
                // Attempting to start service discovery: mBluetoothGatt.discoverServices()
            }
            else if (newState == ProfileState.Disconnected)
            {
                intentAction = Constants.ACTION_GATT_DISCONNECTED;
                mConnetionState = Constants.STATE_CONNECTED;
                // Disconnected from GATT server.
                broadcastUpdate(intentAction);
            }
        }

        public override void OnServicesDiscovered(BluetoothGatt gatt, GattStatus status)
        {
            base.OnServicesDiscovered(gatt, status);

            if (status == GattStatus.Success)
            {
                broadcastUpdate(Constants.ACTION_GATT_SERVICES_DISCOVERED);
            }
            else
            {
                // onServicesDiscovered received: status
            }
        }

        public override void OnCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, GattStatus status)
        {
            base.OnCharacteristicRead(gatt, characteristic, status);

            if (status == GattStatus.Success)
            {
                broadcastUpdate(Constants.ACTION_DATA_AVAILABLE, characteristic);
            }
        }

        private void broadcastUpdate(string action)
        {
            Intent intent = new Intent(action);
            //sendBroadcast(intent);
        }

        private void broadcastUpdate(string action, BluetoothGattCharacteristic characteristic)
        {
            Intent intent = new Intent(action);

            // Special handling goes here.
        }
    }
}

