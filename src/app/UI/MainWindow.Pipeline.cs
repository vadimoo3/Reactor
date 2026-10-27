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
        static double Num(string s) { double v; double.TryParse((s ?? "").Replace(',', '.'), NumberStyles.Float, CultureInfo.InvariantCulture, out v); return v; }
        int ExactCount() { int c; return val["uiCountOn"] == "1" && int.TryParse(val["uiCount"], out c) && c > 0 ? c : 0; }
        int SeedFromLibrary()
        {
            var seeds = layouts.Where(l => (l.Fuel == val["uiFuel"] && LayoutFile.Std(l) != null) || l.Fuel == "early")
                .GroupBy(l => l.G).Select(g => g.First()).ToList();
            var dir = Path.Combine(workDir, "seed"); Directory.CreateDirectory(dir); int i = 0;
            foreach (var l in seeds) File.Copy(l.Path, Path.Combine(dir, "lib-" + (++i) + ".txt"), true);
            return i;
        }

        // ---------- план ----------
        List<Stage> Plan(string m, int T, Layout sel)
        {
            var stage = new Stage { Label = "единый поиск", Tool = "an", Dir = "an" };
            stage.Env["anStrategy"] = m;
            stage.Env["anSecs"] = T.ToString();
            stage.Env["anAttempts"] = "0";
            stage.Env["anThreads"] = Math.Max(1, Environment.ProcessorCount / 2).ToString();
            stage.Env["anSeed"] = new Random().Next(1, int.MaxValue).ToString();
            stage.Env["anVerify"] = Math.Max(24, 4 * T).ToString();
            stage.Env["anLibrary"] = Path.Combine(workDir, "seed");
            stage.Env["anOutFile"] = finalFile;
            stage.Env["anMinF"] = ExactCount().ToString();
            stage.Env["anMaxF"] = ExactCount().ToString();
            stage.Env["anMode"] = sel != null && (m == "improve" || m == "tile") ? "1" : "2";
            stage.Env["anStart"] = stage.Env["anMode"] == "1" ? sel.Path : "";
            if (sel != null && (m == "improve" || m == "tile")) foreach (var kv in EnvFor(sel)) if (sel.Profile.ContainsKey(kv.Key)) stage.Env[kv.Key] = kv.Value;
            return new List<Stage> { stage };
        }

        void StartRun()
        {
            string m = val["uiMethod"]; int T; if (!int.TryParse(val["uiTime"], out T) || !TimePresets.Contains(T)) T = 60;
            var fe = FuelCatalog.Env(val["uiFuel"]);
            if (!new[] { "1", "2", "4" }.Any(r => val["anA" + r] == "1" && !fe.ContainsKey("anA" + r))) { status.Text = "Включите хотя бы один доступный размер стержней."; return; }
            int exact = ExactCount();
            if (exact > 0)
            {
                var env = EnvFor(val["uiFuel"]); var over = new List<string>(); bool any = false;
                foreach (var t in new[] { 1, 2, 4 }) if (env["anA" + t] == "1") { double cost = exact * Num(env["f" + t]); if (cost <= Num(env["budget"])) any = true; else over.Add(exact + " × " + (t == 1 ? "Single" : t == 2 ? "Double" : "Quad") + ": топливо " + (exact * Num(env["f" + t])).ToString("0.###") + " Qa"); }
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
            methodDir = Path.Combine(Results, m); workDir = Path.Combine(methodDir, "runs", DateTime.Now.ToString("yyyyMMdd-HHmmss-fff"));
            Directory.CreateDirectory(workDir);
            finalFile = Path.Combine(methodDir, m + (val["uiFuel"] != "protium" ? "-" + val["uiFuel"] : "") + "-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".txt");
            int seeds = SeedFromLibrary();
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
                var dir = Path.IsPathRooted(s.Dir) ? s.Dir : Path.Combine(workDir, s.Dir); Directory.CreateDirectory(dir);
                var env = EnvFor(val["uiFuel"]); foreach (var kv in s.Env) env[kv.Key] = kv.Value;
                if (s.Tool == "an" && env["anMode"] == "1") { var st0 = LayoutFile.Parse(env["anStart"]); if (st0 != null) { env["anW"] = st0.W.ToString(); env["anH"] = st0.H.ToString(); } }
                stageStarted = DateTime.Now; Log("--- этап " + (stageIdx + 1) + ": " + s.Label);
                File.WriteAllLines(Path.Combine(workDir, "stage-" + stageIdx + ".params.txt"), env.OrderBy(kv => kv.Key).Select(kv => kv.Key + "=" + kv.Value));
                File.AppendAllText(Path.Combine(workDir, "stages.tsv"), stageIdx + "\t" + s.Tool + "\t" + dir + "\r\n");
                try { proc = Engine.Start("--tool " + s.Tool, dir, env, OnLine, code => { proc = null; if (aborted) { Done("Остановлено."); return; } NextStage(); }); }
                catch (Exception ex) { Done("Движок не запустился: " + ex.Message); return; }
                ShowProgress(); return;
            }
        }
        void OnLine(string line)
        {
            if (workDir != null) File.AppendAllText(Path.Combine(workDir, "run.log"), line + "\r\n");
            Log(line);
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
            string stem = finalFile.Substring(0, finalFile.Length - 4);
            LoadRunBest(stem + ".top.tsv");
            var best = runBest.OrderByDescending(x => x.Net).ThenBy(x => x.Cost).FirstOrDefault();
            if (best != null)
            {
                string reliability = best.Tier == 1 ? "защищённая схема" : "без аварийного запаса: штатно, при замене и выгорании проходит, отключения Outlet/Vent не выдерживает";
                string more = runBest.Count > 1 ? " Разных вариантов: " + runBest.Count + ", переключайте стрелками ← →." : "";
                Done("Готово, чистыми " + (best.Net / 1000).ToString("0.###") + " T/тик, производство " + (best.Power / 1000).ToString("0.###")
                    + " T/тик, цена " + best.Cost.ToString("0.###") + " Qa; " + reliability + "." + more);
                ScanLayouts(methodDir, started); selPath = best.File; forceScan = true; ScanLayouts(null, null);
            }
            else
            {
                Done("Рабочей схемы не найдено. " + (pickInfo ?? "") + " Параметры и кандидаты сохранены в протоколе.");
                logToggle.IsChecked = true;
            }
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
            if (workDir != null) Log("Протокол и кандидаты: " + workDir);
        }
        static string Dur(double s) { return s < 90 ? s.ToString("0") + " с" : (s / 60).ToString("0.0") + " мин"; }
    }
}
