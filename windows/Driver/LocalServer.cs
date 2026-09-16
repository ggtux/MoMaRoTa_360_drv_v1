using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32;

namespace MoMaRoTa
{
    // Windows activates this EXE through LocalServer32, never loads the driver
    // into NINA's .NET 8 process. Both bitnesses use this same COM server.
    internal static class LocalServer
    {
        private static readonly object countGate = new object();
        private static int objectCount;
        private static int serverLockCount;
        private static bool startedByCom;
        private static uint mainThreadId;

        [DllImport("kernel32.dll")]
        private static extern uint GetCurrentThreadId();

        [DllImport("user32.dll")]
        private static extern bool PostThreadMessage(uint threadId, uint message, UIntPtr wParam, IntPtr lParam);

        internal static string ExecutablePath => Assembly.GetExecutingAssembly().Location;
        private static string ClassId => typeof(Rotator).GUID.ToString("B").ToUpperInvariant();
        private const string AppId = "{D3129EF7-695E-45B9-87BA-FEC80E04A83E}";

        [STAThread]
        private static int Main(string[] args)
        {
            try
            {
                if (HasArgument(args, "/regserver")) { RegisterServer(); return 0; }
                if (HasArgument(args, "/unregserver")) { UnregisterServer(); return 0; }
                if (HasArgument(args, "/checkregistration")) { CheckRegistration(); return 0; }
                startedByCom = HasArgument(args, "-embedding") || HasArgument(args, "/embedding");
                mainThreadId = GetCurrentThreadId();
                Thread.CurrentThread.Name = "Astro Orbit Local Server Thread";
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                var factory = new RotatorClassFactory();
                Log("Registering COM class factory; startedByCOM=" + startedByCom);
                factory.Register();
                try
                {
                    int resumeResult = RotatorClassFactory.CoResumeClassObjects();
                    if (resumeResult < 0) Marshal.ThrowExceptionForHR(resumeResult);
                    Log("Official ASCOM server model 1.4.2 x86 ready, process " + System.Diagnostics.Process.GetCurrentProcess().Id);
                    var garbageCollection = new System.Windows.Forms.Timer { Interval = 10000 };
                    garbageCollection.Tick += (_, __) => GC.Collect();
                    garbageCollection.Start();
                    Application.Run();
                    garbageCollection.Dispose();
                }
                finally
                {
                    RotatorClassFactory.CoSuspendClassObjects();
                    factory.Revoke();
                    ServerObjects.DisposeAll();
                    Log("LocalServer stopped");
                }
                return 0;
            }
            catch (Exception ex)
            {
                Log(ex.ToString());
                // Registration/installer invocations get a nonzero exit code;
                // COM activation failures are recorded without a blocking dialog.
                return 1;
            }
        }
        internal static void IncrementObjectCount()
        {
            lock (countGate) { objectCount++; Log("Object count: " + objectCount); }
        }
        internal static void DecrementObjectCount()
        {
            lock (countGate) { objectCount--; Log("Object count: " + objectCount); }
            ExitIfUnused();
        }
        internal static void ChangeServerLock(bool increment)
        {
            lock (countGate)
            {
                serverLockCount += increment ? 1 : -1;
                Log("Server lock count: " + serverLockCount);
            }
            ExitIfUnused();
        }
        private static void ExitIfUnused()
        {
            lock (countGate)
                if (startedByCom && objectCount <= 0 && serverLockCount <= 0)
                    PostThreadMessage(mainThreadId, 0x0012, UIntPtr.Zero, IntPtr.Zero);
        }
        private static bool HasArgument(string[] args, string value)
        {
            foreach (string arg in args)
                if (string.Equals(arg, value, StringComparison.OrdinalIgnoreCase)) return true;
            return false;
        }
        private static RegistryView[] Views => Environment.Is64BitOperatingSystem
            ? new[] { RegistryView.Registry32, RegistryView.Registry64 }
            : new[] { RegistryView.Registry32 };
        private static void RequireAdministrator()
        {
            using (var identity = WindowsIdentity.GetCurrent())
                if (!new WindowsPrincipal(identity).IsInRole(WindowsBuiltInRole.Administrator))
                    throw new UnauthorizedAccessException("Register/unregister requires administrator rights.");
        }
        private static void RegisterServer()
        {
            RequireAdministrator();
            foreach (var view in Views)
            {
                using (var machine = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, view))
                using (var classes = machine.OpenSubKey(@"SOFTWARE\Classes", true))
                {
                    // Replace only this driver's old DLL registration. In particular
                    // no InprocServer32 / mscoree entry may survive the migration.
                    classes.DeleteSubKeyTree(@"CLSID\" + ClassId, false);
                    using (var cls = classes.CreateSubKey(@"CLSID\" + ClassId))
                    {
                        cls.SetValue("", Rotator.DisplayName);
                        cls.SetValue("AppId", AppId);
                        using (var progId = cls.CreateSubKey("ProgId")) progId.SetValue("", Rotator.DriverId);
                        using (var categories = cls.CreateSubKey("Implemented Categories"))
                            categories.CreateSubKey("{62C8FE65-4EBB-45E7-B440-6E39B2CDBF29}").Dispose();
                        cls.CreateSubKey("Programmable").Dispose();
                        using (var server = cls.CreateSubKey("LocalServer32"))
                        {
                            server.SetValue("", ExecutablePath);
                            server.SetValue("ServerExecutable", ExecutablePath);
                        }
                    }
                    classes.DeleteSubKeyTree(Rotator.DriverId, false);
                    using (var prog = classes.CreateSubKey(Rotator.DriverId))
                    {
                        prog.SetValue("", Rotator.DisplayName);
                        using (var cls = prog.CreateSubKey("CLSID")) cls.SetValue("", ClassId);
                    }
                    using (var app = classes.CreateSubKey(@"AppID\" + AppId))
                    {
                        app.SetValue("", "Astro Orbit USB ASCOM LocalServer");
                        app.SetValue("AppID", AppId);
                        app.SetValue("AuthenticationLevel", 1, RegistryValueKind.DWord);
                        app.SetValue("RunAs", "Interactive User", RegistryValueKind.String);
                    }
                    using (var executable = classes.CreateSubKey(@"AppID\" + Path.GetFileName(ExecutablePath)))
                        executable.SetValue("AppID", AppId);
                }
            }
            Rotator.Register(typeof(Rotator));
            CheckRegistration();
            Log("Registered isolated COM LocalServer in both Windows registry views");
        }
        private static void UnregisterServer()
        {
            RequireAdministrator();
            Rotator.Unregister(typeof(Rotator));
            foreach (var view in Views)
            {
                using (var machine = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, view))
                using (var classes = machine.OpenSubKey(@"SOFTWARE\Classes", true))
                {
                    classes.DeleteSubKeyTree(@"CLSID\" + ClassId, false);
                    classes.DeleteSubKeyTree(Rotator.DriverId, false);
                    classes.DeleteSubKeyTree(@"AppID\" + Path.GetFileName(ExecutablePath), false);
                    classes.DeleteSubKeyTree(@"AppID\" + AppId, false);
                }
            }
            Log("Unregistered COM LocalServer");
        }
        private static void CheckRegistration()
        {
            foreach (var view in Views)
            {
                using (var machine = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, view))
                using (var cls = machine.OpenSubKey(@"SOFTWARE\Classes\CLSID\" + ClassId))
                {
                    if (cls == null) throw new InvalidOperationException("Missing CLSID in " + view);
                    using (var inproc = cls.OpenSubKey("InprocServer32"))
                        if (inproc != null) throw new InvalidOperationException("Old in-process DLL registration remains in " + view);
                    using (var server = cls.OpenSubKey("LocalServer32"))
                        if (!string.Equals(server?.GetValue("ServerExecutable") as string, ExecutablePath, StringComparison.OrdinalIgnoreCase))
                            throw new InvalidOperationException("Wrong LocalServer32 path in " + view);
                }
            }
        }
        internal static void Log(string text)
        {
            try
            {
                string folder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Astro Orbit", "Logs");
                Directory.CreateDirectory(folder);
                File.AppendAllText(Path.Combine(folder, "LocalServer-" + DateTime.Today.ToString("yyyyMMdd") + ".log"),
                    DateTime.Now.ToString("O") + " " + text + Environment.NewLine);
            }
            catch { /* Logging must never terminate the hardware server. */ }
        }
    }

    [ComVisible(false)]
    public class ReferenceCountedObjectBase
    {
        protected ReferenceCountedObjectBase() { LocalServer.IncrementObjectCount(); }
        ~ReferenceCountedObjectBase() { LocalServer.DecrementObjectCount(); }
    }
    internal static class ServerObjects
    {
        private static readonly object gate = new object();
        private static readonly List<WeakReference> objects = new List<WeakReference>();
        internal static void Track(Rotator driver)
        {
            lock (gate) { objects.RemoveAll(item => !item.IsAlive); objects.Add(new WeakReference(driver)); }
        }
        internal static void DisposeAll()
        {
            lock (gate)
            {
                foreach (var reference in objects)
                {
                    var driver = reference.Target as Rotator;
                    try { driver?.Dispose(); } catch (Exception ex) { LocalServer.Log(ex.ToString()); }
                }
                objects.Clear();
            }
        }
    }
}
