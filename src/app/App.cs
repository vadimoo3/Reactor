// Точка входа: тема, распаковка движка, окно. --selftest <папка> — самопроверка на копии данных в окне за пределами экрана.
using System;
using System.IO;
using System.Reflection;
using System.Windows;
using System.Windows.Markup;

namespace ReactorLab
{
    static class Program
    {
        public static string Root = AppDomain.CurrentDomain.BaseDirectory.TrimEnd('\\');
        public static string SelfTestDir;

        [STAThread]
        static int Main(string[] args)
        {
            if (args.Length >= 2 && args[0] == "--selftest") { SelfTestDir = Path.GetFullPath(args[1]); Root = SelfTestDir; }
            var app = new Application { ShutdownMode = ShutdownMode.OnMainWindowClose };
            app.DispatcherUnhandledException += (s, e) =>
            {
                if (SelfTestDir != null) { File.AppendAllText(Path.Combine(SelfTestDir, "selftest-report.txt"), "CRASH " + e.Exception + "\r\n"); app.Shutdown(99); }
                else MessageBox.Show(e.Exception.ToString(), "Reactor Lab");
                e.Handled = true;
            };
            using (var s = Assembly.GetExecutingAssembly().GetManifestResourceStream("Theme.xaml")) app.Resources = (ResourceDictionary)XamlReader.Load(s);
            try { Engine.Extract(); } catch (Exception ex) { MessageBox.Show("Не удалось распаковать движок: " + ex.Message, "Reactor Lab"); return 1; }
            MainWindow w;
            try { w = new MainWindow(); }
            catch (Exception ex) { if (SelfTestDir != null) File.WriteAllText(Path.Combine(SelfTestDir, "selftest-report.txt"), "CRASH " + ex); else MessageBox.Show(ex.ToString(), "Reactor Lab"); return 1; }
            return app.Run(w);
        }
    }
}
