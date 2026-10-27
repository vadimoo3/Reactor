// Самопроверка: ReactorLab.exe --selftest <папка с копией results, icons, ReactorLab.ini>.
// Окно открывается за пределами экрана, сценарии гоняются по-настоящему, отчёт и снимки пишутся в ту же папку.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace ReactorLab
{
    partial class MainWindow
    {
        readonly StringBuilder report = new StringBuilder(); int fails;

        void PrepareSelfTest()
        {
            WindowStartupLocation = WindowStartupLocation.Manual; Left = -20000; Top = 0; ShowInTaskbar = false; ShowActivated = false;
            Loaded += async (s, e) =>
            {
                try { await RunSelfTest(); }
                catch (Exception ex) { Check("исключение в самопроверке: " + ex, false); }
                File.WriteAllText(Path.Combine(Program.SelfTestDir, "selftest-report.txt"), report + "\r\nИТОГ: " + (fails == 0 ? "всё прошло" : "ошибок " + fails) + "\r\n");
                Application.Current.Shutdown(fails);
            };
        }
        void Check(string what, bool ok, string detail = "") { if (!ok) fails++; report.AppendLine((ok ? "PASS  " : "FAIL  ") + what + (detail == "" ? "" : "  [" + detail + "]")); }
        void Note(string s) { report.AppendLine("      " + s); }
        static async Task<bool> WaitFor(Func<bool> cond, int seconds)
        { var end = DateTime.Now.AddSeconds(seconds); while (DateTime.Now < end) { if (cond()) return true; await Task.Delay(200); } return cond(); }
        void Shot(string name)
        {
            UpdateLayout(); var el = (FrameworkElement)Content;
            var rtb = new RenderTargetBitmap((int)el.ActualWidth, (int)el.ActualHeight, 96, 96, PixelFormats.Pbgra32); rtb.Render(el);
            var enc = new PngBitmapEncoder(); enc.Frames.Add(BitmapFrame.Create(rtb));
            using (var f = File.Create(Path.Combine(Program.SelfTestDir, name + ".png"))) enc.Save(f);
        }
        bool SelectRel(string rel)
        {
            int i = layouts.FindIndex(l => l.Rel == rel); if (i < 0) return false;
            library.SelectedIndex = i; return Sel() != null && Sel().Rel == rel;
        }
        IEnumerable<T> All<T>(DependencyObject root) where T : DependencyObject
        {
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); i++)
            { var c = VisualTreeHelper.GetChild(root, i); if (c is T) yield return (T)c; foreach (var x in All<T>(c)) yield return x; }
        }
        static string TextOf(DependencyObject root)
        {
            var sb = new StringBuilder();
            Action<DependencyObject> walk = null; walk = d => { var t = d as TextBlock; if (t != null) sb.Append(t.Text).Append(' '); for (int i = 0; i < VisualTreeHelper.GetChildrenCount(d); i++) walk(VisualTreeHelper.GetChild(d, i)); };
            walk(root); return sb.ToString();
        }
        async Task<bool> SettleView(Layout l)
        {
            return await WaitFor(() => testProc == null && evalProc == null && !testDelay.IsEnabled && !evalDelay.IsEnabled
                && (l.W != 19 || l.H != 16 || (l.Fuel == "early" && EarlyInput(l) == null) || testCache.ContainsKey(CacheKey(l))), 120);
        }
        double NetOf(string file)
        {
            var env = EnvFor("protium"); env["plFile"] = file;
            var si = new System.Diagnostics.ProcessStartInfo(Engine.Exe, "--tool ev") { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, StandardOutputEncoding = Encoding.UTF8, WorkingDirectory = Program.SelfTestDir };
            foreach (var kv in env) si.EnvironmentVariables["RX_" + kv.Key] = kv.Value;
            using (var p = System.Diagnostics.Process.Start(si)) { var o = p.StandardOutput.ReadToEnd(); p.WaitForExit(); var a = o.Split('\n').FirstOrDefault(x => x.StartsWith("A ")); return a == null ? double.NaN : D(KV(a), "ntot"); }
        }
        int PassCount(string file, string fuel)
        {
            var env = EnvFor(fuel); env["plFile"] = file;
            var si = new System.Diagnostics.ProcessStartInfo(Engine.Exe, "--tool pl") { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, StandardOutputEncoding = Encoding.UTF8, WorkingDirectory = Program.SelfTestDir };
            foreach (var kv in env) si.EnvironmentVariables["RX_" + kv.Key] = kv.Value;
            using (var p = System.Diagnostics.Process.Start(si)) { var o = p.StandardOutput.ReadToEnd(); p.WaitForExit(); return o.Split('\n').Count(x => x.TrimEnd().EndsWith("PASS")); }
        }

        async Task RunSelfTest()
        {
            report.AppendLine("Самопроверка Reactor Lab, " + DateTime.Now + ", папка " + Program.SelfTestDir);
            await Task.Delay(500);
            // 1. старт
            Check("список схем загружен", layouts.Count > 0, layouts.Count + " схем");
            Check("рабочие файлы и корзина не видны в списке", layouts.All(l => !l.Rel.Contains("/work/") && !l.Rel.StartsWith(".trash")));
            Shot("01-start");

            // 2. выбранная схема: цифры, пересчёт, автопроверка
            Check("выбор protium/final.txt", SelectRel("protium/final.txt"));
            var fin = Sel(); await SettleView(fin);
            List<string> lines; testCache.TryGetValue(CacheKey(fin), out lines);
            Check("автопроверка final: 30 сценариев прошли", lines != null && lines.Count(x => x.EndsWith("PASS")) == 30, lines == null ? "нет результата" : string.Join(" | ", lines));
            Check("validate-profit не применяется к схеме с Exchanger (его модель их не считает)", lines != null && !lines.Any(x => x.StartsWith("порядок")));
            var prof = layouts.FirstOrDefault(x => x.Rel == "profit/6S0D0Q_f0_feed5_3433555.txt");
            if (prof != null)
            {
                library.SelectedIndex = layouts.IndexOf(prof); await SettleView(prof); List<string> pl2; testCache.TryGetValue(CacheKey(prof), out pl2);
                Check("validate-profit прибыльной схемы: 6 порядков × штатно/замена/выгорание, все 18 проходят", pl2 != null && pl2.Count(x => x.Contains("PASS")) == 18, pl2 == null ? "" : string.Join(" | ", pl2.Where(x => x.StartsWith("порядок")).Select(x => x.Substring(0, Math.Min(26, x.Length)))));
                Shot("02b-profit"); SelectRel("protium/final.txt"); await SettleView(Sel());
            }
            var head = TextOf(evalArea);
            Check("пересчёт с сейвом показан (чистыми и производство)", head.Contains("Чистыми") && (head.Contains("16,145") || head.Contains("16.145")), head.Trim());
            Check("цифры файла совпадают с README (16,145 T/тик, 15,739 Qa)", TextOf(stats).Contains("16,145") || TextOf(stats).Contains("16.145"), TextOf(stats).Trim());
            Check("иконка Protium = Fuel7", Parts.FuelSprite("protium") == 7 && Parts.Icon('1', "protium") != null);
            Shot("02-final");
            foreach (var planName in new[] { "protium-heat", "monastium-heat" })
            {
                var plan = layouts.FirstOrDefault(x => x.Rel == "profit/" + planName + ".txt");
                if (plan == null) continue;
                library.SelectedIndex = layouts.IndexOf(plan); await SettleView(plan);
                var planEnv = EnvFor(plan); var planText = TextOf(evalArea); List<string> planChecks; testCache.TryGetValue(CacheKey(plan), out planChecks);
                Check("план " + planName + ": закреплены будущие параметры", plan.Profile.Count > 0 && planEnv["fuelPow"] == (plan.Fuel == "monastium" ? "16000" : "10000") && plan.W == 20 && plan.H == 17);
                Check("план " + planName + ": выводится верная чистая выработка", plan.Fuel == "monastium" ? (planText.Contains("194,824") || planText.Contains("194.824")) : (planText.Contains("213,037") || planText.Contains("213.037")), planText);
                Check("план " + planName + ": 18 штатных сценариев, 12 отказов при отключении охлаждения", planChecks != null && planChecks.Count(x => x.EndsWith("PASS")) == 18 && planChecks.Count(x => x.EndsWith("FAIL")) == 12);
                if (plan.Fuel == "monastium") Check("Monastium использует оригинальный Fuel8", Parts.FuelSprite(plan.Fuel) == 8 && Parts.Icon('4', plan.Fuel) != null);
                Shot("plan-" + planName);
            }
            SelectRel("protium/final.txt"); await SettleView(Sel());

            var png = Path.Combine(Program.SelfTestDir, "final-picture.png"); SavePicture(fin, png);
            Check("картинка схемы сохраняется (как draw.py)", File.Exists(png) && new FileInfo(png).Length > 20000, new FileInfo(png).Length + " байт");
            var early = layouts.FirstOrDefault(l => l.Fuel == "early");
            if (early != null)
            {
                library.SelectedIndex = layouts.IndexOf(early); await SettleView(early);
                List<string> el; testCache.TryGetValue(CacheKey(early), out el);
                Check("ранняя модель: thermal-check выбранной схемы, 4 сценария", el != null && el.Count(x => x.Contains("потеря выхода")) == 4, early.Rel + (el == null ? "" : ": " + string.Join(" | ", el.Where(x => x.Contains("потеря выхода")))));
                Check("иконки ранней модели: 1/2/4 = Nefastium (Fuel6)", Parts.FuelSprite(early.Fuel) == 6);
                Shot("03-early");
            }

            // 3. топливо и стержни
            var keep = new Dictionary<string, string>(val);
            foreach (var f in FuelCatalog.All)
            {
                val["uiFuel"] = f.Id; BuildForm(); UpdateLayout();
                var chips = All<ToggleButton>(formHost).Where(t => t.Style == (Style)Application.Current.Resources["Chip"]).ToList();
                int disabled = chips.Count(t => !t.IsEnabled);
                Check("топливо " + f.Name + ": недоступные размеры стержней заблокированы", f.F1 == null ? disabled == 2 : disabled == 0, "заблокировано " + disabled);
            }
            val["uiFuel"] = "protium"; BuildForm();

            // 4. работа с результатами внутри программы
            SelectRel("protium/final.txt");
            PasteText(File.ReadAllText(Sel().Path)); var pasted = Sel();   // системный буфер пользователя не трогаем
            Check("вставка схемы из буфера", pasted != null && pasted.Rel.StartsWith("imported/") && pasted.G == fin.G, pasted == null ? "" : pasted.Rel);
            Check("копирование схемы: текст для буфера = файл схемы", CopyText() == File.ReadAllText(pasted.Path));
            favBtn.IsChecked = true; Check("избранное: отмечено", Favs().Contains(pasted.Rel));
            val["uiSort"] = "fav"; forceScan = true; ScanLayouts(null, null);
            Check("сортировка «Избранные» показывает только избранное", layouts.Count >= 1 && layouts.All(l => Favs().Contains(l.Rel)), layouts.Count + "");
            val["uiSort"] = "power"; forceScan = true; ScanLayouts(null, null);
            var pw = layouts.Select(l => { var sd = LayoutFile.Std(l); return sd == null ? -1 : sd[0]; }).ToList();
            Check("сортировка «Мощные» по убыванию", pw.Zip(pw.Skip(1), (a, b) => a >= b).All(x => x));
            val["uiSort"] = "new"; forceScan = true; ScanLayouts(null, null);
            selPath = pasted.Path; forceScan = true; ScanLayouts(null, null);
            StartRename(); renameBox.Text = "моя схема тест"; CommitRename();
            var renamed = Sel();
            Check("переименование", renamed != null && Path.GetFileName(renamed.Path) == "моя схема тест.txt" && File.Exists(renamed.Path) && !File.Exists(pasted.Path), renamed == null ? "" : renamed.Rel);
            Check("избранное переехало вместе с переименованием", Favs().Contains(renamed.Rel));
            await SettleView(renamed); List<string> rl; testCache.TryGetValue(CacheKey(renamed), out rl);
            Check("движок читает схему с русским именем (автопроверка 30/30)", rl != null && rl.Count(x => x.EndsWith("PASS")) == 30);
            var delPath = renamed.Path; DeleteLayout();
            Check("удаление убирает схему из списка", !File.Exists(delPath) && layouts.All(l => l.Path != delPath) && undoBar.Visibility == Visibility.Visible);
            UndoDelete();
            Check("«Вернуть» восстанавливает схему", File.Exists(delPath) && layouts.Any(l => l.Path == delPath));
            Shot("04-library");
            selPath = delPath; forceScan = true; ScanLayouts(null, null); DeleteLayout();

            // 4б. сравнение двух схем
            SelectRel("protium/final.txt"); var other = layouts.FirstOrDefault(x => x.Rel == "protium/cheaper-alternative.txt") ?? layouts.First(x => x.W == 19 && x.H == 16 && x.Rel != "protium/final.txt" && x.Fuel != "early");
            SetCompare(other); var cs = CompareSummary();
            Check("сравнение двух схем: обводятся отличающиеся клетки и считается разница", cs != null && cs.Contains("отличается клеток") && gridView.Compare == other, cs);
            Shot("04b-compare"); SetCompare(null); Check("Esc выходит из сравнения", compareWith == null && gridView.Compare == null);

            // 5. панель сейва
            ToggleDrawer(); UpdateLayout();
            Check("панель сейва открывается", drawer.Visibility == Visibility.Visible && drawerBody.Children.Count > 3);
            var dtext = TextOf(drawerBody);
            Check("константы игры свёрнуты, выбора иконок нет", !dtext.Contains("Иконки") && drawerBody.Children.OfType<StackPanel>().Any(sp => sp.Visibility == Visibility.Collapsed));
            Shot("05-drawer"); ToggleDrawer();
            Check("панель сейва закрывается", drawer.Visibility == Visibility.Collapsed);

            // 6. крайние случаи
            foreach (var k in new[] { "anA1", "anA2", "anA4" }) val[k] = "0";
            StartRun(); Check("без стержней поиск не стартует и объясняет", stages == null && status.Text.Contains("стержн"), status.Text);
            val["anA1"] = "1"; val["anA4"] = "1";
            foreach (var k in new[] { "anFav", "anFhs", "anFex", "anFre", "anFch" }) val[k] = "1";
            val["budget"] = "100"; val["uiTime"] = "60"; val["uiCountOn"] = "0";
            if (early != null) { library.SelectedIndex = layouts.IndexOf(early); val["uiMethod"] = "improve"; StartRun(); Check("«Улучшение» отказывается от схемы ранней модели", stages == null && status.Text.Contains("текущей модели"), status.Text); }
            SelectRel("protium/final.txt"); val["uiFuel"] = "nefastium"; val["uiMethod"] = "improve"; StartRun();
            Check("«Улучшение» не смешивает топливо схемы и выбранное", stages == null && status.Text.Contains("топливе"), status.Text);
            val["uiFuel"] = "protium";

            // 7. три способа целиком
            Check("пресеты времени: 5 сек, 1 мин, 5 мин", TimePresets.SequenceEqual(new[] { 5, 60, 300 }) && TextOf(formHost).Contains("5 сек") && TextOf(formHost).Contains("1 мин") && TextOf(formHost).Contains("5 мин"));
            BuildForm(); UpdateLayout();
            Check("галочка «Точное количество стержней» есть и показывает сохранённое состояние", All<CheckBox>(formHost).Count() == 1 && All<CheckBox>(formHost).First().IsChecked == (val["uiCountOn"] == "1"));
            foreach (var m in new[] { "improve", "blocks", "random", "tile" })
            {
                SelectRel("protium/final.txt"); val["uiMethod"] = m; val["uiTime"] = "5"; BuildForm();
                var t0 = DateTime.Now; StartRun();
                Check("способ «" + m + "» запустился", stages != null, status.Text);
                bool stopped = await WaitFor(() => stages == null, 240);
                Check("способ «" + m + "» завершился", stopped, Math.Round((DateTime.Now - t0).TotalSeconds) + " с: " + status.Text);
                Note("журнал этапов: " + string.Join(" / ", log.Text.Split('\n').Where(x => x.StartsWith("--- этап") || x.StartsWith("ЛУЧШИЙ") || x.StartsWith("ПОДСКАЗКА") || x.StartsWith("отбор:")).Skip(0).Select(x => x.Trim()).Reverse().Take(16).Reverse()));
                bool got = finalFile != null && File.Exists(finalFile);
                string altF = finalFile == null ? null : finalFile.Substring(0, finalFile.Length - 4) + "-без-запаса.txt";
                if (m == "random") Check("«Полный рандом» даёт итог (защищённый или без аварийного запаса)", got || File.Exists(altF), status.Text);
                if (m == "tile") Check("«Блоками» даёт итог (защищённый или без аварийного запаса)", got || File.Exists(altF), status.Text);
                if (!got && altF != null && File.Exists(altF)) Check("итог без запаса проходит штатную работу и остывание (validate: штатно и замена топлива)", PassCount(altF, "protium") >= 4);
                bool notBetter = m == "improve" && !got && status.Text.Contains("исходная не изменена");
                if (m == "improve") Check("«Улучшение» даёт итог лучше исходной или сообщает, что исходная не изменена", got || File.Exists(altF) || notBetter, status.Text);
                else Note("итог «" + m + "»: " + (got ? Path.GetFileName(finalFile) : "нет") + ", статус: " + status.Text);
                if (got)
                {
                    var r = LayoutFile.Parse(finalFile);
                    Check("итог «" + m + "» выбран в списке", Sel() != null && runBest.Count > 0 && Sel().Path == runBest.OrderByDescending(x => x.Net).ThenBy(x => x.Cost).First().File);
                    Check("итог «" + m + "» проходит оригинальную проверку validate (30/30)", PassCount(finalFile, "protium") == 30);
                    var sd = LayoutFile.Std(r); Check("итог «" + m + "» в бюджете", sd != null && sd[3] <= 100, sd == null ? "" : sd[3] + " Qa");
                    if (m == "improve") { double nb = NetOf(fin.Path), nf = NetOf(finalFile); Check("«Улучшение» не хуже исходной по чистой мощности", nf >= nb - 1e-6, (nb / 1000).ToString("0.###") + " -> " + (nf / 1000).ToString("0.###") + " T/тик"); }
                    await SettleView(Sel());
                }
                Check("параметры и кандидаты сохранены после завершения", workDir != null && File.Exists(Path.Combine(workDir, "stage-0.params.txt")) && Directory.Exists(Path.Combine(workDir, "an")));
                var lg = log.Text;
                if (m == "blocks" || m == "random")
                {
                    Check("«" + m + "»: используется единый поиск", lg.Contains("единый поиск:"));
                    Check("«" + m + "»: сохранена статистика операторов", File.Exists(Path.Combine(workDir, "an", "operators.tsv")));
                    Check("«" + m + "»: лучшие схемы библиотеки взяты в пул", lg.Contains("из библиотеки взято лучших схем"));
                }
                if (!notBetter) Check("«" + m + "»: полоса «Лучшие из поиска» показана", runBest.Count >= 1 && runBar.Visibility == Visibility.Visible, runBest.Count + " вариантов: " + runText.Text);
                if (runBest.Count >= 2)
                {
                    var before = Sel() == null ? "" : Sel().Path; RunStep(1); var after = Sel() == null ? "" : Sel().Path;
                    Check("«" + m + "»: стрелка → переключает на следующий лучший вариант", after != before && runBest.Any(b => string.Equals(b.File, after, StringComparison.OrdinalIgnoreCase)), Path.GetFileName(after) + ": " + runText.Text);
                    RunStep(-1); Check("«" + m + "»: стрелка ← возвращает", Sel() != null && Sel().Path == before);
                    Check("«" + m + "»: все лучшие варианты разные", runBest.Select(b => LayoutFile.Parse(b.File).G + string.Join(" ", LayoutFile.Std(LayoutFile.Parse(b.File)).Skip(4).Take(5))).Distinct().Count() == runBest.Count);
                }
                Shot("06-" + m);
            }
            // 7а. точное количество, которое не влезает в бюджет: сообщение OVER_BUDGET, топливо не заменяется
            val["uiCountOn"] = "1"; val["uiCount"] = "200"; val["uiMethod"] = "blocks"; StartRun();
            Check("точное количество сверх бюджета: OVER_BUDGET, поиск не стартует", stages == null && status.Text.StartsWith("OVER_BUDGET"), status.Text);
            val["uiCountOn"] = "0";
            // 7б. пресет 5 сек и точное количество стержней
            SelectRel("protium/final.txt"); val["uiMethod"] = "random"; val["uiTime"] = "5"; val["uiCountOn"] = "1"; val["uiCount"] = "6"; BuildForm();
            Check("с галочкой поле количества активно", All<TextBox>(formHost).Any(t => t.IsEnabled && t.Text == "6"));
            var tq = DateTime.Now; StartRun(); await WaitFor(() => stages == null, 120);
            double dq = (DateTime.Now - tq).TotalSeconds;
            Check("пресет «5 сек» укладывается в разумное время", dq < 40, Math.Round(dq) + " с");
            Note("этапы с точным количеством: " + string.Join(" / ", log.Text.Split('\n').Where(x => x.StartsWith("--- этап")).Select(x => x.Trim()).Reverse().Take(10).Reverse()));
            var outs = new[] { finalFile, finalFile == null ? null : finalFile.Substring(0, finalFile.Length - 4) + "-без-запаса.txt" }.Where(f => f != null && File.Exists(f)).ToList();
            Check("при точном количестве итог — ровно 6 топливных клеток", outs.Count > 0 && outs.All(f => LayoutFile.Parse(f).G.Count(ch => ch == '1' || ch == '2' || ch == '4') == 6), outs.Count == 0 ? status.Text : string.Join(", ", outs.Select(Path.GetFileName)));
            val["uiCountOn"] = "0"; val["uiTime"] = "60";

            // 8. остановка
            SelectRel("protium/final.txt"); val["uiMethod"] = "blocks"; StartRun();
            await Task.Delay(2500); goBtn.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
            Check("«Остановить» прерывает поиск", await WaitFor(() => stages == null, 20) && status.Text.StartsWith("Остановлено"), status.Text);
            StartRun(); goBtn.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
            Check("повторный клик сразу после старта не останавливает поиск", stages != null);
            await Task.Delay(1700); goBtn.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await WaitFor(() => stages == null, 20);
            foreach (var kv in keep) val[kv.Key] = kv.Value;
        }
    }
}
