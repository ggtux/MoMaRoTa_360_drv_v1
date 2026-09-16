using System;
using System.Collections;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Threading;
using ASCOM;
using ASCOM.DeviceInterface;
using Newtonsoft.Json.Linq;

[assembly: ComVisible(false)]
namespace MoMaRoTa
{
    [ComVisible(true)]
    [Guid("9BA3AC78-96AF-46D7-9976-E3F940DB8587")]
    [ProgId(DriverId)]
    [ClassInterface(ClassInterfaceType.None)]
    [ComDefaultInterface(typeof(IRotatorV3))]
    public sealed class Rotator : ReferenceCountedObjectBase, IRotatorV3
    {
        public const string DriverId = "ASCOM.MoMaRoTa.Rotator";
        internal const string DisplayName = "Astro Orbit";
        private readonly object gate = new object();
        private ILineChannel channel;
        private Protocol protocol;
        private Timer heartbeat;
        private Exception connectionFault;
        private bool disposed;

        public Rotator() { ServerObjects.Track(this); }
        ~Rotator()
        {
            // The heartbeat holds only a weak reference, so COM client termination
            // can release this instance and close its serial connection.
            try { Dispose(); } catch { }
        }

        public bool Connected
        {
            get { lock (gate) return protocol != null && connectionFault == null; }
            set
            {
                lock (gate)
                {
                    if (!value) { Disconnect(); return; }
                    if (disposed) throw new DriverException("Driver has been disposed.");
                    if (protocol != null && connectionFault == null) return;
                    Disconnect();
                    try
                    {
                        string port = Settings.Port;
                        if (string.IsNullOrWhiteSpace(port)) throw new DriverException("Select a COM port in Properties / Setup first.");
                        channel = new SerialChannel(port);
                        protocol = new Protocol(channel);
                        protocol.Handshake();
                        RotatorStatus.Parse(protocol.Exchange("connect"));
                        protocol.Exchange("reverse", Settings.Reverse);
                        connectionFault = null;
                        heartbeat = new Timer(HeartbeatTick, new WeakReference(this), 2000, 2000);
                    }
                    catch (Exception ex)
                    {
                        // If Connect succeeded but a later step failed, release our lease.
                        try { protocol?.Exchange("disconnect", timeoutMs: 1000); } catch { }
                        CloseChannel();
                        throw Translate(ex);
                    }
                }
            }
        }

        private static void HeartbeatTick(object state)
        {
            var driver = ((WeakReference)state).Target as Rotator;
            if (driver != null) driver.KeepAlive();
        }
        private void KeepAlive()
        {
            // No queue of timer callbacks while a foreground transaction is active.
            if (!Monitor.TryEnter(gate)) return;
            try
            {
                if (protocol == null || connectionFault != null) return;
                try { RotatorStatus.Parse(protocol.Exchange("status")); }
                catch (Exception ex) { LocalServer.Log(ex.ToString()); connectionFault = ex; CloseChannel(); }
            }
            finally { Monitor.Exit(gate); }
        }
        private void CloseChannel()
        {
            heartbeat?.Dispose(); heartbeat = null;
            try { channel?.Dispose(); }
            catch (Exception ex) { if (connectionFault == null) connectionFault = ex; }
            finally { channel = null; protocol = null; }
        }
        private void Disconnect()
        {
            Exception error = null;
            try { if (protocol != null) protocol.Exchange("disconnect"); }
            catch (Exception ex) { error = ex; }
            finally { CloseChannel(); connectionFault = null; }
            if (error != null) throw Translate(error);
        }
        private void RequireConnected()
        {
            if (connectionFault != null) throw new DriverException("USB connection failed. Reconnect. " + connectionFault.Message);
            if (protocol == null) throw new NotConnectedException("Astro Orbit USB is not connected.");
        }
        private JObject Request(string command, object value = null)
        {
            lock (gate)
            {
                RequireConnected();
                try { return protocol.Exchange(command, value); }
                catch (DeviceError ex)
                {
                    if (ex.Code == 1031) { connectionFault = ex; CloseChannel(); }
                    throw Translate(ex);
                }
                catch (Exception ex)
                {
                    connectionFault = ex; CloseChannel();
                    throw Translate(ex);
                }
            }
        }
        private RotatorStatus Status(bool checkMotor = true)
        {
            lock (gate)
            {
                RotatorStatus status;
                try { status = RotatorStatus.Parse(Request("status")); }
                catch (ASCOM.DriverException) { throw; }
                catch (Exception ex) { throw Translate(ex); }
                if (checkMotor && !status.MotorHealthy) throw new DriverException("Motor feedback unavailable. Check motor power and wiring.");
                if (checkMotor && !string.IsNullOrEmpty(status.MotionError)) throw new DriverException(status.MotionError);
                return status;
            }
        }
        private static Exception Translate(Exception ex)
        {
            LocalServer.Log(ex.ToString());
            if (ex is ASCOM.DriverException) return ex;
            if (ex is DeviceError device)
            {
                if (device.Code == 1031) return new NotConnectedException(device.Message);
                if (device.Code == 1025) return new InvalidValueException(device.Message);
                return new DriverException(device.Message);
            }
            return new DriverException("Astro Orbit USB: " + ex.Message);
        }
        private void Validate(float value, bool relative)
        {
            lock (gate) RequireConnected();
            if (float.IsNaN(value) || float.IsInfinity(value) ||
                (relative ? value < -360 || value > 360 : value < 0 || value >= 360))
                throw new InvalidValueException("Position", value.ToString(CultureInfo.InvariantCulture), relative ? "-360 to 360 degrees" : "0 <= Position < 360 degrees");
        }

        public string Name => DisplayName;
        public string Description => "Astro Orbit ESP32 / ST3215 USB field rotator";
        public string DriverInfo => "Astro Orbit ASCOM LocalServer 1.4.2 x86; USB protocol 1; 115200 baud; ASCOM Rotator V3. Persistent mechanical position and zero control in Setup.";
        public string DriverVersion => "1.4.2";
        public short InterfaceVersion => 3;
        public ArrayList SupportedActions => new ArrayList { "ZeroPosition" };
        public bool CanReverse { get { lock (gate) { RequireConnected(); return true; } } }
        public bool IsMoving => Status().Moving;
        public float Position => Status().Position;
        public float MechanicalPosition => Status().Mechanical;
        public float TargetPosition => Status().Target;
        public float StepSize => Status().StepSize;
        public bool Reverse
        {
            get => Status(false).Reverse;
            set { lock (gate) { Request("reverse", value); Settings.Reverse = value; } }
        }
        public void Move(float Position) { LocalServer.Log("Move " + Position.ToString(CultureInfo.InvariantCulture)); Validate(Position, true); Request("move", Position); }
        public void MoveAbsolute(float Position) { LocalServer.Log("MoveAbsolute " + Position.ToString(CultureInfo.InvariantCulture)); Validate(Position, false); Request("absolute", Position); }
        public void MoveMechanical(float Position) { LocalServer.Log("MoveMechanical " + Position.ToString(CultureInfo.InvariantCulture)); Validate(Position, false); Request("mechanical", Position); }
        public void Sync(float Position) { LocalServer.Log("Sync " + Position.ToString(CultureInfo.InvariantCulture)); Validate(Position, false); Request("sync", Position); }
        public void Halt() { LocalServer.Log("Halt"); Request("halt"); }
        private void ZeroPosition() { LocalServer.Log("ZeroPosition"); Request("zero"); }
        private void ZeroPositionFromSetup(string portName, bool alreadyConnected)
        {
            if(alreadyConnected) { ZeroPosition(); return; }
            ILineChannel temporaryChannel = null;
            Protocol temporaryProtocol = null;
            try
            {
                temporaryChannel = new SerialChannel(portName);
                temporaryProtocol = new Protocol(temporaryChannel);
                temporaryProtocol.Handshake();
                RotatorStatus.Parse(temporaryProtocol.Exchange("connect"));
                temporaryProtocol.Exchange("zero");
            }
            catch(Exception ex) { throw Translate(ex); }
            finally
            {
                try { temporaryProtocol?.Exchange("disconnect", timeoutMs: 1000); } catch { }
                temporaryChannel?.Dispose();
            }
        }
        public string Action(string ActionName, string ActionParameters)
        {
            if(string.Equals(ActionName, "ZeroPosition", StringComparison.OrdinalIgnoreCase)) { ZeroPosition(); return ""; }
            throw new ActionNotImplementedException(ActionName);
        }
        public void CommandBlind(string Command, bool Raw) { lock (gate) RequireConnected(); throw new MethodNotImplementedException("CommandBlind"); }
        public bool CommandBool(string Command, bool Raw) { lock (gate) RequireConnected(); throw new MethodNotImplementedException("CommandBool"); }
        public string CommandString(string Command, bool Raw) { lock (gate) RequireConnected(); throw new MethodNotImplementedException("CommandString"); }
        public void SetupDialog()
        {
            bool connected;
            lock(gate) connected = protocol != null && connectionFault == null;
            Action<string> zero = portName => ZeroPositionFromSetup(portName, connected);
            LocalServer.Log("SetupDialog entered on " + Thread.CurrentThread.GetApartmentState() + " thread");
                // LocalServer COM calls already arrive on its STA message-loop
                // thread. Showing the dialog there keeps COM messages pumping.
                if (Thread.CurrentThread.GetApartmentState() == ApartmentState.STA)
                {
                    using (var dialog = new SetupDialog(zero, connected)) dialog.ShowDialog();
                    LocalServer.Log("SetupDialog closed");
                    return;
                }
                // Retain compatibility if a host ever invokes this from MTA.
                Exception error = null;
                var thread = new Thread(() => {
                    try { using (var dialog = new SetupDialog(zero, connected)) dialog.ShowDialog(); }
                    catch (Exception ex) { error = ex; }
                });
                thread.SetApartmentState(ApartmentState.STA); thread.Start(); thread.Join();
                if (error != null) throw Translate(error);
            LocalServer.Log("SetupDialog closed");
        }
        public void Dispose()
        {
            lock (gate)
            {
                if (disposed) return;
                try { Disconnect(); }
                finally { disposed = true; GC.SuppressFinalize(this); }
            }
        }

        // ASCOM's installed Profile component handles the platform registry layout.
        [ComRegisterFunction]
        public static void Register(Type type) { RegisterProfile(true); }
        [ComUnregisterFunction]
        public static void Unregister(Type type) { RegisterProfile(false); }
        private static void RegisterProfile(bool register)
        {
            Type type = Type.GetTypeFromProgID("ASCOM.Utilities.Profile", true);
            dynamic profile = Activator.CreateInstance(type);
            try
            {
                profile.DeviceType = "Rotator";
                if (register) profile.Register(DriverId, DisplayName);
                else profile.Unregister(DriverId);
            }
            finally
            {
                object instance = profile;
                if (Marshal.IsComObject(instance)) Marshal.FinalReleaseComObject(instance);
                else (instance as IDisposable)?.Dispose();
            }
        }
    }
}
