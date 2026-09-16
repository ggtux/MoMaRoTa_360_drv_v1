using System;
using System.Runtime.InteropServices;

namespace MoMaRoTa
{
    [ComImport]
    [ComVisible(false)]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("00000001-0000-0000-C000-000000000046")]
    public interface IClassFactory
    {
        void CreateInstance(IntPtr outer, ref Guid interfaceId, out IntPtr instance);
        void LockServer([MarshalAs(UnmanagedType.Bool)] bool locked);
    }

    // COM local servers must publish a class factory to the Service Control
    // Manager. This follows the class-factory sequence used by ASCOM 7's
    // official local-server template: register suspended, then resume atomically.
    public sealed class RotatorClassFactory : IClassFactory
    {
        private const uint LocalServerContext = 0x4;
        private const uint MultipleUse = 0x1;
        private const uint Suspended = 0x4;
        private const int NoAggregation = unchecked((int)0x80040110);
        private const int NoInterface = unchecked((int)0x80004002);
        private static readonly Guid UnknownId = new Guid("00000000-0000-0000-C000-000000000046");
        private static readonly Guid DispatchId = new Guid("00020400-0000-0000-C000-000000000046");
        private uint cookie;

        [DllImport("ole32.dll")]
        private static extern int CoRegisterClassObject(ref Guid classId,
            [MarshalAs(UnmanagedType.IUnknown)] object factory, uint context,
            uint flags, out uint registrationCookie);

        [DllImport("ole32.dll")]
        private static extern int CoRevokeClassObject(uint registrationCookie);

        [DllImport("ole32.dll")]
        internal static extern int CoResumeClassObjects();

        [DllImport("ole32.dll")]
        internal static extern int CoSuspendClassObjects();

        internal void Register()
        {
            Guid classId = typeof(Rotator).GUID;
            int result = CoRegisterClassObject(ref classId, this, LocalServerContext,
                MultipleUse | Suspended, out cookie);
            if (result < 0) Marshal.ThrowExceptionForHR(result);
        }

        internal void Revoke()
        {
            if (cookie == 0) return;
            int result = CoRevokeClassObject(cookie);
            cookie = 0;
            if (result < 0) Marshal.ThrowExceptionForHR(result);
        }

        void IClassFactory.CreateInstance(IntPtr outer, ref Guid interfaceId, out IntPtr instance)
        {
            instance = IntPtr.Zero;
            LocalServer.Log("COM CreateInstance entered, IID " + interfaceId.ToString("B"));
            if (outer != IntPtr.Zero) throw new COMException("Aggregation is not supported.", NoAggregation);

            LocalServer.Log("Creating managed Rotator instance");
            var driver = new Rotator();
            try
            {
                LocalServer.Log("Managed Rotator instance created; exporting requested interface");
                if (interfaceId == DispatchId)
                {
                    LocalServer.Log("Exporting IDispatch");
                    instance = Marshal.GetIDispatchForObject(driver);
                }
                else if (interfaceId == UnknownId)
                {
                    LocalServer.Log("Exporting IUnknown");
                    instance = Marshal.GetIUnknownForObject(driver);
                }
                else
                {
                    foreach (Type type in typeof(Rotator).GetInterfaces())
                    {
                        if (interfaceId != Marshal.GenerateGuidForType(type)) continue;
                        LocalServer.Log("Exporting interface " + type.FullName);
                        instance = Marshal.GetComInterfaceForObject(driver, type);
                        break;
                    }
                    if (instance == IntPtr.Zero) throw new COMException("Interface is not supported.", NoInterface);
                }
                LocalServer.Log("COM Rotator interface exported for IID " + interfaceId.ToString("B"));
            }
            catch
            {
                driver.Dispose();
                throw;
            }
        }

        void IClassFactory.LockServer(bool locked)
        {
            LocalServer.ChangeServerLock(locked);
        }
    }
}
