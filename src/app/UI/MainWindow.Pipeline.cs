// Запуск способов: генераторы с адаптивным распределением времени, доводка нескольких разных кандидатов,
// старт с лучших схем библиотеки, итоговый отбор топ-N для переключения стрелками.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace ReactorLab
{
    partial class MainWindow
    {
        void Log(string s) { log.AppendText(s + "\r\n"); if (log.LineCount > 6000) log.Text = log.Text.Substring(log.Text.Length / 2); log.ScrollToEnd(); }
        static void Kill(Process p) { try { if (p != null && !p.HasExited) p.Kill(); } catch { } }
        void SetGo(bool running)
        {
            var sp = (StackPanel)goBtn.Content; ((TextBlock)sp.Children[0]).Text = running ? "" : "";
            ((TextBlock)sp.Children[1]).Text = running ? "Остановить" : "Найти";
            goBtn.Style = Res<Style>(running ? "Ghost" : "Primary"); goBtn.BorderBrush = running ? B("Border") : null;
            prog.Visibility = running ? Visibility.Visible : Visibility.Collapsed; formHost.IsEnabled = !running;
        }
        static string N(double v, double min = 1000) { return ((long)Math.Max(min, v)).ToString(); }
        static double Num(string s) { double v; double.TryParse((s ?? "").Replace(',', '.'), NumberStyles.Float, CultureInfo.InvariantCulture, out v); return v; }
        // Скорости алгоритмов (замер, попыток в секунду на одном ядре): доля времени превращается в число попыток.
        const double RatePr = 13000, RateMo = 60000, RateDr = 40000, RatePf = 15000, RateFr = 400000, RateGd = 125, RateFx = 83, RateRr = 77000, RateSv = 40000, RateTr = 10500, RateThTicks = 20000;
        // Свободная выделяемая память (MB). Полный случайный перебор хранит все просмотренные сетки (~450 байт на попытку):
        // число его попыток ограничено так, чтобы он брал не больше 20% свободного, и не отнимал память у других программ.
        [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
        class MemStatus { public uint Length = 64, Load; public ulong TotalPhys, AvailPhys, TotalPage, AvailPage, TotalVirt, AvailVirt, AvailExt; }
        [System.Runtime.InteropServices.DllImport("kernel32.dll")] static extern bool GlobalMemoryStatusEx([System.Runtime.InteropServices.In, System.Runtime.InteropServices.Out] MemStatus m);
        static double FreeCommitMB() { var m = new MemStatus(); return GlobalMemoryStatusEx(m) ? m.AvailPage / 1048576.0 : 4096; }
        static double FrMemoryCap() { return Math.Max(50000, FreeCommitMB() * 0.2 * 1048576 / 450); }
        int ExactCount() { int c; return val["uiCountOn"] == "1" && int.TryParse(val["uiCount"], out c) && c > 0 ? c : 0; }
        static int PolishCount(int T) { return T < 30 ? 1 : T < 120 ? 3 : 5; }

        // ---------- генераторы ----------
        List<Gen> Generators(string m, int T)
        {
            var rnd = new Random(); Func<string> seed = () => rnd.Next(1, int.MaxValue).ToString();
            int cnt = ExactCount(); var gens = new List<Gen>();
            Action<string, string, string, Action<Dictionary<string, string>, double>> g = (id, label, tool, fill) => gens.Add(new Gen { Id = id, Label = label, Tool = tool, Fill = fill });
            string refFile = Path.Combine(Results, "full-random", "reference-layout.txt"), stable = Path.Combine(Results, "full-random", "stable-reference.txt");
            if (m == "blocks")
            {
                g("pr", "блоки Protium", "pr", (e, s) => { e["prLoops"] = N(RatePr * s); e["prSeed"] = seed(); });
                g("mo", "модули 3×3", "mo", (e, s) => { e["moDraws"] = N(RateMo * s); e["moSeed"] = seed(); });
                g("dr", "прямые схемы", "dr", (e, s) => { e["drTrials"] = N(RateDr * s); e["drSeed"] = seed(); });
                g("pf", "прибыльные группы", "pf", (e, s) => { e["pfLoops"] = N(RatePf * s); e["pfSeed"] = seed(); });
            }
            else if (m == "random")
            {
                g("rr", "чистый случайный", "rr", (e, s) => { e["rrLoops"] = N(RateRr * s); e["rrSeed"] = seed(); });
                g("fr", "полный случайный перебор", "fr", (e, s) => { e["frAttempts"] = N(Math.Min(Math.Min(2e6, FrMemoryCap()), RateFr * s)); e["frSeed"] = seed(); e["frVerify"] = N(Math.Min(400, 5 * T), 30); e["frRef"] = refFile; e["frStable"] = stable; });
                g("gd", "направленный случайный", "gd", (e, s) => { e["gdTrials"] = N(RateGd * s, 20); e["gdMaxF"] = (cnt > 0 ? Math.Min(30, cnt) : 6).ToString(); e["gdSeed"] = seed(); });
            }
            // reactor-fixed-FUEL: точное число клеток каждого разрешённого размера; без галочки — самые крупные количества, которые влезают в бюджет
            var env = EnvFor(val["uiFuel"]); double budget = Num(env["budget"]), venting = Num(env["pav"]); var combos = new List<int[]>();
            foreach (var t in new[] { 1, 2, 4 })
            {
                if (env["anA" + t] != "1") continue; double price = Num(env["f" + t]);
                if (cnt > 0) { if (cnt * price + venting <= budget) combos.Add(new[] { t, cnt }); continue; }
                int added = 0; for (int c = 8; c >= 1 && added < 2; c--) if (c * price + venting <= budget) { combos.Add(new[] { t, c }); added++; }
            }
            foreach (var cb in combos.Take(T < 30 ? 2 : 4))
            {
                var t = cb[0]; var c = cb[1];
                g("fx" + t + "x" + c, "точное топливо: " + c + " × " + (t == 1 ? "Single" : t == 2 ? "Double" : "Quad"), "fx", (e, s) => { e["fxType"] = t.ToString(); e["fxCount"] = c.ToString(); e["fxTrials"] = N(RateFx * s, 10); e["fxSeed"] = seed(); });
            }
            return gens;
        }
        Stage Slice(Gen gen, double secs, int round)
        {
            var x = new Stage { Label = gen.Label + (round > 1 ? ", раунд " + round : ""), Tool = gen.Tool, Dir = Path.Combine(gen.Id, "r" + round), Gen = gen };
            gen.Fill(x.Env, secs); return x;
        }
        // Раунд 1 — всем поровну; оценка каждого генератора; раунд 2 — время пропорционально результату (у слабых остаётся минимум на разведку).
        List<Stage> Adaptive(List<Gen> gens, double seconds, int T)
        {
            var L = new List<Stage>(); if (gens.Count == 0) return L;
            if (T < 30 || gens.Count == 1) { foreach (var gn in gens) L.Add(Slice(gn, seconds / gens.Count, 1)); return L; }
            foreach (var gn in gens) L.Add(Slice(gn, 0.4 * seconds / gens.Count, 1));
            foreach (var gn in gens)
            {
                var sc = new Stage { Label = "оценка: " + gn.Label, Tool = "pk", Dir = "", Pick = true, Gen = gn };
                sc.Env["pkDir"] = Path.Combine(workDir, gn.Id); sc.Env["pkScore"] = "1"; sc.Env["pkMaxVerify"] = "12"; sc.Env["pkCount"] = ExactCount().ToString(); L.Add(sc);
            }
            L.Add(new Stage { Label = "распределение времени", Expand = () =>
            {
                var pos = gens.Where(x => x.Score > 0).ToList(); double lo = pos.Count > 0 ? pos.Min(x => x.Score) : 0, hi = pos.Count > 0 ? pos.Max(x => x.Score) : 1;
                foreach (var gn in gens) gn.Share = gn.Score > 0 ? 0.15 + 0.85 * (hi > lo ? (gn.Score - lo) / (hi - lo) : 1) : 0.06;
                double sum = gens.Sum(x => x.Share); foreach (var gn in gens) gn.Share /= sum;
                Log("распределение времени по результатам раунда 1: " + string.Join(", ", gens.OrderByDescending(x => x.Share).Select(x => x.Label + " " + Math.Round(100 * x.Share) + "%" + (x.Score > 0 ? " (" + (x.Score / 1000).ToString("0.###") + " T/тик)" : " (нет рабочих)"))));
                return gens.Select(gn => Slice(gn, 0.6 * seconds * gn.Share, 2)).ToList();
            } });
            return L;
        }
        Stage PickStage(string label, string outFile, string baseF, int top, int T)
        {
            var x = new Stage { Label = label, Tool = "pk", Dir = "", Pick = true };
            x.Env["pkDir"] = workDir; x.Env["pkOut"] = outFile; x.Env["pkBase"] = baseF ?? ""; x.Env["pkTop"] = top.ToString();
            x.Env["pkCount"] = ExactCount().ToString(); x.Env["pkMaxVerify"] = N(Math.Min(400, Math.Max(30, 6 * T)), 30); return x;
        }
        // Доводка отжигом каждого из лучших разных кандидатов промежуточного отбора (список берётся из pick.top.tsv после отбора).
        Stage Polish(double seconds, string fallbackGlob, bool allowRestarts)
        {
            return new Stage { Label = "доводка лучших", Expand = () =>
            {
                string th = Math.Max(1, Environment.ProcessorCount / 2).ToString(); var rnd = new Random();
                var man = Path.Combine(workDir, "pick.top.tsv"); var starts = File.Exists(man) ? File.ReadAllLines(man).Select(l => l.Split('\t')[0]).Where(File.Exists).ToList() : new List<string>();
                if (starts.Count == 0 && fallbackGlob != null) foreach (var g in fallbackGlob.Split('|')) { var c = Directory.EnumerateFiles(workDir, g, SearchOption.AllDirectories).FirstOrDefault(); if (c != null) { starts.Add(c); Log("рабочих кандидатов пока нет, отжиг стартует от " + Path.GetFileName(c)); break; } }
                var L = new List<Stage>();
                if (starts.Count == 0)
                {
                    if (!allowRestarts) { Log("доводить нечего"); return L; }
                    var r = new Stage { Label = "отжиг с рестартами", Tool = "an", Dir = "an" }; r.Env["anMode"] = "2"; r.Env["anSecs"] = Math.Max(1, Math.Round(seconds)).ToString(); r.Env["anThreads"] = th; r.Env["anSeed"] = rnd.Next(1, int.MaxValue).ToString(); L.Add(r); return L;
                }
                Log("доводка " + starts.Count + " разных кандидатов: " + string.Join(", ", starts.Select(Path.GetFileName)));
                int i = 0; foreach (var s in starts)
                {
                    i++; var x = new Stage { Label = "доводка отжигом " + i + " из " + starts.Count, Tool = "an", Dir = "an" + i };
                    x.Env["anMode"] = "1"; x.Env["anStart"] = s; x.Env["anSecs"] = Math.Max(1, Math.Round(seconds / starts.Count)).ToString(); x.Env["anThreads"] = th; x.Env["anSeed"] = rnd.Next(1, int.MaxValue).ToString(); L.Add(x);
                }
                return L;
            } };
        }
        // Лучшие схемы библиотеки той же модели и топлива — в пул кандидатов (повторный поиск продолжает, а не начинает заново).
        int SeedFromLibrary()
        {
            var seeds = layouts.Where(l => l.Fuel == val["uiFuel"] && l.W == 19 && l.H == 16 && LayoutFile.Std(l) != null)
                .OrderByDescending(l => LayoutFile.Std(l)[0]).Take(5).ToList();
            var dir = Path.Combine(workDir, "seed"); Directory.CreateDirectory(dir); int i = 0;
            foreach (var l in seeds) File.Copy(l.Path, Path.Combine(dir, "lib-" + (++i) + ".txt"), true);
            return i;
        }

        // ---------- план ----------
        List<Stage> Plan(string m, int T, Layout sel)
        {
            var rnd = new Random(); Func<string> seed = () => rnd.Next(1, int.MaxValue).ToString();
            string th = Math.Max(1, Environment.ProcessorCount / 2).ToString();   // не больше половины ядер: процессор никогда не на 100%
            string refFile = Path.Combine(Results, "full-random", "reference-layout.txt");
            int cnt = ExactCount(), K = PolishCount(T);
            var L = new List<Stage>();
            Func<string, string, Stage> st = (label, tool) => { var x = new Stage { Label = label, Tool = tool, Dir = tool }; L.Add(x); return x; };
            if (m == "blocks")
            {
                L.AddRange(Adaptive(Generators(m, T), 0.5 * T, T));
                L.Add(new Stage { Label = "доводка Protium и валидатор прибыли", Expand = () =>
                {
                    var X = new List<Stage>();
                    if (Directory.Exists(Path.Combine(workDir, "pr"))) foreach (var b in Directory.EnumerateFiles(Path.Combine(workDir, "pr"), "best.txt", SearchOption.AllDirectories).Take(2))
                    { var r = new Stage { Label = "доводка Protium", Tool = "rf", Dir = Path.GetDirectoryName(b) }; r.Env["rfFile"] = b; X.Add(r); }
                    var w = Directory.Exists(Path.Combine(workDir, "pf")) ? Directory.EnumerateFiles(Path.Combine(workDir, "pf"), "winner-path.txt", SearchOption.AllDirectories).LastOrDefault() : null;
                    if (w != null) { var v = new Stage { Label = "валидатор прибыли", Tool = "pv", Dir = Path.GetDirectoryName(w) }; v.Env["pvFile"] = ""; v.Env["pvTicks"] = N(Math.Min(100000, 100000 * 0.03 * T / 1.3), 7300); X.Add(v); }
                    return X;
                } });
                L.Add(PickStage("отбор лучших", Path.Combine(workDir, "pick.txt"), null, K, T));
                L.Add(Polish(0.2 * T, "pick-без-запаса.txt", false));
                // ранняя модель (Nefelium): результаты, как в оригинале, в results\search; с галочкой — ровно столько клеток F (dolorium-60-search)
                string early = Path.Combine(Results, "search"); Directory.CreateDirectory(early);
                var s1 = st("ранняя модель: поиск v2" + (cnt > 0 ? ", ровно " + cnt + " F" : ""), "sv"); s1.Dir = early; s1.Env["svRuns"] = "1"; s1.Env["svAtt"] = N(RateSv * 0.08 * T); s1.Env["svSeed"] = seed(); s1.Env["svExactF"] = cnt.ToString();
                var s2 = st("ранняя модель: гибрид C+I", "hy"); s2.Dir = early; s2.Env["hyIter"] = N(7000 * Math.Min(1, T / 60.0), 200); s2.Env["hySeed"] = seed();
                var s3 = st("ранняя модель: переходы по бюджету", "tr"); s3.Dir = early; s3.Env["trAtt"] = N(RateTr * 0.03 * T, 200); s3.Env["trSeed"] = seed();
                var s4 = st("ранняя модель: сводка", "ve"); s4.Dir = early;
                var s5 = st("ранняя модель: тепловая проверка", "th"); s5.Dir = early; s5.Env["thFile"] = Path.Combine(early, "verification-input.txt"); s5.Env["thTicks"] = N(Math.Min(100000, RateThTicks * 0.05 * T), 2000);
            }
            else if (m == "random")
            {
                L.AddRange(Adaptive(Generators(m, T), 0.55 * T, T));
                L.Add(PickStage("отбор лучших", Path.Combine(workDir, "pick.txt"), refFile, K, T));
                L.Add(Polish(0.3 * T, "pick-без-запаса.txt|cand-1.txt", true));
            }
            else if (m == "tile")
            {
                var src = Path.Combine(workDir, "tile-src"); Directory.CreateDirectory(src); File.Copy(sel.Path, Path.Combine(src, "selected.txt"), true);
                var b = st("блоки из выбранной схемы", "tl"); b.Env["tlDir"] = src; b.Env["anW"] = sel.W.ToString(); b.Env["anH"] = sel.H.ToString(); b.Env["anSecs"] = Math.Max(1, Math.Round(0.9 * T)).ToString(); b.Env["anThreads"] = th; b.Env["anSeed"] = seed();
            }
            else
            {
                var a = st("доводка отжигом", "an"); a.Env["anMode"] = "1"; a.Env["anStart"] = sel.Path; a.Env["anSecs"] = Math.Max(1, Math.Round(0.6 * T)).ToString(); a.Env["anThreads"] = th; a.Env["anSeed"] = seed();
                if (sel.W == 19 && sel.H == 16)
                { var b = st("мутации выбранной схемы", "fr"); b.Env["frAttempts"] = N(Math.Min(Math.Min(2e6, FrMemoryCap()), RateFr * 0.2 * T)); b.Env["frVerify"] = N(Math.Min(400, 5 * T), 30); b.Env["frSeed"] = seed(); b.Env["frRef"] = sel.Path; b.Env["frStable"] = sel.Path; }
            }
            var fin = PickStage("итоговый отбор", finalFile, m == "random" ? refFile : baseFile, 5, T); if (m == "improve") fin.Env["pkFloor"] = "1"; L.Add(fin);
            return L;
        }

        // ---------- запуск ----------
        void StartRun()
        {
            string m = val["uiMethod"]; int T; if (!int.TryParse(val["uiTime"], out T) || !TimePresets.Contains(T)) T = 60;
            var fe = FuelCatalog.Env(val["uiFuel"]);
            if (!new[] { "1", "2", "4" }.Any(r => val["anA" + r] == "1" && !fe.ContainsKey("anA" + r))) { status.Text = "Включите хотя бы один доступный размер стержней."; return; }
            int exact = ExactCount();
            if (exact > 0)
            {
                var env = EnvFor(val["uiFuel"]); var over = new List<string>(); bool any = false;
                foreach (var t in new[] { 1, 2, 4 }) if (env["anA" + t] == "1") { double cost = exact * Num(env["f" + t]) + Num(env["pav"]); if (cost <= Num(env["budget"])) any = true; else over.Add(exact + " × " + (t == 1 ? "Single" : t == 2 ? "Double" : "Quad") + ": топливо " + (exact * Num(env["f" + t])).ToString("0.###") + " Qa + Active Venting " + Num(env["pav"]).ToString("0.###") + " Qa"); }
                if (!any) { status.Text = "OVER_BUDGET: " + string.Join("; ", over) + " — больше бюджета " + Num(env["budget"]).ToString("0.###") + " Qa. Топливо не заменяется: уменьшите количество или разрешите другие стержни."; return; }
                if (over.Count > 0) Log("не влезает в бюджет и пропущено: " + string.Join("; ", over));
            }
            var sel = Sel(); baseFile = null;
            if (m == "improve" || m == "tile")
            {
                if (sel == null || sel.Fuel == "early") { status.Text = m == "tile" ? "Выберите в списке схему текущей модели, из неё и будут найдены блоки." : "Выберите в списке схему текущей модели, её и будем улучшать."; return; }
                if (sel.Fuel != val["uiFuel"]) { status.Text = "Выбранная схема на топливе " + FuelCatalog.All.First(f => f.Id == sel.Fuel).Name + ", а сверху выбрано " + FuelCatalog.All.First(f => f.Id == val["uiFuel"]).Name + ". Выберите то же топливо."; return; }
                if (m == "improve") baseFile = sel.Path;
            }
            if (m == "random") baseFile = Path.Combine(Results, "full-random", "reference-layout.txt");
            methodDir = Path.Combine(Results, m); workDir = Path.Combine(methodDir, "work");
            try { if (Directory.Exists(workDir)) Directory.Delete(workDir, true); } catch { }
            Directory.CreateDirectory(workDir);
            finalFile = Path.Combine(methodDir, m + (val["uiFuel"] != "protium" ? "-" + val["uiFuel"] : "") + "-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".txt");
            int seeds = m == "improve" || m == "tile" ? 0 : SeedFromLibrary();
            stages = Plan(m, T, sel); stageIdx = -1; aborted = false; pickInfo = null; bestLive = null; started = DateTime.Now; SaveIni();
            Log(""); Log((exact > 0 ? "Точно " + exact + " топливных клеток. " : "") + "Способ: " + Methods.First(x => x[0] == m)[1] + ", топливо " + FuelCatalog.All.First(f => f.Id == val["uiFuel"]).Name
                + ", время " + (T < 60 ? T + " с" : (T / 60) + " мин") + (seeds > 0 ? ", из библиотеки взято лучших схем: " + seeds : ""));
            SetGo(true); NextStage();
        }
        void NextStage()
        {
            while (true)
            {
                stageIdx++;
                if (stageIdx >= stages.Count) { Finish(); return; }
                var s = stages[stageIdx];
                if (s.Expand != null) { var more = s.Expand(); stages.InsertRange(stageIdx + 1, more); continue; }
                if (s.Need != null && !File.Exists(s.Need)) { Log("пропуск: " + s.Label + " (нет входных данных)"); continue; }
                var dir = s.Pick ? workDir : Path.IsPathRooted(s.Dir) ? s.Dir : Path.Combine(workDir, s.Dir); Directory.CreateDirectory(dir);
                var env = EnvFor(val["uiFuel"]); foreach (var kv in s.Env) env[kv.Key] = kv.Value;
                if (s.Tool == "an" && env["anMode"] == "1") { var st0 = LayoutFile.Parse(env["anStart"]); if (st0 != null) { env["anW"] = st0.W.ToString(); env["anH"] = st0.H.ToString(); } }
                stageStarted = DateTime.Now; Log("--- этап " + (stageIdx + 1) + ": " + s.Label);
                try { proc = Engine.Start("--tool " + s.Tool, dir, env, OnLine, code => { proc = null; if (aborted) { Done("Остановлено."); return; } NextStage(); }); }
                catch (Exception ex) { Done("Движок не запустился: " + ex.Message); return; }
                ShowProgress(); return;
            }
        }
        void OnLine(string line)
        {
            Log(line);
            if (line.StartsWith("SCORE\t") && stages != null && stageIdx < stages.Count && stages[stageIdx].Gen != null) { var gn = stages[stageIdx].Gen; gn.Score = Num(line.Split('\t')[1]); return; }
            int k = line.IndexOf("новый лучший:"); if (k >= 0) bestLive = line.Substring(k + 14).Split(',')[0].Trim();
            if (line.StartsWith("НЕ ЛУЧШЕ:")) { pickInfo = line; return; }
            if (line.StartsWith("ПОДСКАЗКА:")) pickInfo = line.Substring(10).Trim(); else if ((line.StartsWith("отбор:") || line.StartsWith("ЛУЧШИЙ:")) && (pickInfo == null || !pickInfo.StartsWith("если"))) pickInfo = line;
        }
        void ShowProgress()
        {
            if (stages == null || stageIdx < 0 || stageIdx >= stages.Count) return;
            prog.Value = (double)stageIdx / stages.Count;
            status.Text = "Этап " + (stageIdx + 1) + " из " + stages.Count + ": " + stages[stageIdx].Label + ", " + Dur((DateTime.Now - stageStarted).TotalSeconds)
                + (bestLive != null ? ".  Лучшее пока " + bestLive : "") + ".  Всего " + Dur((DateTime.Now - started).TotalSeconds);
        }
        void Finish()
        {
            string stem = finalFile.Substring(0, finalFile.Length - 4), alt = stem + "-без-запаса.txt";
            LoadRunBest(stem + ".top.tsv");
            Func<string, string> desc = f => { var l = LayoutFile.Parse(f); var sd = l == null ? null : LayoutFile.Std(l); return sd == null ? "" : "мощность " + (sd[0] / 1000).ToString("0.###") + " T/тик, цена " + sd[3].ToString("0.###") + " Qa"; };
            string more = runBest.Count > 1 ? " Лучших разных вариантов: " + runBest.Count + ", переключайте стрелками ← →." : "";
            var mainSrc = runBest.Count > 0 ? runBest[0].Src ?? "" : "";
            string fromLib = mainSrc.IndexOf("\\seed\\", StringComparison.OrdinalIgnoreCase) >= 0 ? "Лучше уже найденной в библиотеке схемы не нашлось. " : "";
            if (File.Exists(finalFile))
            {
                var l = LayoutFile.Parse(finalFile); bool same = baseFile != null && l != null && LayoutFile.Parse(baseFile) != null && LayoutFile.Parse(baseFile).G == l.G;
                Done(fromLib + (same ? (val["uiMethod"] == "random" ? "Лучше эталона full-random не нашлось: " : "Лучше выбранной схемы не нашлось, подобраны только улучшения: ") : "Готово, защищённая схема: ") + desc(finalFile) + "."
                    + (File.Exists(alt) ? " Есть и выгоднее без аварийного запаса: " + desc(alt) + "." : "") + more);
                ScanLayouts(methodDir, started); selPath = finalFile; forceScan = true; ScanLayouts(null, null);
            }
            else if (File.Exists(alt))
            {
                Done(fromLib + "Защищённой от аварий схемы нет. Найдена схема без аварийного запаса (штатно и при остывании проходит, отключения Outlet/Vent не выдерживает): " + desc(alt) + "." + more);
                ScanLayouts(methodDir, started); selPath = alt; forceScan = true; ScanLayouts(null, null);
            }
            else if (val["uiMethod"] == "improve" && pickInfo != null && pickInfo.StartsWith("НЕ ЛУЧШЕ:")) { var t = pickInfo.Substring(9).Trim(); Done(char.ToUpper(t[0]) + t.Substring(1) + "."); }
            else { Done("Рабочей схемы не найдено. " + (pickInfo != null && pickInfo.StartsWith("если") ? char.ToUpper(pickInfo[0]) + pickInfo.Substring(1) + "." : (pickInfo ?? "") + " Попробуйте больше времени.")); logToggle.IsChecked = true; }
            ShowRunBar();
        }
        void LoadRunBest(string manifest)
        {
            runBest.Clear(); if (!File.Exists(manifest)) return;
            foreach (var line in File.ReadAllLines(manifest))
            {
                var f = line.Split('\t'); if (f.Length < 6 || !File.Exists(f[0])) continue;
                runBest.Add(new Best { File = f[0], Tier = (int)Num(f[1]), Net = Num(f[2]), Power = Num(f[3]), Cost = Num(f[4]), Fill = Num(f[5]), Src = f.Length > 6 ? f[6] : "" });
            }
        }
        void Done(string msg)
        {
            SetGo(false); stages = null;
            status.Text = msg + "  Время " + Dur((DateTime.Now - started).TotalSeconds) + ".";
            Log(msg);
            try { if (workDir != null && Directory.Exists(workDir)) Directory.Delete(workDir, true); } catch { }
        }
        static string Dur(double s) { return s < 90 ? s.ToString("0") + " с" : (s / 60).ToString("0.0") + " мин"; }
    }
}
