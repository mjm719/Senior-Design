using Android.App;
using Android.Widget;
using Android.OS;
using Android.Bluetooth;

using System.Threading;
using System.Collections.Generic;
using System.Resources;

namespace Main
{
    [Activity(Label = "BPR", MainLauncher = true, Icon = "@drawable/icon")]
    public class MainActivity : Activity
    {
        static float widthInDp;
        static float heightInDp;

        static int MINPRESSURE = 15;
        static int MAXPRESSURE = 70;
        static int MAXPRESETS = 5;
        public int currentPressure;
        public int targetPressure;

        LinearLayout layout;
        List<LinearLayout> Presets;
        List<Button> ActivateButtons;
        List<ImageButton> EditButtons;
        List<ImageButton> DeleteButtons;
        List<int> PresetVals;
        public int PresetsCount = 0;
        public int CurrentIndex;
        public string CurrentName;
        public int CurrentValue;
        public bool NewPreset;

        TextView currentPressureView;
        TextView targetPressureView;
        TextView statusView;
        Space vMargin1;
        Space vMargin2;
        ImageButton addButton;

        LinearLayout editPressureTextLayout1;
        LinearLayout editPressureTextLayout2;
        LinearLayout editPressureTextLayout3;
        LinearLayout editPressureTextLayout4;
        LinearLayout editPressureTextLayout5;
        ImageButton backButton;
        EditText editPressureName;
        EditText editPressureValue;
        TextView units;
        SeekBar seekBar;
        Button setButton;

        Timer refresh;
        Timer update;

        protected override void OnCreate(Bundle bundle)
        {
            base.OnCreate(bundle);

            widthInDp = Resources.DisplayMetrics.WidthPixels * ((float)Resources.DisplayMetrics.DensityDpi / 160f);
            heightInDp = Resources.DisplayMetrics.HeightPixels * ((float)Resources.DisplayMetrics.DensityDpi / 160f);

            #region Bluetooth

            /*
            BluetoothAdapter adapter = BluetoothAdapter.DefaultAdapter;
            if (adapter == null)
                throw new System.ArgumentException("No Bluetooth adapter found.");

            if (!adapter.IsEnabled)
                throw new System.ArgumentException("Bluetooth adapter is not enabled.");


            BluetoothDevice device = (from bd in adapter.BondedDevices
                                      where bd.Name == "NameOfTheDevice"
                                      select bd).FirstOrDefault();

            if (device == null)
                throw new System.ArgumentException("Named device not found.");

            BluetoothSocket _socket = device.CreateRfcommSocketToServiceRecord(UUID.FromString("00001101-0000-1000-8000-00805f9b34fb"));
            await _socket.ConnectAsync();

            // Read data from the device
            await _socket.InputStream.ReadAsync(buffer, 0, buffer.Length);

            // Write data to the device
            await _socket.OutputStream.WriteAsync(buffer, 0, buffer.Length);
            */

            #endregion

            layout = new LinearLayout(this);
            layout.Orientation = Orientation.Vertical;
            layout.SetGravity(Android.Views.GravityFlags.CenterHorizontal);
            SetContentView(layout);

            currentPressure = 50;
            targetPressure = currentPressure;

            // Start refresh timer immediately, invoke callback every 10 ms.
            // http://stackoverflow.com/questions/13019433/calling-method-on-every-x-minutes
            refresh = new System.Threading.Timer(x => RefreshView(), null, 0, 10);

            // Start update timer immediately, invoke callback every 200 ms.
            update = new System.Threading.Timer(x => Update(), null, 0, 200);

            #region Main Views

            vMargin1 = new Space(this);
            layout.AddView(vMargin1);
            vMargin1.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.025);

            #region Current Pressure

            currentPressureView = new TextView(this);
            layout.AddView(currentPressureView);
            currentPressureView.Gravity = Android.Views.GravityFlags.Center;
            currentPressureView.Text = "Current Pressure: ???";
            currentPressureView.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            currentPressureView.SetTextColor(Android.Graphics.Color.White);

            #endregion

            #region Target Pressure

            targetPressureView = new TextView(this);
            layout.AddView(targetPressureView);
            targetPressureView.Gravity = Android.Views.GravityFlags.Center;
            targetPressureView.Text = "Target Pressure: ???";
            targetPressureView.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            targetPressureView.SetTextColor(Android.Graphics.Color.White);

            #endregion

            #region Status

            statusView = new TextView(this);
            layout.AddView(statusView);
            statusView.Gravity = Android.Views.GravityFlags.Center;
            statusView.Text = "Initializing...";
            statusView.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.015);
            statusView.SetTextColor(Android.Graphics.Color.White);

            #endregion

            vMargin2 = new Space(this);
            layout.AddView(vMargin2);
            vMargin2.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);

            #region Add Button

            addButton = new ImageButton(this);
            layout.AddView(addButton);
            addButton.SetBackgroundResource(Resources.GetIdentifier("icon_add2_white", "drawable", PackageName));
            addButton.LayoutParameters.Width = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);
            addButton.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);
            addButton.Click += AddButtonClick;

            #endregion

            #region Presets

            Presets = new List<LinearLayout>();
            ActivateButtons = new List<Button>();
            EditButtons = new List<ImageButton>();
            DeleteButtons = new List<ImageButton>();
            PresetVals = new List<int>();

            // Start out with 2 premade presets.

            AddPreset("Preset 1", MINPRESSURE);
            AddPreset("Preset 2", MAXPRESSURE);

            #endregion

            #endregion

            #region Edit Views

            #region Back Button

            editPressureTextLayout1 = new LinearLayout(this);
            layout.AddView(editPressureTextLayout1);
            editPressureTextLayout1.Orientation = Orientation.Horizontal;
            editPressureTextLayout1.SetGravity(Android.Views.GravityFlags.Left);
            editPressureTextLayout1.SetPadding((int)(Resources.DisplayMetrics.WidthPixels * 0.04), (int)(Resources.DisplayMetrics.HeightPixels * 0.04), 0, 0);

            backButton = new ImageButton(this);
            editPressureTextLayout1.AddView(backButton);
            backButton.SetBackgroundResource(Resources.GetIdentifier("icon_back2_white", "drawable", PackageName));
            backButton.LayoutParameters.Width = (int)(Resources.DisplayMetrics.HeightPixels * 0.04);
            backButton.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.04);
            backButton.Click += BackButtonClick;

            #endregion

            #region Preset Name

            editPressureTextLayout2 = new LinearLayout(this);
            layout.AddView(editPressureTextLayout2);
            editPressureTextLayout2.Orientation = Orientation.Horizontal;
            editPressureTextLayout2.SetGravity(Android.Views.GravityFlags.Center);
            editPressureTextLayout2.SetPadding(0, 0, 0, 0);

            editPressureName = new EditText(this);
            editPressureTextLayout2.AddView(editPressureName);
            editPressureName.Text = "???";
            editPressureName.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            editPressureName.SetTextColor(Android.Graphics.Color.White);
            editPressureName.TextChanged += EditName;

            #endregion

            #region Pressure Value

            editPressureTextLayout3 = new LinearLayout(this);
            layout.AddView(editPressureTextLayout3);
            editPressureTextLayout3.Orientation = Orientation.Horizontal;
            editPressureTextLayout3.SetGravity(Android.Views.GravityFlags.Center);
            editPressureTextLayout3.SetPadding(0, 0, 0, 0);

            editPressureValue = new EditText(this);
            editPressureTextLayout3.AddView(editPressureValue);
            editPressureValue.Text = "???";
            editPressureValue.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            editPressureValue.SetTextColor(Android.Graphics.Color.White);
            editPressureValue.TextChanged += EditSeekBar;

            units = new TextView(this);
            editPressureTextLayout3.AddView(units);
            units.Text = " psi";
            units.TextSize = (int)(Resources.DisplayMetrics.WidthPixels * 0.0175);
            units.SetTextColor(Android.Graphics.Color.White);

            #endregion

            #region Seek Bar

            editPressureTextLayout4 = new LinearLayout(this);
            layout.AddView(editPressureTextLayout4);
            editPressureTextLayout4.Orientation = Orientation.Horizontal;
            editPressureTextLayout4.SetGravity(Android.Views.GravityFlags.Center);
            editPressureTextLayout4.SetPadding(0, 0, 0, 0);

            seekBar = new SeekBar(this);
            editPressureTextLayout4.AddView(seekBar);
            seekBar.LayoutParameters.Width = (int)(Resources.DisplayMetrics.WidthPixels * 0.8);
            seekBar.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);
            seekBar.ProgressChanged += EditPSI;

            #endregion

            #region Set Button

            editPressureTextLayout5 = new LinearLayout(this);
            layout.AddView(editPressureTextLayout5);
            editPressureTextLayout5.Orientation = Orientation.Horizontal;
            editPressureTextLayout5.SetGravity(Android.Views.GravityFlags.Center);
            editPressureTextLayout5.SetPadding(0, 20, 0, 0);

            setButton = new Button(this);
            editPressureTextLayout5.AddView(setButton);
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

            LoadMainViews();
        }

        private void RefreshView()
        {
            // Refresh Current Pressure.
            this.RunOnUiThread((() => currentPressureView.Text = "Current Pressure: " + currentPressure + " psi"));

            // Refresh Target Pressure.
            this.RunOnUiThread((() => targetPressureView.Text = "Target Pressure: " + targetPressure + " psi"));

            // Refresh Status.
            if (currentPressure < targetPressure)
            {
                this.RunOnUiThread((() => statusView.Text = "Pressurizing..."));
            }

            if (currentPressure > targetPressure)
            {
                this.RunOnUiThread((() => statusView.Text = "Releasing..."));
            }

            if (currentPressure == targetPressure)
            {
                this.RunOnUiThread((() => statusView.Text = "Pressure stabilized."));
            }

            try
            {
                if (Presets.Count >= MAXPRESETS)
                {
                    addButton.Visibility = Android.Views.ViewStates.Invisible;
                }

                if (Presets.Count < MAXPRESETS)
                {
                    addButton.Visibility = Android.Views.ViewStates.Visible;
                }
            }
            catch { };
        }

        private void Update()
        {
            // Update Current Pressure from Bluetooth every 200 ms.

            // Code snippit to test pressure changes:
            if (currentPressure < targetPressure)
            {
                currentPressure += 1;
            }

            if (currentPressure > targetPressure)
            {
                currentPressure -= 1;
            }
        }

        private void ActivateButtonClick(object sender, System.EventArgs e)
        {
            Java.Lang.Object tag;
            Java.Lang.Object senderTag;

            for (int i = 0; i < Presets.Count; i++)
            {
                tag = "Activate Button " + (i + 1).ToString();
                senderTag = (sender as Button).Tag;

                if (senderTag.ToString() == tag.ToString())
                {
                    targetPressure = PresetVals[i];
                }
            }
        }

        private void EditButtonClick(object sender, System.EventArgs e)
        {
            NewPreset = false;

            Java.Lang.Object tag;
            Java.Lang.Object senderTag;

            for (int i = 0; i < Presets.Count; i++)
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

            for (int i = 0; i < Presets.Count; i++)
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
            int value = MINPRESSURE;

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
            }
            // If this is an Edit operation, assign new name/value to cooresponding preset.
            else
            {
                Button activateButton = new Button(this);
                activateButton = (Button)Presets[CurrentIndex].FindViewWithTag("Activate Button " + (CurrentIndex + 1).ToString());
                activateButton.Text = CurrentName;

                TextView presetText = new TextView(this);
                presetText = (TextView)Presets[CurrentIndex].FindViewWithTag("Preset Text " + (CurrentIndex + 1).ToString());
                presetText.Text = CurrentValue.ToString() + " psi";

                PresetVals[CurrentIndex] = CurrentValue;
            }

            NewPreset = false;
        }

        private void EditPreset(int index)
        {
            CurrentIndex = index;

            // Extract name from preset at given index.
            Java.Lang.Object activateTag = "Activate Button " + (index + 1).ToString();
            Button activateButton = (Button)Presets[index].FindViewWithTag(activateTag);
            string name = activateButton.Text;

            // Extract value from preset at given index.
            Java.Lang.Object presetTextTag = "Preset Text " + (index + 1).ToString();
            TextView presetValue = (TextView)Presets[index].FindViewWithTag(presetTextTag);
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
                CurrentValue = MINPRESSURE;
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
            layout.RemoveView(Presets[index]);

            Presets.RemoveAt(index);
            ActivateButtons.RemoveAt(index);
            EditButtons.RemoveAt(index);
            DeleteButtons.RemoveAt(index);
            PresetVals.RemoveAt(index);

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
            for (int i = deletedIndex; i < Presets.Count; i++)
            {
                oldActivateTag = "Activate Button " + (i + 2).ToString();
                newActivateTag = "Activate Button " + (i + 1).ToString();
                Button activateButton = (Button)Presets[i].FindViewWithTag(oldActivateTag);
                activateButton.Tag = newActivateTag;

                oldPresetTextTag = "Preset Text " + (i + 2).ToString();
                newPresetTextTag = "Preset Text " + (i + 1).ToString();
                TextView presetText = (TextView)Presets[i].FindViewWithTag(oldPresetTextTag);
                presetText.Tag = newPresetTextTag;

                oldEditTag = "Edit Button " + (i + 2).ToString();
                newEditTag = "Edit Button " + (i + 1).ToString();
                ImageButton editButton = (ImageButton)Presets[i].FindViewWithTag(oldEditTag);
                editButton.Tag = newEditTag;

                oldDeleteTag = "Delete Button " + (i + 2).ToString();
                newDeleteTag = "Delete Button " + (i + 1).ToString();
                ImageButton deleteButton = (ImageButton)Presets[i].FindViewWithTag(oldDeleteTag);
                deleteButton.Tag = newDeleteTag;
            }
        }

        private void AddPreset(string name, int val)
        {
            PresetsCount++;

            // Add preset value to PresetVals.
            PresetVals.Add(val);

            // Remove 'Add' button and all existing presets from view.

            // Remove 'Add' button from view.
            layout.RemoveView(addButton);

            // Remove all existing presets from view.
            foreach (LinearLayout preset in Presets)
            {
                layout.RemoveView(preset);
            }

            // Add 'Add' button and all presets back to view.

            // Add in new preset. Index tags starting at 1, to n.
            Presets.Add(CreateLayout(name, val, Presets.Count + 1));

            // Add all presets back to view.
            foreach (LinearLayout preset in Presets)
            {
                layout.AddView(preset);
            }

            // Add 'Add' button to view.
            layout.AddView(addButton);

            // Set tags and click events for all buttons in preset.
            int i = Presets.Count;

            Java.Lang.Object activateTag = "Activate Button " + i.ToString();
            Button activateButton = (Button)Presets[i - 1].FindViewWithTag(activateTag);
            activateButton.Click += ActivateButtonClick;
            ActivateButtons.Add(activateButton);

            Java.Lang.Object presetTextTag = "Preset Text " + i.ToString();

            Java.Lang.Object editTag = "Edit Button " + i.ToString();
            ImageButton editButton = (ImageButton)Presets[i - 1].FindViewWithTag(editTag);
            editButton.Click += EditButtonClick;
            EditButtons.Add(editButton);

            Java.Lang.Object deleteTag = "Delete Button " + i.ToString();
            ImageButton deleteButton = (ImageButton)Presets[i - 1].FindViewWithTag(deleteTag);
            deleteButton.Click += DeleteButtonClick;
            DeleteButtons.Add(deleteButton);
        }

        private void LoadMainViews()
        {
            #region Remove Edit Views

            try
            {
                editPressureTextLayout1.RemoveView(backButton);
                editPressureTextLayout2.RemoveView(editPressureName);
                editPressureTextLayout3.RemoveView(editPressureValue);
                editPressureTextLayout3.RemoveView(units);
                editPressureTextLayout4.RemoveView(seekBar);
                editPressureTextLayout5.RemoveView(setButton);

                layout.RemoveView(editPressureTextLayout1);
                layout.RemoveView(editPressureTextLayout2);
                layout.RemoveView(editPressureTextLayout3);
                layout.RemoveView(editPressureTextLayout4);
                layout.RemoveView(editPressureTextLayout5);
            }
            catch { };

            #endregion

            #region Load Main Views

            try
            {
                layout.AddView(vMargin1);
                layout.AddView(currentPressureView);
                layout.AddView(targetPressureView);
                layout.AddView(statusView);
                layout.AddView(vMargin2);

                foreach (LinearLayout preset in Presets)
                {
                    layout.AddView(preset);
                }

                layout.AddView(addButton);
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
                layout.RemoveView(vMargin1);
                layout.RemoveView(currentPressureView);
                layout.RemoveView(targetPressureView);
                layout.RemoveView(statusView);
                layout.RemoveView(vMargin2);

                foreach (LinearLayout preset in Presets)
                {
                    layout.RemoveView(preset);
                }

                layout.RemoveView(addButton);
            }
            catch { };

            #endregion

            #region Load Edit Views

            try
            {
                layout.AddView(editPressureTextLayout1);
                layout.AddView(editPressureTextLayout2);
                layout.AddView(editPressureTextLayout3);
                layout.AddView(editPressureTextLayout4);
                layout.AddView(editPressureTextLayout5);

                editPressureTextLayout1.AddView(backButton);
                editPressureTextLayout2.AddView(editPressureName);
                editPressureName.Text = name;
                editPressureTextLayout3.AddView(editPressureValue);
                editPressureValue.Text = value.ToString();
                editPressureTextLayout3.AddView(units);
                editPressureTextLayout4.AddView(seekBar);
                editPressureTextLayout5.AddView(setButton);
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
            activateButton.SetTextColor(Android.Graphics.Color.Rgb(224, 247, 250)); // #E0F7FA
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
            //presetText.SetTextColor(Android.Graphics.Color.Rgb(224, 247, 250)); // #E0F7FA

            #endregion

            layout.AddView(hMargin2);
            hMargin2.LayoutParameters.Width = (int)(Resources.DisplayMetrics.WidthPixels * 0.005);

            #region Edit Button

            ImageButton editButton = new ImageButton(this);
            editButton.Tag = "Edit Button " + index.ToString();
            layout.AddView(editButton);

            editButton.SetBackgroundResource(Resources.GetIdentifier("icon_edit2_white", "drawable", PackageName));

            editButton.LayoutParameters.Width = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);
            editButton.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.05);

            #endregion

            layout.AddView(hMargin3);
            hMargin3.LayoutParameters.Width = (int)(Resources.DisplayMetrics.WidthPixels * 0.01);

            #region Delete Button

            ImageButton deleteButton = new ImageButton(this);
            deleteButton.Tag = "Delete Button " + index.ToString();
            layout.AddView(deleteButton);

            deleteButton.SetBackgroundResource(Resources.GetIdentifier("icon_delete3_white", "drawable", PackageName));

            deleteButton.LayoutParameters.Width = (int)(Resources.DisplayMetrics.HeightPixels * 0.04);
            deleteButton.LayoutParameters.Height = (int)(Resources.DisplayMetrics.HeightPixels * 0.04);

            #endregion

            return layout;
        }

        private int ConvertToPSIRange(int progress)
        {
            return (int)(((float)progress / 100) * (MAXPRESSURE - MINPRESSURE) + MINPRESSURE);
        }

        private int ConvertToProgress(string editPressureText)
        {
            return (int)(((float)System.Int32.Parse(editPressureText) - MINPRESSURE) / (MAXPRESSURE - MINPRESSURE) * 100);
        }
    }
}

