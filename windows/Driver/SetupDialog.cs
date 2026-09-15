using System;
using System.Drawing;
using System.IO.Ports;
using System.Windows.Forms;
using Microsoft.Win32;

namespace MoMaRoTa
{
    internal static class Settings
    {
        private const string Key = @"Software\MoMaRoTa\UsbRotator";
        internal static string Port
        {
            get { using (var key = Registry.CurrentUser.OpenSubKey(Key)) return (string)key?.GetValue("Port", "") ?? ""; }
            set { using (var key = Registry.CurrentUser.CreateSubKey(Key)) key.SetValue("Port", value); }
        }
        internal static bool Reverse
        {
            get { using (var key = Registry.CurrentUser.OpenSubKey(Key)) return Convert.ToInt32(key?.GetValue("Reverse", 0) ?? 0) != 0; }
            set { using (var key = Registry.CurrentUser.CreateSubKey(Key)) key.SetValue("Reverse", value ? 1 : 0, RegistryValueKind.DWord); }
        }
    }
    internal sealed class SetupDialog : Form
    {
        internal SetupDialog()
        {
            Text = Rotator.DisplayName;
            ClientSize = new Size(450, 240); AutoScaleMode = AutoScaleMode.Dpi;
            FormBorderStyle = FormBorderStyle.FixedDialog; MaximizeBox = false; MinimizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            var label = new Label { Text = "USB COM port (115200 baud)", Location = new Point(20, 20), AutoSize = true };
            var ports = new ComboBox { Location = new Point(20, 48), Width = 240, DropDownStyle = ComboBoxStyle.DropDownList };
            Action refresh = () => {
                string selected = ports.SelectedItem as string ?? Settings.Port;
                ports.Items.Clear(); string[] names = SerialPort.GetPortNames(); Array.Sort(names); ports.Items.AddRange(names);
                if (!string.IsNullOrEmpty(selected) && !ports.Items.Contains(selected)) ports.Items.Add(selected);
                if (!string.IsNullOrEmpty(selected)) ports.SelectedItem = selected;
            };
            var scan = new Button { Text = "Refresh", Location = new Point(280, 46), Width = 140 };
            scan.Click += (_, __) => refresh(); refresh();
            var reverse = new CheckBox { Text = "Reverse rotation direction", Location = new Point(20, 87), AutoSize = true, Checked = Settings.Reverse };
            var info = new Label { Text = "Close the serial monitor before connecting.\nAfter a controller restart, perform a new plate-solve Sync.\nUse one USB client at a time.", Location = new Point(20, 120), Size = new Size(410, 60) };
            var ok = new Button { Text = "Save", Location = new Point(250, 195), Width = 80 };
            ok.Click += (_, __) => {
                if (ports.SelectedItem == null) { MessageBox.Show("Select a COM port."); return; }
                Settings.Port = (string)ports.SelectedItem; Settings.Reverse = reverse.Checked;
                DialogResult = DialogResult.OK; Close();
            };
            var cancel = new Button { Text = "Cancel", Location = new Point(340, 195), Width = 80, DialogResult = DialogResult.Cancel };
            AcceptButton = ok; CancelButton = cancel;
            Controls.AddRange(new Control[] { label, ports, scan, reverse, info, ok, cancel });
        }
    }
}
