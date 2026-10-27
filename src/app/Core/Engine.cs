// Вшитый движок ReactorEngine.exe: распаковка и запуск; параметры передаются в RX_<key>.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;

namespace ReactorLab
{
    static class Engine
    {
        public static string Exe;
        public static void Extract()
        {
            byte[] b;
            using (var s = Assembly.GetExecutingAssembly().GetManifestResourceStream("ReactorEngine.exe"))
            { b = new byte[s.Length]; int o = 0; while (o < b.Length) o += s.Read(b, o, b.Length - o); }
            string hash; using (var md5 = MD5.Create()) hash = BitConverter.ToString(md5.ComputeHash(b)).Replace("-", "").Substring(0, 12);
            string dir = Program.SelfTestDir != null ? Path.Combine(Program.SelfTestDir, "engine") : Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ReactorLab");
            Directory.CreateDirectory(dir);
            Exe = Path.Combine(dir, "engine-" + hash + ".exe");
            if (!File.Exists(Exe)) File.WriteAllBytes(Exe, b);
        }
        public static Process Start(string args, string cwd, IDictionary<string, string> env, Action<string> line, Action<int> exit)
        {
            var si = new ProcessStartInfo(Exe, args) { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true, StandardOutputEncoding = Encoding.UTF8, StandardErrorEncoding = Encoding.UTF8, WorkingDirectory = cwd };
            foreach (var kv in env) si.EnvironmentVariables["RX_" + kv.Key] = kv.Value.Replace(',', '.');
            var p = new Process { StartInfo = si, EnableRaisingEvents = true };
            var disp = Dispatcher.CurrentDispatcher;
            p.OutputDataReceived += (s, e) => { if (e.Data != null) disp.BeginInvoke(line, e.Data); };
            p.ErrorDataReceived += (s, e) => { if (e.Data != null) disp.BeginInvoke(line, e.Data); };
            p.Exited += (s, e) => { p.WaitForExit(); disp.BeginInvoke(exit, p.ExitCode); };
            p.Start(); p.BeginOutputReadLine(); p.BeginErrorReadLine();
            try { p.PriorityClass = ProcessPriorityClass.BelowNormal; } catch { }
            return p;
        }
        public static string Run(string args)
        {
            var si = new ProcessStartInfo(Exe, args) { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, StandardOutputEncoding = Encoding.UTF8 };
            using (var p = Process.Start(si)) { string o = p.StandardOutput.ReadToEnd(); p.WaitForExit(); return o; }
        }
    }
}
