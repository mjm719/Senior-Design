using Android;
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
using System.Resources;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using Android.Runtime;

namespace Main
{
    public static class Constants
    {
        public const int PRESSURESTART = 35;
        public const int PRESSUREMIN = 1;
        public const int PRESSUREMAX = 150;
        public const int PRESSURESETMIN = 1;
        public const int PRESSURESETMAX = 70;
        public const int OUTPUTMIN = 1648;
        public const int OUTPUTMAX = 14745;
        public const int MAXPRESETS = 5;
        public const int PERIOD = 50;
        public const int READWAIT = 20;

        public const int REQUEST_ENABLE_BT = 1;

        public const string LIST_NAME = "NAME";
        public const string LIST_UUID = "UUID";
    }

    public static class Global
    {
        public static int currentPressure;
        public static int targetPressure;

        public static bool forceIdle;

        public static bool pressureFault;
        public static int status;
        public static bool batteryShutdown;

        public static LinearLayout layout;
        public static List<LinearLayout> Presets;
        public static List<Button> ActivateButtons;
        public static List<ImageButton> EditButtons;
        public static List<ImageButton> DeleteButtons;
        public static List<int> PresetVals;

        public static bool startRead = false;
        public static bool startWrite = false;

        public static bool isConnected = false;
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

            return name;
        }
    }

    [Activity(Label = "BPR", MainLauncher = true, Icon = "@drawable/icon")]
    public class MainActivity : Activity
    {
        static float widthInDp;
        static float heightInDp;

        public int PresetsCount = 0;
        public int CurrentIndex;
        public string CurrentName;
        public int CurrentValue;
        public bool NewPreset;

        LinearLayout loadingScreenLayout1;
        LinearLayout loadingScreenLayout2;
        TextView loadingTextView;
        ImageView loadingWheel;

        TextView currentPressureView;
        TextView targetPressureView;
        TextView statusView;
        Space vMargin1;
        Space vMargin2;
        ImageButton addButton;

        LinearLayout editPressureLayout1;
        LinearLayout editPressureLayout2;
        LinearLayout editPressureLayout3;
        LinearLayout editPressureLayout4;
        LinearLayout editPressureLayout5;
        ImageButton backButton;
        EditText editPressureName;
        EditText editPressureValue;
        TextView units;
        SeekBar seekBar;
        Button setButton;

        public LinearLayout layout;
        public Button readButton;
        public Button writeButton;
        public TextView debug;

        public BluetoothAdapter mBluetoothAdapter;
        public static BluetoothManager mBluetoothManager;
        public BluetoothDeviceReceiver mReceiver;

        System.Threading.Timer loadingScreenDots;
        System.Threading.Timer loadingScreenWheel;
        System.Threading.Timer refresh;
        System.Threading.Timer read;
        System.Threading.Timer write;

        protected override void OnCreate(Bundle bundle)
        {
            base.OnCreate(bundle);

            widthInDp = Resources.DisplayMetrics.WidthPixels * ((float)Resources.DisplayMetrics.DensityDpi / 160f);
            heightInDp = Resources.DisplayMetrics.HeightPixels * ((float)Resources.DisplayMetrics.DensityDpi / 160f);

            #region Bluetooth

            #region Bluetooth Setup

            // Initializes Bluetooth adapter.
            mBluetoothManager = (BluetoothManager)GetSystemService(Context.BluetoothService);
            mBluetoothAdapter = mBluetoothManager.Adapter;

            #region Permissions

            // Request permissions, if not previously granted.
            TryGetLocation();

            #endregion

            #region BLE Setup

            // Ensures Bluetooth is available on the device and it is enabled. If not,
            // displays a dialog requesting user permission to enable Bluetooth.

            if (mBluetoothAdapter == null || !mBluetoothAdapter.IsEnabled)
            {
                // Bluetooth is not enabled on device.
                // Requesting permission to enable Bluetooth on device...

                Intent enableBtIntent = new Intent(BluetoothAdapter.ActionRequestEnable);
                StartActivityForResult(enableBtIntent, Constants.REQUEST_ENABLE_BT);
            }
            else if (mBluetoothAdapter.IsEnabled)
            {
                // Bluetooth is enabled on device.
            }

            #endregion

            #endregion

            #region Load Paired Devices

            // Loading paired devices...

            ICollection<BluetoothDevice> pairedDevices = mBluetoothAdapter.BondedDevices;

            if (pairedDevices.Count > 0)
            {
                // There are pre-existing paired devices.
                // Get the name and address of each device.

                foreach (BluetoothDevice device in pairedDevices)
                {
                    string deviceName = device.Name;
                    string deviceAddress = device.Address;
                }
            }
            else
            {
                // No paired devices were found.
            }

            #endregion

            #region Find Devices

            // BLE Discovery method has various missing classes/functions, so use the regular
            // Bluetooth Discovery implementation.

            mReceiver = new BluetoothDeviceReceiver();
            mReceiver.mBluetoothAdapter = mBluetoothAdapter;

            IntentFilter filter_ActionFound = new IntentFilter(BluetoothDevice.ActionFound);
            IntentFilter filter_ActionDiscoveryStarted = new IntentFilter(BluetoothAdapter.ActionDiscoveryStarted);
            IntentFilter filter_ActionDiscoveryFinished = new IntentFilter(BluetoothAdapter.ActionDiscoveryFinished);

            RegisterReceiver(mReceiver, filter_ActionFound);
            RegisterReceiver(mReceiver, filter_ActionDiscoveryStarted);
            RegisterReceiver(mReceiver, filter_ActionDiscoveryFinished);

            // Start searching for devices.
            mBluetoothAdapter.StartDiscovery();

            #endregion

            #endregion

            Global.layout = new LinearLayout(this);
            Global.layout.Orientation = Orientation.Vertical;
            Global.layout.SetGravity(Android.Views.GravityFlags.CenterHorizontal);
            SetContentView(Global.layout);

            Global.currentPressure = Constants.PRESSURESTART;
            Global.targetPressure = Global.currentPressure;

            #region Loading Views

            loadingScreenLayout1 = new LinearLayout(this);
            Global.layout.AddView(loadingScreenLayout1);
            loadingScreenLayout1.Orientation = Orientation.Horizontal;
            loadingScreenLayout1.SetGravity(Android.Views.GravityFlags.Center);
            loadingScreenLayout1.SetPadding((int)(Resources.DisplayMetrics.WidthPixels * 0.04), (int)(Resources.DisplayMetrics.HeightPixels * 0.04), 0, 0);

            loadingTextView = new TextView(this);
            loadingScreenLayout1.AddView(loadingTextView);
            loadingTextView.Gravity = Android.Views.GravityFlags.Center;
            loadingTextView.Text = "Loading...";
            loadingTextView.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            loadingTextView.SetTextColor(Android.Graphics.Color.White);

            loadingScreenLayout2 = new LinearLayout(this);
            Global.layout.AddView(loadingScreenLayout2);
            loadingScreenLayout2.Orientation = Orientation.Horizontal;
            loadingScreenLayout2.SetGravity(Android.Views.GravityFlags.Center);
            loadingScreenLayout2.SetPadding((int)(Resources.DisplayMetrics.WidthPixels * 0.04), (int)(Resources.DisplayMetrics.HeightPixels * 0.04), 0, 0);

            loadingWheel = new ImageView(this);
            loadingScreenLayout2.AddView(loadingWheel);
            loadingWheel.SetBackgroundResource(Resources.GetIdentifier("icon_bike_tire_white", "drawable", PackageName));
            loadingWheel.LayoutParameters.Width = (int)(Resources.DisplayMetrics.HeightPixels * 0.2);
            loadingWheel.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.2);

            #endregion

            #region Main Views

            vMargin1 = new Space(this);
            Global.layout.AddView(vMargin1);
            vMargin1.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.025);

            #region Current Pressure

            currentPressureView = new TextView(this);
            Global.layout.AddView(currentPressureView);
            currentPressureView.Gravity = Android.Views.GravityFlags.Center;
            currentPressureView.Text = "Current Pressure: ???";
            currentPressureView.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            currentPressureView.SetTextColor(Android.Graphics.Color.White);

            #endregion

            #region Target Pressure

            targetPressureView = new TextView(this);
            Global.layout.AddView(targetPressureView);
            targetPressureView.Gravity = Android.Views.GravityFlags.Center;
            targetPressureView.Text = "Target Pressure: ???";
            targetPressureView.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            targetPressureView.SetTextColor(Android.Graphics.Color.White);

            #endregion

            #region Status

            statusView = new TextView(this);
            Global.layout.AddView(statusView);
            statusView.Gravity = Android.Views.GravityFlags.Center;
            statusView.Text = "Initializing...";
            statusView.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.015);
            statusView.SetTextColor(Android.Graphics.Color.White);

            #endregion

            vMargin2 = new Space(this);
            Global.layout.AddView(vMargin2);
            vMargin2.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);

            #region Add Button

            addButton = new ImageButton(this);
            Global.layout.AddView(addButton);
            addButton.SetBackgroundResource(Resources.GetIdentifier("icon_add", "drawable", PackageName));
            addButton.LayoutParameters.Width = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);
            addButton.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);
            addButton.Click += AddButtonClick;

            #endregion

            #region Presets

            Global.Presets = new List<LinearLayout>();
            Global.ActivateButtons = new List<Button>();
            Global.EditButtons = new List<ImageButton>();
            Global.DeleteButtons = new List<ImageButton>();
            Global.PresetVals = new List<int>();

            // Start out with 2 premade presets.

            AddPreset("Preset 1", Constants.PRESSURESETMIN);
            AddPreset("Preset 2", Constants.PRESSURESETMAX);

            #endregion

            #endregion

            #region Edit Views

            #region Back Button

            editPressureLayout1 = new LinearLayout(this);
            Global.layout.AddView(editPressureLayout1);
            editPressureLayout1.Orientation = Orientation.Horizontal;
            editPressureLayout1.SetGravity(Android.Views.GravityFlags.Left);
            editPressureLayout1.SetPadding((int)(Resources.DisplayMetrics.WidthPixels * 0.04), (int)(Resources.DisplayMetrics.HeightPixels * 0.04), 0, 0);

            backButton = new ImageButton(this);
            editPressureLayout1.AddView(backButton);
            backButton.SetBackgroundResource(Resources.GetIdentifier("icon_back", "drawable", PackageName));
            backButton.LayoutParameters.Width = (int)(Resources.DisplayMetrics.HeightPixels * 0.04);
            backButton.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.04);
            backButton.Click += BackButtonClick;

            #endregion

            #region Preset Name

            editPressureLayout2 = new LinearLayout(this);
            Global.layout.AddView(editPressureLayout2);
            editPressureLayout2.Orientation = Orientation.Horizontal;
            editPressureLayout2.SetGravity(Android.Views.GravityFlags.Center);
            editPressureLayout2.SetPadding(0, 0, 0, 0);

            editPressureName = new EditText(this);
            editPressureLayout2.AddView(editPressureName);
            editPressureName.Text = "???";
            editPressureName.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            editPressureName.SetTextColor(Android.Graphics.Color.White);
            editPressureName.TextChanged += EditName;

            #endregion

            #region Pressure Value

            editPressureLayout3 = new LinearLayout(this);
            Global.layout.AddView(editPressureLayout3);
            editPressureLayout3.Orientation = Orientation.Horizontal;
            editPressureLayout3.SetGravity(Android.Views.GravityFlags.Center);
            editPressureLayout3.SetPadding(0, 0, 0, 0);

            editPressureValue = new EditText(this);
            editPressureLayout3.AddView(editPressureValue);
            editPressureValue.Text = "???";
            editPressureValue.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            editPressureValue.SetTextColor(Android.Graphics.Color.White);
            editPressureValue.TextChanged += EditSeekBar;

            units = new TextView(this);
            editPressureLayout3.AddView(units);
            units.Text = " psi";
            units.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            units.SetTextColor(Android.Graphics.Color.White);

            #endregion

            #region Seek Bar

            editPressureLayout4 = new LinearLayout(this);
            Global.layout.AddView(editPressureLayout4);
            editPressureLayout4.Orientation = Orientation.Horizontal;
            editPressureLayout4.SetGravity(Android.Views.GravityFlags.Center);
            editPressureLayout4.SetPadding(0, 0, 0, 0);

            seekBar = new SeekBar(this);
            editPressureLayout4.AddView(seekBar);
            seekBar.LayoutParameters.Width = (int)(Resources.DisplayMetrics.WidthPixels * 0.8);
            seekBar.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);
            seekBar.ProgressChanged += EditPSI;

            #endregion

            #region Set Button

            editPressureLayout5 = new LinearLayout(this);
            Global.layout.AddView(editPressureLayout5);
            editPressureLayout5.Orientation = Orientation.Horizontal;
            editPressureLayout5.SetGravity(Android.Views.GravityFlags.Center);
            editPressureLayout5.SetPadding(0, 20, 0, 0);

            setButton = new Button(this);
            editPressureLayout5.AddView(setButton);
            setButton.Text = "Set";
            setButton.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0125);
            setButton.Gravity = Android.Views.GravityFlags.Center;
            setButton.LayoutParameters.Width = (int)(Resources.DisplayMetrics.WidthPixels * 0.4);
            setButton.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.075);
            setButton.SetAllCaps(false);
            setButton.SetTextColor(Android.Graphics.Color.Rgb(224, 247, 250)); // #E0F7FA
            setButton.SetBackgroundColor(Android.Graphics.Color.Rgb(0, 128, 0)); // #008000
            setButton.Click += SetButtonClick;

            #endregion

            #endregion

            LoadLoadingViews();

            #region Initialize Timers

            // http://stackoverflow.com/questions/13019433/calling-method-on-every-x-minutes

            // Loading Screen Timers.
            loadingScreenDots = new System.Threading.Timer(x => LoadingScreenDots(), null, 0, 250);
            loadingScreenWheel = new System.Threading.Timer(x => LoadingScreenWheel(), null, 0, 10);

            // Main Screen Timers.
            refresh = new System.Threading.Timer(x => RefreshView(), null, 0, 10);
            write = new System.Threading.Timer(x => Write(), null, 0, Constants.PERIOD);
            read = new System.Threading.Timer(x => Read(), null, Constants.PERIOD / 2, Constants.PERIOD);

            #endregion
        }

        public void TryGetLocation()
        {
            int Sdk = (int)Build.VERSION.SdkInt;

            if (Sdk < 23)
            {
                // Sdk must be at least 23.
                return;
            }
            else
            {
                // Obtaining permissions...
            }

            GetLocationPermission();
        }

        readonly string[] PermissionsLocation =
        {
            Manifest.Permission.AccessCoarseLocation,
            Manifest.Permission.AccessFineLocation
        };

        const int RequestLocationId = 0;

        public void GetLocationPermission()
        {
            const string permission = Manifest.Permission.AccessFineLocation;

            if (CheckSelfPermission(permission) == (int)Permission.Granted)
            {
                // All permissions have been previously granted.
                return;
            }

            // Requesting permissions...
            RequestPermissions(PermissionsLocation, RequestLocationId);
        }

        private void LoadingScreenDots()
        {
            if (loadingTextView == null)
            {
                return;
            }

            if (loadingTextView.Text == "Loading...")
            {
                this.RunOnUiThread((() => loadingTextView.Text = "Loading   "));
            }
            else if (loadingTextView.Text == "Loading   ")
            {
                this.RunOnUiThread((() => loadingTextView.Text = "Loading.  "));
            }
            else if (loadingTextView.Text == "Loading.  ")
            {
                this.RunOnUiThread((() => loadingTextView.Text = "Loading.. "));
            }
            else if (loadingTextView.Text == "Loading.. ")
            {
                this.RunOnUiThread((() => loadingTextView.Text = "Loading..."));
            }
        }

        private void LoadingScreenWheel()
        {
            if (loadingWheel == null)
            {
                return;
            }

            loadingWheel.Rotation += (float)1.5;
        }

        private void RefreshView()
        {
            // Refresh Current Pressure.
            this.RunOnUiThread((() => currentPressureView.Text = "Current Pressure: " + Global.currentPressure + " psi"));

            // Refresh Target Pressure.
            this.RunOnUiThread((() => targetPressureView.Text = "Target Pressure: " + Global.targetPressure + " psi"));

            // Refresh Status.
            if (Global.status == 0)
            {
                this.RunOnUiThread((() => statusView.Text = "Initializing..."));
            }

            if (Global.status == 1)
            {
                this.RunOnUiThread((() => statusView.Text = "Pressure Stabilized."));
            }

            if (Global.status == 2)
            {
                this.RunOnUiThread((() => statusView.Text = "Releasing..."));
            }

            if (Global.status == 3)
            {
                this.RunOnUiThread((() => statusView.Text = "Pressurizing..."));
            }

            try
            {
                for (int i = 0; i <= Global.Presets.Count; i++)
                {
                    Java.Lang.Object tag = "Activate Button " + (i + 1).ToString();
                    Button activateButton = (Button)Global.Presets[i].FindViewWithTag(tag);

                    if (Global.status == 2 || Global.status == 3)
                    {
                        this.RunOnUiThread((() => activateButton.SetBackgroundColor(Android.Graphics.Color.Rgb(128, 128, 128)))); // Gray
                    }
                    else
                    {
                        this.RunOnUiThread((() => activateButton.SetBackgroundColor(Android.Graphics.Color.Rgb(0, 128, 0)))); // #008000
                    }
                }
            }
            catch { };

            // Set Add Button Visibility.
            try
            {
                if (Global.Presets.Count >= Constants.MAXPRESETS)
                {
                    addButton.Visibility = Android.Views.ViewStates.Invisible;
                }

                if (Global.Presets.Count < Constants.MAXPRESETS)
                {
                    addButton.Visibility = Android.Views.ViewStates.Visible;
                }
            }
            catch { };
        }

        private void Read()
        {
            if (Global.startRead)
            {
                ConvertDataIn(mReceiver.Read());
            }
        }

        private void Write()
        {
            if (Global.startWrite)
            {
                // mReceiver.Write(ConvertDataOut(Global.targetPressure, Global.forceIdle));

                // On first Write, load Main Views.
                if (Global.startWrite && !Global.startRead)
                {
                    LoadMainViews();
                }

                // Stop writing, start reading.
                Global.startWrite = false;
                Global.startRead = true;
            }
        }

        private void ActivateButtonClick(object sender, System.EventArgs e)
        {
            if (Global.status == 2 || Global.status == 3)
            {
                // Disable button click if unstable.
                return;
            }

            Java.Lang.Object tag;
            Java.Lang.Object senderTag;

            for (int i = 0; i < Global.Presets.Count; i++)
            {
                tag = "Activate Button " + (i + 1).ToString();
                senderTag = (sender as Button).Tag;

                if (senderTag.ToString() == tag.ToString())
                {
                    Global.targetPressure = Global.PresetVals[i];
                }
            }
        }

        private void EditButtonClick(object sender, System.EventArgs e)
        {
            NewPreset = false;

            Java.Lang.Object tag;
            Java.Lang.Object senderTag;

            for (int i = 0; i < Global.Presets.Count; i++)
            {
                tag = "Edit Button " + (i + 1).ToString();
                senderTag = (sender as ImageButton).Tag.ToString();

                if (senderTag.ToString() == tag.ToString())
                {
                    EditPreset(i);
                }
            }
        }

        private void DeleteButtonClick(object sender, System.EventArgs e)
        {
            Java.Lang.Object tag;
            Java.Lang.Object senderTag;

            for (int i = 0; i < Global.Presets.Count; i++)
            {
                tag = "Delete Button " + (i + 1).ToString();
                senderTag = (sender as ImageButton).Tag;

                if (senderTag.ToString() == tag.ToString())
                {
                    // Delete preset.
                    DeletePreset(i);
                }
            }
        }

        private void AddButtonClick(object sender, System.EventArgs e)
        {
            NewPreset = true;

            string name = "Preset " + (PresetsCount + 1).ToString();
            int value = Constants.PRESSURESETMIN;

            LoadEditViews(name, value);
        }

        private void BackButtonClick(object sender, System.EventArgs e)
        {
            LoadMainViews();
        }

        private void SetButtonClick(object sender, System.EventArgs e)
        {
            // Load main menu.
            LoadMainViews();

            // If this is an Add operation, create a new preset with the new name/value.
            if (NewPreset)
            {
                AddPreset(CurrentName, CurrentValue);

                // Reload Main Views.
                LoadMainViews();
            }
            // If this is an Edit operation, assign new name/value to cooresponding preset.
            else
            {
                Button activateButton = new Button(this);
                activateButton = (Button)Global.Presets[CurrentIndex].FindViewWithTag("Activate Button " + (CurrentIndex + 1).ToString());
                activateButton.Text = CurrentName;

                TextView presetText = new TextView(this);
                presetText = (TextView)Global.Presets[CurrentIndex].FindViewWithTag("Preset Text " + (CurrentIndex + 1).ToString());
                presetText.Text = CurrentValue.ToString() + " psi";

                Global.PresetVals[CurrentIndex] = CurrentValue;
            }

            NewPreset = false;
        }

        private void EditPreset(int index)
        {
            CurrentIndex = index;

            // Extract name from preset at given index.
            Java.Lang.Object activateTag = "Activate Button " + (index + 1).ToString();
            Button activateButton = (Button)Global.Presets[index].FindViewWithTag(activateTag);
            string name = activateButton.Text;

            // Extract value from preset at given index.
            Java.Lang.Object presetTextTag = "Preset Text " + (index + 1).ToString();
            TextView presetValue = (TextView)Global.Presets[index].FindViewWithTag(presetTextTag);
            int value = System.Int32.Parse(presetValue.Text.Substring(0, presetValue.Text.Length - 4)); // Extract value from TextView.

            LoadEditViews(name, value);
        }

        private void EditName(object sender, System.EventArgs e)
        {
            CurrentName = editPressureName.Text;
        }

        private void EditPSI(object sender, System.EventArgs e)
        {
            // If sliding seek bar, update pressure text.
            if (seekBar.IsInTouchMode)
            {
                editPressureValue.Text = ConvertToPSIRange(seekBar.Progress).ToString();

                CurrentValue = ConvertToPSIRange(seekBar.Progress);
            }
        }

        private void EditSeekBar(object sender, System.EventArgs e)
        {
            try
            {
                CurrentValue = System.Int32.Parse(editPressureValue.Text);
            }
            catch
            {
                CurrentValue = Constants.PRESSURESETMIN;
            }

            // If changing pressure text, update seek bar position.
            if (editPressureValue.IsInEditMode)
            {
                try
                {
                    seekBar.Progress = ConvertToProgress(editPressureValue.Text);
                }
                catch
                {
                    seekBar.Progress = 0;
                }
            }
        }

        private void DeletePreset(int index)
        {
            Global.layout.RemoveView(Global.Presets[index]);

            Global.Presets.RemoveAt(index);
            Global.ActivateButtons.RemoveAt(index);
            Global.EditButtons.RemoveAt(index);
            Global.DeleteButtons.RemoveAt(index);
            Global.PresetVals.RemoveAt(index);

            ShiftTagsAfterDelete(index);
        }

        private void ShiftTagsAfterDelete(int deletedIndex)
        {
            Java.Lang.Object oldActivateTag;
            Java.Lang.Object oldPresetTextTag;
            Java.Lang.Object oldEditTag;
            Java.Lang.Object oldDeleteTag;

            Java.Lang.Object newActivateTag;
            Java.Lang.Object newPresetTextTag;
            Java.Lang.Object newEditTag;
            Java.Lang.Object newDeleteTag;

            // Shift every preset tag, starting at the index of the previously removed index.
            for (int i = deletedIndex; i < Global.Presets.Count; i++)
            {
                oldActivateTag = "Activate Button " + (i + 2).ToString();
                newActivateTag = "Activate Button " + (i + 1).ToString();
                Button activateButton = (Button)Global.Presets[i].FindViewWithTag(oldActivateTag);
                activateButton.Tag = newActivateTag;

                oldPresetTextTag = "Preset Text " + (i + 2).ToString();
                newPresetTextTag = "Preset Text " + (i + 1).ToString();
                TextView presetText = (TextView)Global.Presets[i].FindViewWithTag(oldPresetTextTag);
                presetText.Tag = newPresetTextTag;

                oldEditTag = "Edit Button " + (i + 2).ToString();
                newEditTag = "Edit Button " + (i + 1).ToString();
                ImageButton editButton = (ImageButton)Global.Presets[i].FindViewWithTag(oldEditTag);
                editButton.Tag = newEditTag;

                oldDeleteTag = "Delete Button " + (i + 2).ToString();
                newDeleteTag = "Delete Button " + (i + 1).ToString();
                ImageButton deleteButton = (ImageButton)Global.Presets[i].FindViewWithTag(oldDeleteTag);
                deleteButton.Tag = newDeleteTag;
            }
        }

        private void AddPreset(string name, int val)
        {
            PresetsCount++;

            // Add preset value to PresetVals.
            Global.PresetVals.Add(val);

            // Remove 'Add' button and all existing presets from view.

            // Remove 'Add' button from view.
            Global.layout.RemoveView(addButton);

            // Remove all existing presets from view.
            foreach (LinearLayout preset in Global.Presets)
            {
                Global.layout.RemoveView(preset);
            }

            // Add 'Add' button and all presets back to view.

            // Add in new preset. Index tags starting at 1, to n.
            Global.Presets.Add(CreateLayout(name, val, Global.Presets.Count + 1));

            // Add all presets back to view.
            foreach (LinearLayout preset in Global.Presets)
            {
                Global.layout.AddView(preset);
            }

            // Add 'Add' button to view.
            Global.layout.AddView(addButton);

            // Set tags and click events for all buttons in preset.
            int i = Global.Presets.Count;

            Java.Lang.Object activateTag = "Activate Button " + i.ToString();
            Button activateButton = (Button)Global.Presets[i - 1].FindViewWithTag(activateTag);
            activateButton.Click += ActivateButtonClick;
            Global.ActivateButtons.Add(activateButton);

            Java.Lang.Object presetTextTag = "Preset Text " + i.ToString();

            Java.Lang.Object editTag = "Edit Button " + i.ToString();
            ImageButton editButton = (ImageButton)Global.Presets[i - 1].FindViewWithTag(editTag);
            editButton.Click += EditButtonClick;
            Global.EditButtons.Add(editButton);

            Java.Lang.Object deleteTag = "Delete Button " + i.ToString();
            ImageButton deleteButton = (ImageButton)Global.Presets[i - 1].FindViewWithTag(deleteTag);
            deleteButton.Click += DeleteButtonClick;
            Global.DeleteButtons.Add(deleteButton);
        }

        private void LoadLoadingViews()
        {
            #region Remove Main Views

            try
            {
                Global.layout.RemoveView(vMargin1);
                Global.layout.RemoveView(currentPressureView);
                Global.layout.RemoveView(targetPressureView);
                Global.layout.RemoveView(statusView);
                Global.layout.RemoveView(vMargin2);

                foreach (LinearLayout preset in Global.Presets)
                {
                    Global.layout.RemoveView(preset);
                }

                Global.layout.RemoveView(addButton);
            }
            catch { };

            #endregion

            #region Remove Edit Views

            try
            {
                editPressureLayout1.RemoveView(backButton);
                editPressureLayout2.RemoveView(editPressureName);
                editPressureLayout3.RemoveView(editPressureValue);
                editPressureLayout3.RemoveView(units);
                editPressureLayout4.RemoveView(seekBar);
                editPressureLayout5.RemoveView(setButton);

                Global.layout.RemoveView(editPressureLayout1);
                Global.layout.RemoveView(editPressureLayout2);
                Global.layout.RemoveView(editPressureLayout3);
                Global.layout.RemoveView(editPressureLayout4);
                Global.layout.RemoveView(editPressureLayout5);
            }
            catch { };

            #endregion
        }

        private void LoadMainViews()
        {
            #region Stop Loading Screen Timers

            try
            {
                loadingScreenDots.Change(1000, Timeout.Infinite);
                loadingScreenWheel.Change(1000, Timeout.Infinite);
            }
            catch { };

            #endregion

            #region Remove Loading Screen Views

            try
            {
                loadingScreenLayout1.RemoveView(loadingTextView);
                loadingScreenLayout2.RemoveView(loadingWheel);

                Global.layout.RemoveView(loadingScreenLayout1);
                Global.layout.RemoveView(loadingScreenLayout2);
            }
            catch { };

            #endregion

            #region Remove Edit Views

            try
            {
                editPressureLayout1.RemoveView(backButton);
                editPressureLayout2.RemoveView(editPressureName);
                editPressureLayout3.RemoveView(editPressureValue);
                editPressureLayout3.RemoveView(units);
                editPressureLayout4.RemoveView(seekBar);
                editPressureLayout5.RemoveView(setButton);

                Global.layout.RemoveView(editPressureLayout1);
                Global.layout.RemoveView(editPressureLayout2);
                Global.layout.RemoveView(editPressureLayout3);
                Global.layout.RemoveView(editPressureLayout4);
                Global.layout.RemoveView(editPressureLayout5);
            }
            catch { };

            #endregion

            #region Load Main Views

            try
            {
                Global.layout.AddView(vMargin1);
                Global.layout.AddView(currentPressureView);
                Global.layout.AddView(targetPressureView);
                Global.layout.AddView(statusView);
                Global.layout.AddView(vMargin2);

                foreach (LinearLayout preset in Global.Presets)
                {
                    Global.layout.AddView(preset);
                }

                Global.layout.AddView(addButton);
            }
            catch { };

            #endregion
        }

        private void LoadEditViews(string name, int value)
        {
            CurrentName = name;
            CurrentValue = value;

            #region Remove Main Views

            try
            {
                Global.layout.RemoveView(vMargin1);
                Global.layout.RemoveView(currentPressureView);
                Global.layout.RemoveView(targetPressureView);
                Global.layout.RemoveView(statusView);
                Global.layout.RemoveView(vMargin2);

                foreach (LinearLayout preset in Global.Presets)
                {
                    Global.layout.RemoveView(preset);
                }

                Global.layout.RemoveView(addButton);
            }
            catch { };

            #endregion

            #region Load Edit Views

            try
            {
                Global.layout.AddView(editPressureLayout1);
                Global.layout.AddView(editPressureLayout2);
                Global.layout.AddView(editPressureLayout3);
                Global.layout.AddView(editPressureLayout4);
                Global.layout.AddView(editPressureLayout5);

                editPressureLayout1.AddView(backButton);
                editPressureLayout2.AddView(editPressureName);
                editPressureName.Text = name;
                editPressureLayout3.AddView(editPressureValue);
                editPressureValue.Text = value.ToString();
                editPressureLayout3.AddView(units);
                editPressureLayout4.AddView(seekBar);
                editPressureLayout5.AddView(setButton);
            }
            catch { };

            #endregion
        }

        private LinearLayout CreateLayout(string name, int val, int index)
        {
            //(int)(Resources.DisplayMetrics.HeightPixels * 0.075);

            #region Layout

            LinearLayout layout = new LinearLayout(this);
            layout.Orientation = Orientation.Horizontal;
            layout.SetGravity(Android.Views.GravityFlags.Center);
            layout.SetPadding((int)(Resources.DisplayMetrics.HeightPixels * 0.01), 0, (int)(Resources.DisplayMetrics.HeightPixels * 0.01), (int)(Resources.DisplayMetrics.HeightPixels * 0.02));

            #endregion

            #region Margins

            Space hMargin1 = new Space(this);
            Space hMargin2 = new Space(this);
            Space hMargin3 = new Space(this);

            #endregion

            #region Activate Button

            Button activateButton = new Button(this);
            activateButton.Text = name;
            activateButton.Tag = "Activate Button " + index.ToString();
            layout.AddView(activateButton);

            activateButton.Gravity = Android.Views.GravityFlags.Center;

            activateButton.LayoutParameters.Width = (int)(Resources.DisplayMetrics.WidthPixels * 0.5);
            activateButton.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.075);

            activateButton.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0125);
            activateButton.SetAllCaps(false);
            activateButton.SetTextColor(Android.Graphics.Color.Rgb(255, 255, 255)); // White
            activateButton.SetBackgroundColor(Android.Graphics.Color.Rgb(0, 128, 0)); // #008000

            #endregion

            layout.AddView(hMargin1);
            hMargin1.LayoutParameters.Width = (int)(Resources.DisplayMetrics.WidthPixels * 0.005);

            #region Preset Text

            TextView presetText = new TextView(this);
            presetText.Tag = "Preset Text " + index.ToString();
            presetText.Text = val.ToString() + " psi";
            layout.AddView(presetText);

            presetText.Gravity = Android.Views.GravityFlags.Center;

            presetText.LayoutParameters.Width = (int)(Resources.DisplayMetrics.WidthPixels * 0.2);
            presetText.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.075);

            presetText.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0125);
            presetText.SetAllCaps(false);
            presetText.SetTextColor(Android.Graphics.Color.White);
            presetText.SetTextColor(Android.Graphics.Color.Rgb(255, 255, 255)); // #White

            #endregion

            layout.AddView(hMargin2);
            hMargin2.LayoutParameters.Width = (int)(Resources.DisplayMetrics.WidthPixels * 0.005);

            #region Edit Button

            ImageButton editButton = new ImageButton(this);
            editButton.Tag = "Edit Button " + index.ToString();
            layout.AddView(editButton);

            editButton.SetBackgroundResource(Resources.GetIdentifier("icon_edit", "drawable", PackageName));

            editButton.LayoutParameters.Width = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);
            editButton.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);

            #endregion

            layout.AddView(hMargin3);
            hMargin3.LayoutParameters.Width = (int)(Resources.DisplayMetrics.WidthPixels * 0.01);

            #region Delete Button

            ImageButton deleteButton = new ImageButton(this);
            deleteButton.Tag = "Delete Button " + index.ToString();
            layout.AddView(deleteButton);

            deleteButton.SetBackgroundResource(Resources.GetIdentifier("icon_delete", "drawable", PackageName));

            deleteButton.LayoutParameters.Width = (int)(Resources.DisplayMetrics.HeightPixels * 0.04);
            deleteButton.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.04);

            #endregion

            return layout;
        }

        private int ConvertToPSIRange(int progress)
        {
            return (int)(((float)progress / 100) * (Constants.PRESSURESETMAX - Constants.PRESSURESETMIN) + Constants.PRESSURESETMIN);
        }

        private int ConvertToProgress(string editPressureText)
        {
            return (int)(((float)System.Int32.Parse(editPressureText) - Constants.PRESSURESETMIN) / (Constants.PRESSURESETMAX - Constants.PRESSURESETMIN) * 100);
        }

        private Int32 ConvertDataOut(int targetPressure, bool forceIdle = false)
        {
            // Array to hold data, in bytes.
            byte[] dataBytes = new byte[4];

            // Array to hold data, in bits.
            var dataBits = new System.Collections.BitArray(dataBytes);

            // Convert target pressure to UART value.
            Int32 targetPressureUART = ConvertUARTOut(targetPressure);

            // Set target pressure bits.
            string binary = Convert.ToString(targetPressureUART, 2);
            int length = binary.Length;

            for (int i = 0; i < length; i++)
            {
                bool value = false;

                if (binary.Substring(length - i - 1, 1) == "1")
                {
                    value = true;
                }

                dataBits.Set(i, value);
            }

            // Set Force Idle bit.
            dataBits.Set(16, forceIdle);

            // Copy bit array to byte array.
            dataBits.CopyTo(dataBytes, 0);

            // Convert data to Int32.
            Int32 output = BitConverter.ToInt32(dataBytes, 0);

            return output;
        }

        private void ConvertDataIn(byte[] input)
        {
            if (input == null)
            {
                return;
            }
            
            // Convert Byte Array to Bit Array.
            var dataBits = new System.Collections.BitArray(input);

            // Set string of UART value of Current Pressure from dataBits[13:0].
            string currentPressureUARTString = "";

            for (int i = 0; i < 14; i++)
            {
                if (dataBits[i])
                {
                    currentPressureUARTString = "1" + currentPressureUARTString;
                }
                else
                {
                    currentPressureUARTString = "0" + currentPressureUARTString;
                }
            }

            // Set UART value of Current Pressure from string of UART value of Current Pressure.
            Int32 currentPressureUART = Convert.ToInt32(currentPressureUARTString, 2);

            // Convert from UART value to actual pressure value.
            Global.currentPressure = ConvertUARTIn(currentPressureUART);

            // Set Pressure Sensor Fault Flag from input[16].
            Global.pressureFault = dataBits[16];

            // Set Status from input[18:17].
            if (!dataBits[18] && !dataBits[17])
            {
                Global.status = 0;
            }
            else if (!dataBits[18] && dataBits[17])
            {
                Global.status = 1;
            }
            else if (dataBits[18] && !dataBits[17])
            {
                Global.status = 2;
            }
            else if (dataBits[18] && dataBits[17])
            {
                Global.status = 3;
            }

            // Set Battery Shutdown from input[19].
            Global.batteryShutdown = dataBits[19];

            ;
        }

        private Int32 ConvertUARTOut(Int32 output)
        {
            return (((output - Constants.PRESSUREMIN) * (Constants.OUTPUTMAX - Constants.OUTPUTMIN)) / 
                (Constants.PRESSUREMAX - Constants.PRESSUREMIN)) + Constants.OUTPUTMIN;
        }

        private Int32 ConvertUARTIn(Int32 input)
        {
            return (((input - Constants.OUTPUTMIN) * (Constants.PRESSUREMAX - Constants.PRESSUREMIN)) / 
                (Constants.OUTPUTMAX - Constants.OUTPUTMIN)) + Constants.PRESSUREMIN;
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

                    // Device was found.

                    // Cancelling discovery...
                    mBluetoothAdapter.CancelDiscovery();

                    mBluetoothGatt = device.ConnectGatt(context, false, mGattCallback);

                    mGattCallback.mBluetoothAdapter = mBluetoothAdapter;
                    mGattCallback.mBluetoothDeviceAddress = deviceAddress;
                    mGattCallback.mBluetoothGatt = mBluetoothGatt;
                }
            }

            if (action == BluetoothAdapter.ActionDiscoveryStarted)
            {
                // Discovery started...
            }

            if (action == BluetoothAdapter.ActionDiscoveryFinished)
            {
                // Discovery finished.

                if (!found)
                {
                    // Device was not found.
                }
            }
        }

        public byte[] Read()
        {
            // Returns input if Read operation is successful; returns null otherwise.

            try
            {
                return mGattCallback.Read();
            }
            catch
            {
                return null;
            }
        }

        public bool Write(Int32 output)
        {
            // Returns true if Write operation is successful; returns false otherwise.

            try
            {
                return mGattCallback.Write(output);
            }
            catch
            {
                return false;
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
    
    public class GattCallback : BluetoothGattCallback
    {
        public BluetoothManager mBluetoothManager { get; set; }
        public BluetoothAdapter mBluetoothAdapter { get; set; }
        public string mBluetoothDeviceAddress { get; set; }
        public BluetoothGatt mBluetoothGatt { get; set; }

        public int mConnectionState = BluetoothLeService.STATE_DISCONNECTED;

        public GattUpdateReceiver mGattUpdateReveiver = new GattUpdateReceiver();

        public DeviceControlActivity mDeviceControlActivity = new DeviceControlActivity();
        public BluetoothGattCharacteristic mGattCharacteristic;

        public byte[] data;

        public override void OnConnectionStateChange(BluetoothGatt gatt, GattStatus status, ProfileState newState)
        {
            base.OnConnectionStateChange(gatt, status, newState);

            if (newState == ProfileState.Connected)
            {
                mConnectionState = BluetoothLeService.STATE_CONNECTED;

                // Connected to GATT server.
                // Attempting to start servce discovery...

                try
                {
                    mBluetoothGatt.DiscoverServices();
                }
                catch
                {
                    // Remote service discovery could not be started.
                }
            }
            else if (newState == ProfileState.Disconnected)
            {
                // Disconnected from GATT server.
                mConnectionState = BluetoothLeService.STATE_DISCONNECTED;
            }
        }
        
        public override void OnServicesDiscovered(BluetoothGatt gatt, GattStatus status)
        {
            base.OnServicesDiscovered(gatt, status);

            if (status == GattStatus.Success)
            {
                // Service discovery successful.
                
                // Gathering available GATT services...
                mDeviceControlActivity.DisplayGattServices(gatt.Services);

                // Read/Write commands may now be issued.
                // Load Main Views.
                Global.isConnected = true;

                // Write once, then continually Read.
                Global.startWrite = true;
            }
            else
            {
                // Service discovery failed.
            }
        }
        
        public override void OnCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, GattStatus status)
        {
            base.OnCharacteristicRead(gatt, characteristic, status);

            if (status == GattStatus.Success)
            {
                BroadcastUpdate(BluetoothLeService.ACTION_DATA_AVAILABLE, characteristic);
            }
        }

        public override void OnCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, [GeneratedEnum] GattStatus status)
        {
            base.OnCharacteristicWrite(gatt, characteristic, status);

            if (status == GattStatus.Success)
            {
                BroadcastUpdate(BluetoothLeService.ACTION_DATA_AVAILABLE, characteristic);
            }
        }

        private void BroadcastUpdate(string action, BluetoothGattCharacteristic characteristic)
        {
            Intent intent = new Intent(action);

            data = characteristic.GetValue();
        }

        public byte[] Read()
        {
            // Returns input if Read operation is successful; returns null otherwise.

            byte[] input = null;

            // Check if mDeviceControlActivity has been instantiated.
            if (mDeviceControlActivity == null)
            {
                return input;
            }

            mGattCharacteristic = mDeviceControlActivity.mGattCharacteristics[5][0];

            if (mBluetoothGatt.ReadCharacteristic(mGattCharacteristic))
            {
                // Read operation successful.

                // Delay to allow data to be recieved.
                System.Threading.Thread.Sleep(Constants.READWAIT);

                // Return the data received from callback.
                input = this.data;
            }
            else
            {
                // Read operation failed.
            }

            return input;
        }

        public bool Write(Int32 output)
        {
            // Returns true if Write operation is successful; returns false otherwise.

            // Check if mDeviceControlActivity has been instantiated.
            if (mDeviceControlActivity == null)
            {
                return false;
            }

            mGattCharacteristic = mDeviceControlActivity.mGattCharacteristics[5][0];

            mGattCharacteristic.SetValue(output, GattFormat.Uint32, 0);

            BluetoothGattDescriptor mDescriptor = mGattCharacteristic.GetDescriptor(UUID.FromString("f0001131-0451-4000-b000-000000000000"));

            if (mBluetoothGatt.WriteCharacteristic(mGattCharacteristic))
            {
                // Write operation successful.

                return true;
            }
            else
            {
                // Write operation failed.

                return false;
            }
        }
    }

    public class GattUpdateReceiver : BroadcastReceiver
    {
        public override void OnReceive(Context context, Intent intent)
        {
            string action = intent.Action;

            if (BluetoothLeService.ACTION_DATA_AVAILABLE.Equals(action))
            {
                string data = intent.GetStringExtra(BluetoothLeService.EXTRA_DATA);
            }
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