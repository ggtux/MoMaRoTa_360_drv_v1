using System;
using System.Diagnostics;
using System.IO.Ports;
using System.Text;

namespace MoMaRoTa
{
    internal sealed class SerialChannel : ILineChannel
    {
        private readonly SerialPort port;
        private readonly StringBuilder partial = new StringBuilder();
        private bool overflow;
        internal SerialChannel(string name)
        {
            port = new SerialPort(name, 115200, Parity.None, 8, StopBits.One) {
                Handshake = Handshake.None, DtrEnable = false, RtsEnable = false,
                NewLine = "\n", Encoding = Encoding.ASCII, WriteTimeout = 2000, ReadTimeout = 3000
            };
            try { port.Open(); }
            catch { port.Dispose(); throw; }
        }
        public void WriteLine(string text) { port.WriteLine(text); }
        public string ReadLine(int timeoutMs)
        {
            var watch = Stopwatch.StartNew();
            while (watch.ElapsedMilliseconds < timeoutMs)
            {
                port.ReadTimeout = Math.Max(1, timeoutMs - (int)watch.ElapsedMilliseconds);
                char c = (char)port.ReadChar();
                if (c == '\n')
                {
                    string result = overflow ? "" : partial.ToString();
                    partial.Clear(); overflow = false;
                    return result;
                }
                if (c == '\r' || overflow) continue;
                if (partial.Length >= 2048) { partial.Clear(); overflow = true; }
                else partial.Append(c);
            }
            throw new TimeoutException("Serial response timed out.");
        }
        public void Dispose() { port.Dispose(); }
    }
}
