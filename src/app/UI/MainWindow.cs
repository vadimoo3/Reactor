// Главное окно: состояние, загрузка параметров, общие элементы, каркас и верхняя строка.
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
    partial class MainWindow : Window
    {
        readonly string root = Program.Root;
        string Results { get { return Path.Combine(root, "results"); } }
        string IconDir { get { return Path.Combine(root, "icons"); } }
        string IniPath { get { return Path.Combine(root, "ReactorLab.ini"); } }

        List<Param> pars = new List<Param>(); List<Tool> tools = new List<Tool>(); Dictionary<string, string> groupTitle = new Dictionary<string, string>();
        Dictionary<string, string> val = new Dictionary<string, string>();
        class Stage { public string Label, Tool, Dir, Need; public bool Pick; public Gen Gen; public Func<List<Stage>> Expand; public Dictionary<string, string> Env = new Dictionary<string, string>(); }
        // Генератор кандидатов для адаптивного распределения времени: сколько попыток в секунду и как заполнить параметры на заданное время.
        class Gen { public string Id, Label, Tool; public Action<Dictionary<string, string>, double> Fill; public double Score = -1, Share; }
        // Лучший результат поиска для переключения стрелками: файл, уровень надёжности, чистая мощность, производство, цена, заполнение.
        class Best { public string File; public int Tier; public double Net, Power, Cost, Fill; public string Src; }
        static readonly string[][] Methods = {
            new[] { "blocks", "Блоки", "Все готовые конструкции архива: блоки Protium с доводкой, модули 3×3, прямые схемы, прибыльные группы с валидатором, ранняя модель. Лучшее доводится отжигом." },
            new[] { "random", "Полный рандом", "Случайные сетки и мутации эталона, лучший кандидат доводится отжигом." },
            new[] { "improve", "Улучшение", "Доводит схему, выбранную в списке. Хуже исходной не станет." },
            new[] { "tile", "Блоками", "Находит в выбранной схеме (например, после полного рандома) повторяющийся блок и строит из него упорядоченную рабочую схему." } };
        static readonly int[] TimePresets = { 5, 60, 300 };
        Dictionary<string, string> EnvFor(string fuel) { var e = Env(); foreach (var kv in FuelCatalog.Env(fuel)) e[kv.Key] = kv.Value; return e; }

        Dictionary<string, ToggleButton> methodBtn = new Dictionary<string, ToggleButton>(); StackPanel formHost;
        Button goBtn; TextBlock status, saveSummary; ProgressBar prog; TextBox log; ToggleButton logToggle;
        ListBox library; List<Layout> layouts = new List<Layout>(); string selPath; bool rebuilding, forceScan;
        Border undoBar; TextBlock undoText; string undoFrom, undoTo; DispatcherTimer undoTimer; TextBox renameBox; ToggleButton favBtn;
        GridView gridView; TextBlock viewTitle, viewSub, hoverInfo; WrapPanel stats, counts; ContentControl testArea; StackPanel evalArea;
        Process evalProc; DispatcherTimer evalDelay;
        Border drawer; StackPanel drawerBody;
        Process proc, testProc; DateTime started, stageStarted; DispatcherTimer timer, testDelay;
        List<Stage> stages; int stageIdx; bool aborted; string methodDir, workDir, finalFile, baseFile, pickInfo, bestLive;
        List<Best> runBest = new List<Best>(); Layout compareWith;
        Dictionary<string, List<string>> testCache = new Dictionary<string, List<string>>();

        static T Res<T>(string k) { return (T)Application.Current.Resources[k]; }
        static Brush B(string k) { return Res<Brush>(k); }

        public MainWindow()
        {
            Title = "Reactor Lab"; Width = 1560; Height = 940; MinWidth = 1200; MinHeight = 740; Background = B("Bg");
            FontFamily = new FontFamily("Segoe UI"); FontSize = 13.5; WindowStartupLocation = WindowStartupLocation.CenterScreen;
            LoadSchema();
            val["uiMethod"] = "blocks"; val["uiTime"] = "60"; val["uiCountOn"] = "0"; val["uiCount"] = "6"; val["uiFuel"] = "protium"; val["uiFav"] = ""; val["uiSort"] = "new";
            LoadIni(); ApplyIconConfig(); Parts.LoadIcons(IconDir);
            Content = BuildShell();
            BuildForm();
            testDelay = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(350) }; testDelay.Tick += (s, e) => { testDelay.Stop(); RunTests(); };
            evalDelay = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(200) }; evalDelay.Tick += (s, e) => { evalDelay.Stop(); RunEval(); };
            ScanLayouts(null, null);
            timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(500) }; timer.Tick += (s, e) => { if (proc != null) ShowProgress(); }; timer.Start();
            Activated += (s, e) => { Parts.LoadIcons(IconDir); if (proc == null) ScanLayouts(null, null); gridView.InvalidateVisual(); };
            PreviewKeyDown += OnKey;
            Closing += (s, e) => { SaveIni(); Kill(proc); Kill(testProc); };
            if (Program.SelfTestDir != null) PrepareSelfTest();
        }

        // ---------- данные ----------
        void LoadSchema()
        {
            foreach (var line in Engine.Run("--params").Split('\n'))
            {
                var f = line.TrimEnd('\r').Split('\t');
                if (f[0] == "G" && f.Length >= 3) groupTitle[f[1]] = f[2];
                else if (f[0] == "T" && f.Length >= 6) tools.Add(new Tool { Id = f[1], Title = f[2], Dir = f[3], Groups = f[4].Split(','), Desc = f[5] });
                else if (f[0] == "P" && f.Length >= 7) pars.Add(new Param { Grp = f[1], Key = f[2], Label = f[3], Def = f[4], Kind = f[5][0], Opts = f[6] });
            }
            foreach (var p in pars) val[p.Key] = p.Def;
        }
        void LoadIni()
        {
            if (!File.Exists(IniPath)) return;
            foreach (var l in File.ReadAllLines(IniPath, Encoding.UTF8)) { int e = l.IndexOf('='); if (e > 0) { string k = l.Substring(0, e); if (val.ContainsKey(k)) val[k] = l.Substring(e + 1); } }
        }
        void SaveIni() { try { File.WriteAllLines(IniPath, val.Select(kv => kv.Key + "=" + kv.Value), new UTF8Encoding(false)); } catch { } }
        Dictionary<string, string> Env() { return val.Where(kv => !kv.Key.StartsWith("ui") && !kv.Key.StartsWith("ic")).ToDictionary(kv => kv.Key, kv => kv.Value); }

        // ---------- элементы ----------
        static TextBlock Glyph(string code, double size = 13, Brush fg = null)
        { var t = new TextBlock { Text = code, FontFamily = Res<FontFamily>("Glyph"), FontSize = size, VerticalAlignment = VerticalAlignment.Center }; if (fg != null) t.Foreground = fg; return t; }
        static TextBlock Text(string t, double size = 13.5, string brush = "Text", FontWeight? w = null)
        { return new TextBlock { Text = t, FontSize = size, Foreground = B(brush), TextWrapping = TextWrapping.Wrap, FontWeight = w ?? FontWeights.Normal }; }
        static Button Btn(string text, string style = null, string glyph = null)
        {
            object content = text;
            if (glyph != null)
            {
                var sp = new StackPanel { Orientation = Orientation.Horizontal };
                var g = Glyph(glyph); var t = new TextBlock { Text = text, Margin = new Thickness(text == null ? 0 : 8, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center };
                foreach (var tb in new[] { g, t }) tb.SetBinding(TextBlock.ForegroundProperty, new Binding("Foreground") { RelativeSource = new RelativeSource(RelativeSourceMode.FindAncestor, typeof(Button), 1) });
                sp.Children.Add(g); if (text != null) sp.Children.Add(t); content = sp;
            }
            var b = new Button { Content = content }; if (style != null) b.Style = Res<Style>(style); return b;
        }
        static Border Divider(Thickness m) { return new Border { Height = 1, Background = B("Line"), Margin = m }; }

        UIElement BuildShell()
        {
            var g = new Grid { Background = B("Bg") };
            g.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            g.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) });
            g.Children.Add(BuildTopBar());
            var body = new Grid(); Grid.SetRow(body, 1); g.Children.Add(body);
            body.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(400) });
            body.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            var left = BuildLeft(); body.Children.Add(left);
            var right = BuildLayoutView(); Grid.SetColumn(right, 1); body.Children.Add(right);
            drawer = BuildDrawer(); Grid.SetRow(drawer, 1); g.Children.Add(drawer);
            return g;
        }

        UIElement BuildTopBar()
        {
            var bar = new DockPanel { Background = B("Side"), LastChildFill = false };
            var title = new TextBlock { Text = "Reactor Lab", FontSize = 17, FontWeight = FontWeights.SemiBold, Margin = new Thickness(20, 12, 20, 12), VerticalAlignment = VerticalAlignment.Center };
            bar.Children.Add(title);
            var save = Btn(null, "Ghost", ""); save.Margin = new Thickness(0, 8, 14, 8);
            var sp = (StackPanel)save.Content; saveSummary = new TextBlock { Margin = new Thickness(8, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center };
            saveSummary.SetBinding(TextBlock.ForegroundProperty, new Binding("Foreground") { RelativeSource = new RelativeSource(RelativeSourceMode.FindAncestor, typeof(Button), 1) });
            sp.Children.Add(saveSummary); save.ToolTip = "Ваш сейв: уровни улучшений, бюджет, цены. Здесь же константы игры.";
            save.Click += (s, e) => ToggleDrawer();
            DockPanel.SetDock(save, Dock.Right); bar.Children.Add(save);
            UpdateSaveSummary();
            var wrap = new Border { BorderBrush = B("Line"), BorderThickness = new Thickness(0, 0, 0, 1), Child = bar };
            return wrap;
        }
        void UpdateSaveSummary()
        {
            Func<string, string> v = k => val.ContainsKey(k) ? val[k] : "?";
            saveSummary.Text = "Бюджет " + v("budget").Split('.')[0] + "," + (v("budget") + ".00").Split('.')[1].Substring(0, 2) + " Qa      Exch " + v("bEx") + "   Vent " + v("bAv") + "   Sink " + v("bHs") + "   Reinf " + v("bRe") + "   Chrono " + v("bCh");
        }

    }
}
