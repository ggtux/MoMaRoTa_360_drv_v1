using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using MoMaRoTa;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

internal sealed class FakeChannel : ILineChannel
{
    internal readonly Queue<string> Lines = new Queue<string>();
    internal readonly List<JObject> Requests = new List<JObject>();
    internal Action<JObject> OnWrite;
    public void WriteLine(string text)
    {
        if (text.Length > 255) throw new Exception("Request exceeds firmware buffer.");
        var request = JObject.Parse(text.Substring(Protocol.Prefix.Length));
        Requests.Add(request); OnWrite?.Invoke(request);
    }
    public string ReadLine(int timeoutMs)
    {
        if (Lines.Count == 0) throw new TimeoutException();
        return Lines.Dequeue();
    }
    public void Dispose() { }
    internal void Reply(JObject request, JObject value = null, int error = 0, string boot = "0123456789abcdef")
    {
        Lines.Enqueue(Protocol.Prefix + new JObject {
            ["id"] = request["id"], ["boot"] = boot, ["error"] = error,
            ["message"] = "test error", ["value"] = value ?? new JObject()
        }.ToString(Formatting.None));
    }
}
internal static class Program
{
    private static int passed;
    private static void Test(string name, Action test) { test(); passed++; Console.WriteLine("PASS " + name); }
    private static void Assert(bool condition) { if (!condition) throw new Exception("Assertion failed"); }
    private static T Throws<T>(Action action) where T : Exception
    {
        try { action(); } catch (T ex) { return ex; }
        throw new Exception("Expected " + typeof(T).Name);
    }
    private static JObject Hello() => new JObject { ["device"] = "MoMaRoTa", ["protocol"] = 1 };
    private static JObject Status() => new JObject { ["position"] = 15.5, ["mechanical"] = 5.5, ["target"] = 20,
        ["stepSize"] = 0.0399502841, ["moving"] = true, ["reverse"] = false, ["motorHealthy"] = true, ["motionError"] = "" };
    private static void Main()
    {
        Test("debug, malformed JSON and stale replies are ignored", () => {
            var c = new FakeChannel(); c.OnWrite = q => {
                c.Lines.Enqueue("booting..."); c.Lines.Enqueue("@MOROTA {bad json");
                c.Lines.Enqueue("@MOROTA {\"id\":\"old\"}"); c.Reply(q, Hello());
            };
            new Protocol(c).Handshake(50);
        });
        Test("protocol prefix after an unfinished debug line", () => {
            var c = new FakeChannel(); c.OnWrite = q => { c.Reply(q, Hello()); c.Lines.Enqueue("unused"); var reply = c.Lines.Dequeue(); c.Lines.Clear(); c.Lines.Enqueue("Debug: " + reply); };
            new Protocol(c).Handshake(50);
        });
        Test("wrong device rejected", () => {
            var c = new FakeChannel(); c.OnWrite = q => c.Reply(q, new JObject { ["device"] = "Other", ["protocol"] = 1 });
            Throws<InvalidOperationException>(() => new Protocol(c).Handshake(50));
        });
        Test("unsupported protocol rejected", () => {
            var c = new FakeChannel(); c.OnWrite = q => c.Reply(q, new JObject { ["device"] = "MoMaRoTa", ["protocol"] = 2 });
            Throws<InvalidOperationException>(() => new Protocol(c).Handshake(50));
        });
        Test("controller reboot detected", () => {
            var c = new FakeChannel(); c.OnWrite = q => c.Reply(q, Hello()); var p = new Protocol(c); p.Handshake(50);
            c.OnWrite = q => c.Reply(q, Status(), boot: "different-boot");
            Throws<InvalidOperationException>(() => p.Exchange("status"));
        });
        Test("device error retained", () => {
            var c = new FakeChannel(); c.OnWrite = q => c.Reply(q, error: 1031);
            Assert(Throws<DeviceError>(() => new Protocol(c).Exchange("status")).Code == 1031);
        });
        Test("movement timeout never replays movement", () => {
            var c = new FakeChannel(); var p = new Protocol(c);
            Throws<TimeoutException>(() => p.Exchange("move", 5)); Assert(c.Requests.Count == 1);
        });
        Test("Hello alone is retried after timeout", () => {
            var c = new FakeChannel(); c.OnWrite = q => { if (c.Requests.Count == 2) c.Reply(q, Hello()); };
            new Protocol(c).Handshake(1000); Assert(c.Requests.Count == 2);
        });
        Test("session stable, request IDs unique", () => {
            var c = new FakeChannel(); c.OnWrite = q => c.Reply(q);
            var p = new Protocol(c); p.Exchange("connect"); p.Exchange("move", 5);
            Assert((string)c.Requests[0]["session"] == (string)c.Requests[1]["session"]);
            Assert((string)c.Requests[0]["id"] != (string)c.Requests[1]["id"]);
            Assert(((string)c.Requests[0]["session"]).Length == 32);
        });
        Test("German locale uses numeric JSON with decimal point", () => {
            var old = CultureInfo.CurrentCulture; CultureInfo.CurrentCulture = CultureInfo.GetCultureInfo("de-DE");
            try { var c = new FakeChannel(); c.OnWrite = q => c.Reply(q); new Protocol(c).Exchange("absolute", 12.5f);
                Assert(c.Requests.Single()["value"].Type == JTokenType.Float); Assert((double)c.Requests.Single()["value"] == 12.5); }
            finally { CultureInfo.CurrentCulture = old; }
        });
        Test("status coordinates and movement parsed", () => {
            var s = RotatorStatus.Parse(Status()); Assert(s.Moving && s.MotorHealthy && s.Position == 15.5f && s.Mechanical == 5.5f && s.Target == 20);
        });
        Test("missing status field rejected", () => { var s = Status(); s.Remove("moving"); Throws<InvalidOperationException>(() => RotatorStatus.Parse(s)); });
        Test("nonfinite status rejected", () => { var s = Status(); s["position"] = double.NaN; Throws<InvalidOperationException>(() => RotatorStatus.Parse(s)); });
        Test("out-of-range status rejected", () => { var s = Status(); s["mechanical"] = 360; Throws<InvalidOperationException>(() => RotatorStatus.Parse(s)); });
        Test("motor failure stays visible", () => { var s = Status(); s["motionError"] = "Motor stalled"; s["motorHealthy"] = false;
            var parsed = RotatorStatus.Parse(s); Assert(parsed.MotionError == "Motor stalled" && !parsed.MotorHealthy); });
        Console.WriteLine($"{passed} protocol tests passed.");
    }
}
