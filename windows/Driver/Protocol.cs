using System;
using System.Diagnostics;
using System.Threading;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MoMaRoTa
{
    // This file is also compiled into the hardware-free protocol tests.
    internal interface ILineChannel : IDisposable
    {
        void WriteLine(string text);
        string ReadLine(int timeoutMs);
    }

    internal sealed class DeviceError : Exception
    {
        public int Code { get; }
        public DeviceError(int code, string message) : base(message) { Code = code; }
    }

    internal sealed class Protocol
    {
        internal const string Prefix = "@MOROTA ";
        private readonly ILineChannel channel;
        private readonly string session = Guid.NewGuid().ToString("N");
        private string boot;
        internal Protocol(ILineChannel channel) { this.channel = channel; }

        internal JObject Exchange(string command, object value = null, int timeoutMs = 3000)
        {
            string id = Guid.NewGuid().ToString("N");
            var request = new JObject { ["id"] = id, ["cmd"] = command, ["session"] = session };
            if (value != null) request["value"] = JToken.FromObject(value);
            channel.WriteLine(Prefix + request.ToString(Formatting.None));
            var watch = Stopwatch.StartNew();
            while (watch.ElapsedMilliseconds < timeoutMs)
            {
                string line = channel.ReadLine(Math.Max(1, timeoutMs - (int)watch.ElapsedMilliseconds));
                int marker = line.IndexOf(Prefix, StringComparison.Ordinal);
                if (marker < 0) continue; // Boot messages and debug logs are not replies.
                JObject reply;
                try { reply = JObject.Parse(line.Substring(marker + Prefix.Length)); }
                catch (JsonException) { continue; }
                if ((string)reply["id"] != id) continue; // Ignore old/foreign transactions.
                string receivedBoot = (string)reply["boot"];
                if (string.IsNullOrEmpty(receivedBoot)) throw new InvalidOperationException("Missing controller boot identifier.");
                if (boot != null && boot != receivedBoot)
                    throw new InvalidOperationException("Controller restarted. Reconnect and perform a new plate-solve Sync.");
                if (reply["error"]?.Type != JTokenType.Integer)
                    throw new InvalidOperationException("Invalid protocol error field.");
                int error = (int)reply["error"];
                if (error != 0) throw new DeviceError(error, (string)reply["message"] ?? "Controller error");
                if (command == "hello")
                {
                    var hello = reply["value"] as JObject;
                    if ((string)hello?["device"] != "MoMaRoTa" || (int?)hello?["protocol"] != 1)
                        throw new InvalidOperationException("This is not a compatible Astro Orbit USB firmware (protocol 1).");
                    boot = receivedBoot;
                }
                return reply["value"] as JObject ?? new JObject();
            }
            throw new TimeoutException("No matching Astro Orbit response. The command was not retried; its execution state may be unknown.");
        }

        internal void Handshake(int timeoutMs = 60000)
        {
            // Only Hello is retried. Never automatically replay a movement.
            var watch = Stopwatch.StartNew();
            while (true)
            {
                try { Exchange("hello", timeoutMs: Math.Min(1500, Math.Max(1, timeoutMs - (int)watch.ElapsedMilliseconds))); return; }
                catch (TimeoutException)
                {
                    if (watch.ElapsedMilliseconds >= timeoutMs) throw;
                    Thread.Sleep(100);
                }
            }
        }
    }

    internal sealed class RotatorStatus
    {
        internal float Position, Mechanical, Target, StepSize;
        internal bool Moving, Reverse, MotorHealthy;
        internal string MotionError;
        internal static RotatorStatus Parse(JObject value)
        {
            return new RotatorStatus {
                Position = Angle(value, "position"), Mechanical = Angle(value, "mechanical"),
                Target = Angle(value, "target"), StepSize = Number(value, "stepSize"),
                Moving = Boolean(value, "moving"), Reverse = Boolean(value, "reverse"),
                MotorHealthy = Boolean(value, "motorHealthy"),
                MotionError = value["motionError"]?.Type == JTokenType.String ? (string)value["motionError"] : throw new InvalidOperationException("Missing motor error status.")
            };
        }
        private static bool Boolean(JObject value, string name)
        {
            if (value[name]?.Type != JTokenType.Boolean) throw new InvalidOperationException("Invalid status: " + name);
            return (bool)value[name];
        }
        private static float Number(JObject value, string name)
        {
            var token = value[name];
            if (token == null || (token.Type != JTokenType.Float && token.Type != JTokenType.Integer))
                throw new InvalidOperationException("Invalid status: " + name);
            float number = (float)token;
            if (float.IsNaN(number) || float.IsInfinity(number)) throw new InvalidOperationException("Non-finite status: " + name);
            return number;
        }
        private static float Angle(JObject value, string name)
        {
            float angle = Number(value, name);
            if (angle < 0 || angle >= 360) throw new InvalidOperationException("Invalid angle: " + name);
            return angle;
        }
    }
}
