using System;
using System.Runtime.InteropServices;

internal static class Program
{
    // Deliberately no driver/ASCOM assembly references: activate the installed
    // server through COM, from the modern runtime also used by NINA 3.
    [STAThread]
    private static int Main(string[] args)
    {
        if (!OperatingSystem.IsWindows()) { Console.Error.WriteLine("Run this test on Windows."); return 1; }
        dynamic driver = null;
        int result = 0;
        try
        {
            driver = Activator.CreateInstance(Type.GetTypeFromProgID("ASCOM.MoMaRoTa.Rotator", true));
            Console.WriteLine("Client runtime: " + RuntimeInformation.FrameworkDescription);
            Console.WriteLine("Name: " + driver.Name + ", driver " + driver.DriverVersion + ", interface " + driver.InterfaceVersion);
            if (driver.DriverVersion != "1.4.2" || driver.InterfaceVersion != 3)
                throw new InvalidOperationException("Install Astro Orbit 1.4.2 and restart the server first.");
            if (!Marshal.IsComObject((object)driver)) throw new InvalidOperationException("Expected COM wrapper.");
            if (Array.Exists(args, item => item == "--connect"))
            {
                driver.Connected = true; // Uses the COM port saved in Setup. No movement.
                Console.WriteLine("Position: " + driver.Position + ", mechanical: " + driver.MechanicalPosition + ", moving: " + driver.IsMoving);
                Console.WriteLine("Target: " + driver.TargetPosition + ", step: " + driver.StepSize + ", reverse supported: " + driver.CanReverse);
            }
            Console.WriteLine(".NET 8 COM calls passed.");
        }
        catch (Exception ex) { Console.Error.WriteLine(ex); result = 1; }
        finally
        {
            if (driver != null)
            {
                try { if (driver.Connected) driver.Connected = false; }
                catch (Exception ex) { Console.Error.WriteLine("Disconnect failed: " + ex); result = 1; }
                finally { if (Marshal.IsComObject((object)driver)) Marshal.FinalReleaseComObject((object)driver); }
            }
        }
        return result;
    }
}
