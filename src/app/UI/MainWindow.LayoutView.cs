// Выбранная схема: сетка, цифры, мгновенный пересчёт, автопроверка симуляцией.
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
    partial class MainWindow
    {
        // ---------- выбранная схема ----------
        UIElement BuildLayoutView()
        {
            var g = new Grid { Margin = new Thickness(28, 18, 28, 18) };
            foreach (var h in new[] { GridLength.Auto, new GridLength(1, GridUnitType.Star), GridLength.Auto, GridLength.Auto, GridLength.Auto }) g.RowDefinitions.Add(new RowDefinition { Height = h });
            var head = new DockPanel();
            var acts = new StackPanel { Orientation = Orientation.Horizontal, VerticalAlignment = VerticalAlignment.Top };
            favBtn = new ToggleButton { Style = Res<Style>("Seg"), Content = Glyph("\uE734", 14), ToolTip = "Избранное" };
            favBtn.Checked += (s, e) => { SetFav(true); favBtn.Content = Glyph("\uE735", 14, B("Warn")); }; favBtn.Unchecked += (s, e) => { SetFav(false); favBtn.Content = Glyph("\uE734", 14); };
            acts.Children.Add(favBtn);
            foreach (var a in new[] { new[] { "\uE8AC", "Переименовать" }, new[] { "\uE8C8", "Копировать схему в буфер" }, new[] { "\uE792", "Сохранить как файл" }, new[] { "\uE74D", "Удалить" } })
            {
                var b = Btn(null, "Ghost", a[0]); b.ToolTip = a[1]; b.Width = 38; var tip = a[1];
                b.Click += (s, e) => { if (tip == "Переименовать") StartRename(); else if (tip.StartsWith("Копировать")) CopyLayout(); else if (tip.StartsWith("Сохранить")) SaveAs(); else DeleteLayout(); };
                acts.Children.Add(b);
            }
            DockPanel.SetDock(acts, Dock.Right); head.Children.Add(acts);
                        var titles = new StackPanel();
            runBar = BuildRunBar(); titles.Children.Add(runBar);
            viewTitle = new TextBlock { FontSize = 19, FontWeight = FontWeights.SemiBold, TextTrimming = TextTrimming.CharacterEllipsis };
            renameBox = new TextBox { FontSize = 16, Visibility = Visibility.Collapsed, Width = 360, HorizontalAlignment = HorizontalAlignment.Left };
            renameBox.KeyDown += (s, e) => { if (e.Key == Key.Enter) CommitRename(); if (e.Key == Key.Escape) { renameBox.Visibility = Visibility.Collapsed; viewTitle.Visibility = Visibility.Visible; } };
            renameBox.LostKeyboardFocus += (s, e) => { if (renameBox.Visibility == Visibility.Visible) CommitRename(); };
            viewSub = Text("", 12.5, "TextMuted"); titles.Children.Add(viewTitle); titles.Children.Add(renameBox); titles.Children.Add(viewSub);
            stats = new WrapPanel { Margin = new Thickness(0, 12, 0, 6) }; titles.Children.Add(stats);
            evalArea = new StackPanel { Margin = new Thickness(0, 0, 0, 10) }; titles.Children.Add(evalArea);
            head.Children.Add(titles); g.Children.Add(head);
            gridView = new GridView(); gridView.HoverChanged += ShowHover; Grid.SetRow(gridView, 1); g.Children.Add(gridView);
            hoverInfo = Text("", 12.5, "TextMuted"); hoverInfo.Margin = new Thickness(0, 8, 0, 0); Grid.SetRow(hoverInfo, 2); g.Children.Add(hoverInfo);
            counts = new WrapPanel { Margin = new Thickness(0, 10, 0, 4) }; Grid.SetRow(counts, 3); g.Children.Add(counts);
            testArea = new ContentControl { Margin = new Thickness(0, 10, 0, 0) }; Grid.SetRow(testArea, 4); g.Children.Add(testArea);
            return g;
        }
        void ShowLayout()
        {
            var l = Sel(); gridView.L = l; gridView.Hover = -1; gridView.InvalidateVisual(); counts.Children.Clear(); stats.Children.Clear(); ShowHover(-1);
            if (l == null) { viewTitle.Text = "Схем пока нет"; viewSub.Text = "Запустите поиск слева, найденные схемы появятся в списке."; testArea.Content = null; return; }
            renameBox.Visibility = Visibility.Collapsed; viewTitle.Visibility = Visibility.Visible;
            rebuilding = true; favBtn.IsChecked = Favs().Contains(l.Rel); favBtn.Content = Glyph(favBtn.IsChecked == true ? "\uE735" : "\uE734", 14, favBtn.IsChecked == true ? B("Warn") : null); rebuilding = false;
            viewTitle.Text = Path.GetFileNameWithoutExtension(l.Path); viewSub.Text = l.Rel + ",  " + l.W + "×" + l.H + ",  " + l.Time.ToString("dd.MM.yyyy HH:mm");
            var std = LayoutFile.Std(l);
            if (std != null)
            {
                Stat("Мощность", (std[0] / 1000).ToString("0.###") + " T/тик"); Stat("Тепло", (std[1] / 1000).ToString("0.###") + " T/тик");
                Stat("Охлаждение", (std[2] / 1000).ToString("0.###") + " T/тик"); Stat("Цена", std[3].ToString("0.###") + " Qa");
                string[] up = { "Active Venting", "Heat Sinks", "Heat Exch.", "Reinforced", "Chrono" };
                var bought = Enumerable.Range(0, 5).Where(i => std[4 + i] == 1).Select(i => up[i]).ToList();
                Stat("Купить +1", bought.Count == 0 ? "ничего" : string.Join(", ", bought));
            }
            else if (l.Header.Length > 0) stats.Children.Add(new TextBlock { Text = l.Header, FontFamily = Res<FontFamily>("Mono"), FontSize = 12, Foreground = B("TextMuted"), TextWrapping = TextWrapping.Wrap });
            foreach (char c in Parts.Chars)
            {
                int n = l.G.Count(x => x == c); if (n == 0) continue;
                var sp = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 16, 6) };
                sp.Children.Add(Parts.Badge(c, 22, l.Fuel));
                string nm = Parts.Name[c]; if ("124".IndexOf(c) >= 0) nm += " " + (l.Fuel == "early" ? "Nefastium" : FuelCatalog.All.First(f => f.Id == l.Fuel).Name); if (c == 'F') nm = "Quad Dolorium";
                sp.Children.Add(new TextBlock { Text = nm + "  " + n, Margin = new Thickness(6, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center });
                counts.Children.Add(sp);
            }
            ShowRunBar(); RunTestsSoon(); RunEvalSoon();
        }
        void Stat(string k, string v)
        {
            var sp = new StackPanel { Margin = new Thickness(0, 0, 28, 4) };
            sp.Children.Add(Text(k, 12, "TextMuted")); sp.Children.Add(new TextBlock { Text = v, FontFamily = Res<FontFamily>("Mono"), FontSize = 15 });
            stats.Children.Add(sp);
        }
        void ShowHover(int h)
        {
            var l = gridView.L;
            if (h < 0 || l == null) { hoverInfo.Text = l == null ? "" : CompareSummary() ?? "Наведите на клетку, чтобы увидеть компонент и соседей. Стрелки ↑ ↓ — схемы списка, ← → — лучшие из поиска, Ctrl+клик по схеме в списке — сравнить."; return; }
            char c = l.G[h]; string n; Parts.Name.TryGetValue(c, out n);
            int x = h % l.W, y = h / l.W; var nb = new List<string>();
            foreach (var d in new[] { new[] { -1, 0 }, new[] { 1, 0 }, new[] { 0, -1 }, new[] { 0, 1 } })
            { int xx = x + d[0], yy = y + d[1]; if (xx >= 0 && yy >= 0 && xx < l.W && yy < l.H) { char q = l.G[yy * l.W + xx]; nb.Add(Parts.Name.ContainsKey(q) ? Parts.Name[q] : q.ToString()); } }
            string was = compareWith != null && compareWith.W == l.W && compareWith.H == l.H && compareWith.G[h] != c ? ".  В «" + Path.GetFileNameWithoutExtension(compareWith.Path) + "»: " + (Parts.Name.ContainsKey(compareWith.G[h]) ? Parts.Name[compareWith.G[h]] : compareWith.G[h].ToString()) : "";
            hoverInfo.Text = "Столбец " + (x + 1) + ", строка " + (y + 1) + ": " + (n ?? c.ToString()) + ".  Соседи: " + string.Join(", ", nb) + was;
        }

        // ---------- лучшие из последнего поиска (стрелки ← →) ----------
        Border runBar; TextBlock runText;
        Border BuildRunBar()
        {
            var bar = new Border { Background = B("Card"), BorderBrush = B("Line"), BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(8), Padding = new Thickness(6, 4, 6, 4), Margin = new Thickness(0, 0, 0, 12), Visibility = Visibility.Collapsed };
            var d = new DockPanel();
            var prev = Btn(null, "Ghost", "\uE76B"); prev.ToolTip = "Предыдущий вариант (←)"; prev.Click += (s, e) => RunStep(-1);
            var next = Btn(null, "Ghost", "\uE76C"); next.ToolTip = "Следующий вариант (→)"; next.Click += (s, e) => RunStep(1);
            var close = Btn(null, "Ghost", "\uE711"); close.ToolTip = "Скрыть (Esc)"; close.Click += (s, e) => { runBest.Clear(); ShowRunBar(); };
            DockPanel.SetDock(prev, Dock.Left); DockPanel.SetDock(close, Dock.Right); DockPanel.SetDock(next, Dock.Right);
            d.Children.Add(prev); d.Children.Add(close); d.Children.Add(next);
            runText = Text("", 13); runText.VerticalAlignment = VerticalAlignment.Center; runText.Margin = new Thickness(6, 0, 6, 0); d.Children.Add(runText);
            bar.Child = d; return bar;
        }
        void ShowRunBar()
        {
            if (runBar == null) return;
            if (runBest.Count == 0) { runBar.Visibility = Visibility.Collapsed; return; }
            runBar.Visibility = Visibility.Visible; var l = Sel(); int i = l == null ? -1 : runBest.FindIndex(b => string.Equals(b.File, l.Path, StringComparison.OrdinalIgnoreCase));
            if (i < 0) { runText.Text = "Лучшие из последнего поиска: " + runBest.Count + ". Стрелки ← → переключают."; return; }
            var b0 = runBest[i];
            runText.Text = "Лучшие из поиска: " + (i + 1) + " из " + runBest.Count + ".  " + (b0.Tier == 1 ? "Защищённая" : "Без аварийного запаса") + ", чистыми " + (b0.Net / 1000).ToString("0.###") + " T/тик, производство "
                + (b0.Power / 1000).ToString("0.###") + ", цена " + b0.Cost.ToString("0.###") + " Qa, заполнение " + Math.Round(b0.Fill * 100) + "%" + (b0.Fill >= 0.9 ? " (впритык)" : "");
        }
        void RunStep(int d)
        {
            if (runBest.Count == 0) return; var l = Sel(); int i = l == null ? -1 : runBest.FindIndex(b => string.Equals(b.File, l.Path, StringComparison.OrdinalIgnoreCase));
            int j = i < 0 ? 0 : (i + d + runBest.Count) % runBest.Count;
            int k = layouts.FindIndex(x => string.Equals(x.Path, runBest[j].File, StringComparison.OrdinalIgnoreCase));
            if (k < 0) { val["uiSort"] = "new"; selPath = runBest[j].File; forceScan = true; ScanLayouts(null, null); return; }
            library.SelectedIndex = k; library.ScrollIntoView(library.SelectedItem);
        }
        // ---------- сравнение двух схем (Ctrl+клик в списке) ----------
        void SetCompare(Layout other)
        {
            compareWith = other; gridView.Compare = other; gridView.InvalidateVisual(); ShowHover(-1);
        }
        string CompareSummary()
        {
            var l = Sel(); if (compareWith == null || l == null) return null;
            string name = Path.GetFileNameWithoutExtension(compareWith.Path);
            if (compareWith.W != l.W || compareWith.H != l.H) return "Сравнение с «" + name + "»: другой размер поля, сравнить по клеткам нельзя. Esc — выйти из сравнения.";
            int diff = 0; for (int i = 0; i < l.G.Length; i++) if (l.G[i] != compareWith.G[i]) diff++;
            var a = LayoutFile.Std(l); var b = LayoutFile.Std(compareWith);
            string nums = a != null && b != null ? "  Мощность " + ((a[0] - b[0]) / 1000 >= 0 ? "+" : "") + ((a[0] - b[0]) / 1000).ToString("0.###") + " T/тик, цена " + (a[3] - b[3] >= 0 ? "+" : "") + (a[3] - b[3]).ToString("0.###") + " Qa." : "";
            return "Сравнение с «" + name + "»: отличается клеток " + diff + " (обведены)." + nums + "  Ctrl+клик — другая схема, Esc — выйти.";
        }

        // ---------- мгновенный пересчёт выбранной схемы ----------
        void RunEvalSoon() { if (evalDelay == null) return; evalDelay.Stop(); evalDelay.Start(); }
        static Dictionary<string, string> KV(string line)
        { var d = new Dictionary<string, string>(); foreach (var t in line.Split(' ').Skip(1)) { int e = t.IndexOf('='); if (e > 0) d[t.Substring(0, e)] = t.Substring(e + 1); } return d; }
        static double D(Dictionary<string, string> d, string k) { double v; return d.ContainsKey(k) && double.TryParse(d[k], NumberStyles.Float, CultureInfo.InvariantCulture, out v) ? v : 0; }
        void RunEval()
        {
            Kill(evalProc); evalProc = null; evalArea.Children.Clear();
            var l = Sel(); if (l == null || l.Fuel == "early") return;
            var env = EnvFor(l); env["plFile"] = l.Path; var lines = new List<string>(); Process me = null;
            me = evalProc = Engine.Start("--tool ev", Results, env, line => { if (evalProc == me) lines.Add(line); }, code =>
            {
                if (evalProc != me || Sel() != l) return; evalProc = null;
                var a = lines.FirstOrDefault(x => x.StartsWith("A ")); var b = lines.FirstOrDefault(x => x.StartsWith("B "));
                if (a == null) return;
                ShowEval(l.Profile.Count > 0 ? "С параметрами схемы" : "С текущими параметрами", KV(a), null);
                if (l.Profile.Count > 0) evalArea.Children.Add(Text("Планируемая сборка: параметры закреплены в файле; это не подтверждение покупки улучшений в игре.", 12.5, "TextMuted"));
                if (b != null && KV(b)["flags"] != KV(a)["flags"]) ShowEval("Если купить все отмеченные", KV(b), KV(a));
                {
                    var off = new List<string>(); foreach (var r in new[] { "1", "2", "4" }) if (l.G.Contains(r[0]) && val["anA" + r] == "0") off.Add(Parts.Name[r[0]]);
                    if (off.Count > 0) evalArea.Children.Add(Text("В этой схеме есть " + string.Join(", ", off) + ", а в поиске они отключены: «Улучшение» её не возьмёт.", 12.5, "Warn"));
                }
            });
        }
        void ShowEval(string title, Dictionary<string, string> e, Dictionary<string, string> baseE)
        {
            var row = new WrapPanel { Margin = new Thickness(0, 4, 0, 2) };
            var t = Text(title, 12.5, "TextMuted"); t.Width = 230; t.VerticalAlignment = VerticalAlignment.Center; row.Children.Add(t);
            Func<string, double, string> delta = (k, v) => baseE == null ? "" : (Math.Abs(v - D(baseE, k)) < 1e-9 ? "" : (v - D(baseE, k) > 0 ? "  +" : "  ") + ((v - D(baseE, k)) / (k == "cost" ? 1 : 1000)).ToString("0.###"));
            double heat = D(e, "heat"), cool = D(e, "cool"), cost = D(e, "cost"), budget = D(e, "budget"), ptot = D(e, "ptot");
            double ntot = D(e, "ntot"); double generated = e.ContainsKey("generated") ? D(e, "generated") : ptot;
            Mini(row, "Чистыми", (ntot / 1000).ToString("0.###") + " T/тик" + delta("ntot", ntot), ntot > 0 ? "Accent" : "Danger");
            Mini(row, "Производство", (generated / 1000).ToString("0.###") + delta("ptot", ptot), "Text");
            if (Math.Abs(generated - ptot) > 1e-6) Mini(row, "Продажа", (ptot / 1000).ToString("0.###") + " T/тик", "Text");
            Mini(row, "Охлаждение", (cool / 1000).ToString("0.###") + " из " + (heat / 1000).ToString("0.###") + " T/тик" + delta("cool", cool), cool >= heat ? "Text" : "Danger");
            Mini(row, "Цена", cost.ToString("0.###") + " из " + budget.ToString("0.##") + " Qa" + delta("cost", cost), cost <= budget ? "Text" : "Danger");
            if (D(e, "bad") > 0) Mini(row, "Ошибка", "недопустимая схема или перегрев", "Danger");
            evalArea.Children.Add(row);
        }
        void Mini(Panel row, string k, string v, string brush)
        {
            var sp = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 24, 0), VerticalAlignment = VerticalAlignment.Center };
            sp.Children.Add(Text(k + "  ", 12.5, "TextMuted")); var tv = new TextBlock { Text = v, FontFamily = Res<FontFamily>("Mono"), FontSize = 13.5, Foreground = B(brush) }; sp.Children.Add(tv);
            row.Children.Add(sp);
        }
        // ---------- автопроверка выбранной схемы ----------
        void RunTestsSoon() { Kill(testProc); testProc = null; testDelay.Stop(); testDelay.Start(); }
        string CacheKey(Layout l) { var env = EnvFor(l); return l.Path + "|" + l.Time.Ticks + "|" + string.Join(";", pars.Where(x => x.Grp == "engine" || x.Grp == "early" || x.Grp == "save" || x.Grp == "price" || x.Key == "plSteps").OrderBy(x => x.Key).Select(x => x.Key + "=" + env[x.Key])); }
        // Автопроверка = оригинальные проверки архива для выбранной схемы:
        //  текущая модель — validate (reactor-protium) + validate-profit (reactor-profit: 4 порядка обработки × пауза между циклами);
        //  ранняя модель — thermal-check (reactor-search: потеря Outlet × истечение топлива).
        void RunTests()
        {
            var l = Sel(); if (l == null) { testArea.Content = null; return; }
            string cacheKey = CacheKey(l); List<string> cached; if (testCache.TryGetValue(cacheKey, out cached)) { testArea.Content = l.Fuel == "early" ? EarlyTable(cached, false) : TestTable(cached, false); return; }
            var tmp = Path.Combine(Path.GetTempPath(), "ReactorLab-test"); Directory.CreateDirectory(tmp);
            var lines = new List<string>(); Process me = null;
            if (l.Fuel == "early")
            {
                var input = EarlyInput(l); if (input == null) { testArea.Content = Text("Для этой схемы ранней модели нет профиля улучшений (p0, p1, p2), проверка недоступна.", 12.5, "TextMuted"); return; }
                testArea.Content = EarlyTable(lines, true);
                var f = Path.Combine(tmp, "verification-input.txt"); File.WriteAllText(f, input + "\n");
                var env = Env(); env["thFile"] = f;
                me = testProc = Engine.Start("--tool th", tmp, env, line => { if (testProc == me) lines.Add(line); }, code =>
                {
                    if (testProc != me) return; testProc = null;
                    if (lines.Count(x => x.Contains("потеря выхода")) < 4) { testArea.Content = Text("Проверка не выполнена: " + string.Join(" ", lines), 12.5, "Danger"); return; }
                    testCache[cacheKey] = lines; if (Sel() == l) testArea.Content = EarlyTable(lines, false);
                });
                return;
            }
            testArea.Content = TestTable(lines, true);
            var env2 = EnvFor(l); env2["plFile"] = l.Path;
            me = testProc = Engine.Start("--tool pl", tmp, env2, line => { if (testProc == me) lines.Add(line); }, code =>
            {
                if (testProc != me) return; testProc = null;
                if (code != 0 || lines.Count(x => x.EndsWith("PASS") || x.EndsWith("FAIL")) != 30)
                { testArea.Content = Text("Проверка не выполнена: " + string.Join(" ", lines), 12.5, "Danger"); return; }
                testCache[cacheKey] = lines; if (Sel() == l) testArea.Content = TestTable(lines, false);
            });
        }
        // Строка verification-input для одной схемы, как её строит verify-v2: имя, профиль улучшений, Dolorium extra, Nefastium extra, Fission extra, сетка.
        static string EarlyInput(Layout l)
        {
            var stem = Path.GetFileNameWithoutExtension(l.Path); var head = l.Header.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
            int pr = -1, dl = 0, nl = 0, ff = 0;
            if (stem.StartsWith("transition") && head.Length > 12) { int.TryParse(head[5], out pr); int.TryParse(head[6], out dl); int.TryParse(head[7], out nl); int.TryParse(head[12], out ff); }
            else if (stem.Length > 1 && char.IsDigit(stem[stem.Length - 1]) && stem[stem.Length - 2] == 'p') pr = stem[stem.Length - 1] - '0';
            if (pr < 0 || pr > 2 || l.W != 19 || l.H != 16) return null;
            return stem.Replace(' ', '_') + " " + pr + " " + dl + " " + nl + " " + ff + " " + l.G;
        }
        static UIElement Cell(string line, bool loading, Func<string, Brush> B, Func<string, double, string, FrameworkElement> text, Func<string, double, Brush, TextBlock> glyph)
        {
            if (line == null) return new Border { Background = B("Muted"), CornerRadius = new CornerRadius(4), Width = 90, Height = 14, HorizontalAlignment = HorizontalAlignment.Left, Opacity = loading ? 0.6 : 0.3, VerticalAlignment = VerticalAlignment.Center };
            bool ok = line.Contains("PASS"); var sp = new StackPanel { Orientation = Orientation.Horizontal, VerticalAlignment = VerticalAlignment.Center };
            var gl = glyph(ok ? "\uE73E" : "\uE711", 12, B(ok ? "Accent" : "Danger")); gl.Margin = new Thickness(0, 0, 6, 0); sp.Children.Add(gl);
            sp.Children.Add(text(ok ? "прошла" : "сбой", 13, ok ? "Text" : "Danger")); return sp;
        }
        UIElement Table(string title, string[] cols, string[] rows, Func<int, int, string> lineAt, bool loading)
        {
            var grid = new Grid { HorizontalAlignment = HorizontalAlignment.Left, Margin = new Thickness(0, 0, 0, 10) };
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(190) });
            foreach (var c in cols) grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(130) });
            for (int r = 0; r <= rows.Length; r++) grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(28) });
            var tt = Text(title, 12, "TextMuted"); grid.Children.Add(tt);
            for (int c = 0; c < cols.Length; c++) { var t = Text(cols[c], 12, "TextMuted"); Grid.SetColumn(t, c + 1); grid.Children.Add(t); }
            for (int r = 0; r < rows.Length; r++)
            {
                var rl = Text(rows[r], 13); Grid.SetRow(rl, r + 1); rl.VerticalAlignment = VerticalAlignment.Center; grid.Children.Add(rl);
                for (int c = 0; c < cols.Length; c++) { var cell = Cell(lineAt(r, c), loading, B, (a, b, d) => Text(a, b, d), (a, b, d) => Glyph(a, b, d)); Grid.SetRow(cell, r + 1); Grid.SetColumn(cell, c + 1); grid.Children.Add(cell); }
            }
            return grid;
        }
        UIElement Header(List<string> lines, bool loading)
        {
            int fails = lines.Count(x => x.EndsWith("FAIL") || x.Contains(": FAIL") || (x.Contains("потеря выхода") && !x.EndsWith("PASS")));
            var head = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 8) };
            head.Children.Add(new TextBlock { Text = "Проверка", FontSize = 15, FontWeight = FontWeights.SemiBold });
            head.Children.Add(Text(loading ? "   идёт симуляция…" : fails == 0 ? "   все сценарии прошли" : "   сбоев: " + fails, 13, loading ? "TextMuted" : fails == 0 ? "Accent" : "Danger"));
            return head;
        }
        UIElement TestTable(List<string> lines, bool loading)
        {
            var outer = new StackPanel(); outer.Children.Add(Header(lines, loading));
            outer.Children.Add(Table("validate (protium)", new[] { "Штатно", "Outlet off", "Vent off", "Замена топлива" }, new[] { "Порядок обмена 1", "Порядок обмена 2" },
                (r, c) => { string[] k = { "штатно", "Outlet off", "Vent off", "замена топлива" }; return lines.FirstOrDefault(x => x.StartsWith("обмен " + r + " " + k[c] + ":")); }, loading));
            outer.Children.Add(Table("validate-profit (profit)", new[] { "Сетка вперёд", "Сетка назад", "Outlet, затем Inlet", "Inlet, затем Outlet" }, new[] { "Штатно", "Замена топлива", "Выгорание", "Outlet off", "Vent off" },
                (r, c) => { string[] k = { "штатно", "замена топлива", "выгорание", "Outlet off", "Vent off" }; return lines.FirstOrDefault(x => x.StartsWith("обмен " + (c + 2) + " " + k[r] + ":")); }, loading));
            outer.Children.Add(Table("Выгорание", new[] { "Без замены топлива" }, new[] { "Порядок обмена 1", "Порядок обмена 2" },
                (r, c) => lines.FirstOrDefault(x => x.StartsWith("обмен " + r + " выгорание:")), loading));
            return outer;
        }
        UIElement EarlyTable(List<string> lines, bool loading)
        {
            var outer = new StackPanel(); outer.Children.Add(Header(lines, loading));
            outer.Children.Add(Table("thermal-check (ранняя модель)", new[] { "Топливо горит", "Топливо истекает" }, new[] { "Все Outlet", "Потерян один Outlet" },
                (r, c) => lines.FirstOrDefault(x => x.Contains("потеря выхода " + r + " истечение " + c + ":")), loading));
            return outer;
        }
    }
}
