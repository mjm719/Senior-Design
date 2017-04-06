﻿using Android;
using Android.App;
using Android.Bluetooth;
using Android.Bluetooth.LE;
using Android.Content;
using Android.Content.PM;
using Android.OS;
using Android.Views;
using Android.Widget;
using Java.Lang;
using Java.Util;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;
using System.Timers;

namespace BluetoothLE
{
    public static class Constants
    {
        public const int REQUEST_ENABLE_BT = 1;

        public const string LIST_NAME = "NAME";
        public const string LIST_UUID = "UUID";
    }

    public class Global
    {
        public static string DebugText = "";
    }

    [Activity(Label = "Bluetooth LE Testing", MainLauncher = true, Icon = "@drawable/icon")]
    public class MainActivity : Activity
    {
        private LinearLayout layout;
        public TextView debug;

        private BluetoothAdapter mBluetoothAdapter;
        public static BluetoothManager mBluetoothManager;

        System.Threading.Timer refresh;

        protected override async void OnCreate(Bundle bundle)
        {
            base.OnCreate(bundle);

            layout = new LinearLayout(this);
            layout.Orientation = Orientation.Vertical;
            layout.SetGravity(Android.Views.GravityFlags.CenterHorizontal);
            SetContentView(layout);

            debug = new TextView(this);
            debug.TextSize = 8;
            layout.AddView(debug);
            Global.DebugText += "Debugging...\n";

            // Start refresh timer immediately, invoke callback every 10 ms.
            // http://stackoverflow.com/questions/13019433/calling-method-on-every-x-minutes
            refresh = new System.Threading.Timer(x => RefreshView(), null, 0, 10);

            #region Bluetooth Setup

            // Initializes Bluetooth adapter.
            mBluetoothManager = (BluetoothManager) GetSystemService(Context.BluetoothService);
            mBluetoothAdapter = mBluetoothManager.Adapter;

            #region Permissions

            // Request permissions, if not previously granted.
            await TryGetLocationAsync();

            #endregion

            #region BLE Setup

            // Ensures Bluetooth is available on the device and it is enabled. If not,
            // displays a dialog requesting user permission to enable Bluetooth.

            if (mBluetoothAdapter == null || !mBluetoothAdapter.IsEnabled)
            {
                Global.DebugText += "Bluetooth is not enabled on device.\n";
                Global.DebugText += "Requesting permission to enable Bluetooth on device...\n";

                Intent enableBtIntent = new Intent(BluetoothAdapter.ActionRequestEnable);
                StartActivityForResult(enableBtIntent, Constants.REQUEST_ENABLE_BT);
            }
            else if (mBluetoothAdapter.IsEnabled)
            {
                Global.DebugText += "Bluetooth is enabled on device.\n";
            }

            #endregion

            #endregion

            #region Load Paired Devices

            Global.DebugText += "Loading paired devices...\n";

            ICollection<BluetoothDevice> pairedDevices = mBluetoothAdapter.BondedDevices;

            if (pairedDevices.Count > 0)
            {
                // There are pre-existing paired devices.
                // Get the name and address of each device.

                foreach (BluetoothDevice device in pairedDevices)
                {
                    string deviceName = device.Name;
                    string deviceAddress = device.Address;

                    Global.DebugText = Global.DebugText + deviceName + " (" + deviceAddress + ")\n";
                }
            }
            else
            {
                Global.DebugText += "No paired devices were found.\n";
            }

            #endregion

            #region Find Devices

            // BLE Discovery method has various missing classes/functions, so use the regular
            // Bluetooth Discovery implementation.

            BluetoothDeviceReceiver mReceiver = new BluetoothDeviceReceiver();
            mReceiver.mBluetoothAdapter = mBluetoothAdapter;

            IntentFilter filter_ActionFound = new IntentFilter(BluetoothDevice.ActionFound);
            IntentFilter filter_ActionDiscoveryStarted = new IntentFilter(BluetoothAdapter.ActionDiscoveryStarted);
            IntentFilter filter_ActionDiscoveryFinished = new IntentFilter(BluetoothAdapter.ActionDiscoveryFinished);

            RegisterReceiver(mReceiver, filter_ActionFound);
            RegisterReceiver(mReceiver, filter_ActionDiscoveryStarted);
            RegisterReceiver(mReceiver, filter_ActionDiscoveryFinished);

            // Start searching for devices.
            Global.DebugText += "Searching for discoverable devices...\n";
            mBluetoothAdapter.StartDiscovery();

            #endregion
        }

        async Task TryGetLocationAsync()
        {
            int Sdk = (int)Build.VERSION.SdkInt;

            if (Sdk < 23)
            {
                Global.DebugText += "Sdk must be at least 23.\n";
                return;
            }
            else
            {
                Global.DebugText += "Obtaining permissions...\n";
            }

            await GetLocationPermissionAsnyc();
        }

        readonly string[] PermissionsLocation =
        {
            Manifest.Permission.AccessCoarseLocation,
            Manifest.Permission.AccessFineLocation
        };

        const int RequestLocationId = 0;

        async Task GetLocationPermissionAsnyc()
        {
            const string permission = Manifest.Permission.AccessFineLocation;

            if (CheckSelfPermission(permission) == (int)Permission.Granted)
            {
                Global.DebugText += "All permissions have been previously granted.\n";
                return;
            }

            Global.DebugText += "Requesting permissions...\n";
            RequestPermissions(PermissionsLocation, RequestLocationId);

            await GetLocationPermissionAsnyc();
        }

        private void RefreshView()
        {
            // Refresh Debug Text.
            this.RunOnUiThread((() => debug.Text = Global.DebugText));
        }
    }

    public class BluetoothDeviceReceiver : BroadcastReceiver
    {
        public BluetoothAdapter mBluetoothAdapter { get; set; }
        public BluetoothGatt mBluetoothGatt;

        private string btAddress = "CC:78:AB:83:3C:06";
        private bool found = false;

        private GattCallback mGattCallback = new GattCallback();

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

                    Global.DebugText += "Device was found.\n";
                    Global.DebugText = Global.DebugText + deviceName + " (" + deviceAddress + ")\n";

                    Global.DebugText += "Cancelling discovery...\n";
                    mBluetoothAdapter.CancelDiscovery();

                    mBluetoothGatt = device.ConnectGatt(context, false, mGattCallback);

                    mGattCallback.mBluetoothAdapter = mBluetoothAdapter;
                    mGattCallback.mBluetoothDeviceAddress = deviceAddress;
                    mGattCallback.mBluetoothGatt = mBluetoothGatt;
                }
            }

            if (action == BluetoothAdapter.ActionDiscoveryStarted)
            {
                Global.DebugText += "Discovery started...\n";
            }

            if (action == BluetoothAdapter.ActionDiscoveryFinished)
            {
                Global.DebugText += "Discovery finished.\n";

                if (!found)
                {
                    Global.DebugText += "Device was not found.\n";
                }
            }
        }
    }

    public class BluetoothLeService : Service
    {
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

        private IBinder mBinder = new LocalBinder();

        public override IBinder OnBind(Intent intent)
        {
            return mBinder;
        }
    }

    public class LocalBinder : Android.OS.Binder
    {
        BluetoothLeService GetService()
        {
            return (BluetoothLeService)BluetoothLeService.BluetoothService;
        }
    }

    // Various callback methods defined by the BLE API.
    public class GattCallback : BluetoothGattCallback
    {
        public BluetoothManager mBluetoothManager { get; set; }
        public BluetoothAdapter mBluetoothAdapter { get; set; }
        public string mBluetoothDeviceAddress { get; set; }
        public BluetoothGatt mBluetoothGatt { get; set; }

        public int mConnectionState = BluetoothLeService.STATE_DISCONNECTED;

        public DeviceControlActivity mDeviceControlActivity = new DeviceControlActivity();

        public override void OnConnectionStateChange(BluetoothGatt gatt, GattStatus status, ProfileState newState)
        {
            base.OnConnectionStateChange(gatt, status, newState);

            string intentAction;

            if (newState == ProfileState.Connected)
            {
                intentAction = BluetoothLeService.ACTION_GATT_CONNECTED;
                mConnectionState = BluetoothLeService.STATE_CONNECTED;
                broadcastUpdate(intentAction);
                // Connected to GATT server.
                Global.DebugText += "Connected to GATT server.\n";
                // Attempting to start service discovery...
                Global.DebugText += "Attempting to start servce discovery...\n";

                try
                {
                    mBluetoothGatt.DiscoverServices();
                }
                catch
                {
                    Global.DebugText += "Remote service discovery could not be started.\n";
                }
            }
            else if (newState == ProfileState.Disconnected)
            {
                intentAction = BluetoothLeService.ACTION_GATT_DISCONNECTED;
                mConnectionState = BluetoothLeService.STATE_CONNECTED;
                // Disconnected from GATT server.
                Global.DebugText += "Disconnected from GATT server.\n";
                broadcastUpdate(intentAction);
            }
        }

        // New services discovered.
        public override void OnServicesDiscovered(BluetoothGatt gatt, GattStatus status)
        {
            base.OnServicesDiscovered(gatt, status);

            if (status == GattStatus.Success)
            {
                Global.DebugText += "Service discovery successful.\n";
                broadcastUpdate(BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED);

                #region IN DEVELOPMENT

                mDeviceControlActivity.DisplayGattServices(gatt.Services);

                BluetoothGattCharacteristic characteristic = mDeviceControlActivity.mGattCharacteristics[5][0];

                //bool read = mBluetoothGatt.ReadCharacteristic(characteristic);
                bool write = mBluetoothGatt.WriteCharacteristic(characteristic);

                //mBluetoothGatt.SetCharacteristicNotification(characteristic, true);

                //BluetoothGattDescriptor descriptor = characteristic.GetDescriptor(UUID.FromString(SampleGattAttributes.CLIENT_CHARACTERISTIC_CONFIG));
                //byte[] notificationValue = new byte[32];
                //BluetoothGattDescriptor.EnableIndicationValue.CopyTo(notificationValue, 0);
                //descriptor.SetValue(notificationValue);
                //mBluetoothGatt.WriteDescriptor(descriptor);

                Thread.Sleep(1000);

                ;
                
                #endregion
            }
            else
            {
                Global.DebugText += "Service discovery was not successful.\n";
                // onServicesDiscovered received.
            }
        }

        // Result of a characteristic read operation.
        public override void OnCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, GattStatus status)
        {
            base.OnCharacteristicRead(gatt, characteristic, status);

            if (status == GattStatus.Success)
            {
                broadcastUpdate(BluetoothLeService.ACTION_DATA_AVAILABLE, characteristic);
            }
        }

        private void broadcastUpdate(string action)
        {
            Intent intent = new Intent(action);

            // Send broadcast.
            ;
        }

        private void broadcastUpdate(string action, BluetoothGattCharacteristic characteristic)
        {
            Intent intent = new Intent(action);

            // Special handling goes here.
            ;

            // Send broadcast.
            ;
        }
    }

    public class mGattUpdateReceiver : BluetoothDeviceReceiver
    {
        public override void OnReceive(Context context, Intent intent)
        {
            base.OnReceive(context, intent);

            string action = intent.Action;

            if (BluetoothLeService.ACTION_GATT_CONNECTED.Equals(action))
            {
                // mConnected = true;
                ;
                // updateConnectionState(R.string.connected);
                ;
                // invalidateOptionsMenu();
                ;
            }
            else if (BluetoothLeService.ACTION_GATT_DISCONNECTED.Equals(action))
            {
                // mConnected = false;
                ;
                // updateConnectionState(R.string.disconnected);
                ;
                // invalidateOptionsMenu();
                ;
                // clearUI();
                ;
            }
            else if (BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED.Equals(action))
            {
                // displayGattServices(mBluetoothLeService.getSupportedGattServices());
                ;
            }
            else if (BluetoothLeService.ACTION_DATA_AVAILABLE.Equals(action))
            {
                // displayData(intent.getStringExtra(BluetoothService.EXTRA_DATA));
                ;
            }
        }
    }

    public class SampleGattAttributes
    {
        public static string CLIENT_CHARACTERISTIC_CONFIG = "00002902-0000-1000-8000-00805f9b34fb";

        private static Dictionary<string, string> Attributes = new Dictionary<string, string>()
        {
			// Sample Services.
            {   "0000180a-0000-1000-8000-00805f9b34fb", "Device Information Service"    },
            {   "f0001110-0451-4000-b000-000000000000", "LED Service"                   },
            {   "f0001111-0451-4000-b000-000000000000", "LED0 State"                    },
            {   "f0001112-0451-4000-b000-000000000000", "LED1 State"                    },
            {   "f0001120-0451-4000-b000-000000000000", "Button Service"                },
            {   "f0001121-0451-4000-b000-000000000000", "Button0 State"                 },
            {   "f0001122-0451-4000-b000-000000000000", "Button1 State"                 },
            {   "f0001130-0451-4000-b000-000000000000", "Data Service"                  },
            {   "f0001131-0451-4000-b000-000000000000", "String char"                   },
            {   "f0001132-0451-4000-b000-000000000000", "Stream char"                   },

			// Sample Characteristics.
            {   "00002a29-0000-1000-8000-00805f9b34fb", "Manufacturer Name String"      },
        };

        public static string Lookup(string key, string defaultName)
        {
            string name = defaultName;

            try
            {
                name = Attributes[key];
            }
            catch { }

            Thread.Sleep(100);
            Global.DebugText = Global.DebugText + key + " (" + name + ")\n";

            return name;
        }
    }

    public class DeviceControlActivity : Activity
    {
        public List<IList<BluetoothGattCharacteristic>> mGattCharacteristics;

        public void DisplayGattServices(IList<BluetoothGattService> gattServices)
        {
            if (gattServices == null)
            {
                return;
            }

            string uuid = null;
            IList<HashMap> gattServiceData = new List<HashMap>();
            IList<IList<HashMap>> gattCharacteristicData = new List<IList<HashMap>>();
            mGattCharacteristics = new List<IList<BluetoothGattCharacteristic>>();

            // Loops through available GATT Services.
            foreach (BluetoothGattService gattService in gattServices)
            {
                HashMap currentServiceData = new HashMap();

                string unknownServiceString = "Unknown Service";

                uuid = gattService.Uuid.ToString();

                currentServiceData.Put(Constants.LIST_NAME, SampleGattAttributes.Lookup(uuid, unknownServiceString));
                currentServiceData.Put(Constants.LIST_UUID, uuid);

                gattServiceData.Add(currentServiceData);

                IList<HashMap> gattCharacteristicGroupData = new List<HashMap>();
                IList<BluetoothGattCharacteristic> gattCharacteristics = gattService.Characteristics;
                IList<BluetoothGattCharacteristic> charas = new List<BluetoothGattCharacteristic>();

                // Loops through available characteristics.
                foreach (BluetoothGattCharacteristic gattCharacteristic in gattCharacteristics)
                {
                    charas.Add(gattCharacteristic);

                    HashMap currentCharaData = new HashMap();

                    string unknownCharaString = "Unknown Characteristic";

                    uuid = gattCharacteristic.Uuid.ToString();
                    
                    currentCharaData.Put(Constants.LIST_NAME, SampleGattAttributes.Lookup(uuid, unknownCharaString));
                    currentCharaData.Put(Constants.LIST_UUID, uuid);

                    gattCharacteristicGroupData.Add(currentCharaData);
                }

                mGattCharacteristics.Add(charas);
                gattCharacteristicData.Add(gattCharacteristicGroupData);
            }
        }
    }
}

