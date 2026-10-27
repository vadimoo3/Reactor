// Библиотека схем: сканирование results, сортировка, избранное, переименование, удаление с отменой, буфер обмена.
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
        // ---------- библиотека ----------
        bool ScanLayouts(string preferDir, DateTime? after)
        {
            var keep = selPath;
            var found = new List<Layout>();
            if (Directory.Exists(Results))
                foreach (var f in Directory.EnumerateFiles(Results, "*.txt", SearchOption.AllDirectories))
                { if (f.IndexOf("\\work\\", StringComparison.OrdinalIgnoreCase) >= 0 || f.IndexOf("\\.trash\\", StringComparison.OrdinalIgnoreCase) >= 0) continue; var l = LayoutFile.Parse(f); if (l != null) { l.Rel = f.Substring(Results.Length + 1).Replace('\\', '/'); found.Add(l); } }
            var favs = Favs();
            if (val["uiSort"] == "fav") found = found.Where(l => favs.Contains(l.Rel)).ToList();
            found = val["uiSort"] == "power" ? found.OrderByDescending(l => { var sd = LayoutFile.Std(l); return sd == null ? -1 : sd[0]; }).ThenByDescending(l => l.Time).ToList() : found.OrderByDescending(l => l.Time).ToList();
            if (!forceScan && found.Count == layouts.Count && found.Zip(layouts, (a, b) => a.Path == b.Path && a.Time == b.Time).All(x => x) && preferDir == null) return false;
            forceScan = false;
            rebuilding = true; layouts = found; library.Items.Clear();
            foreach (var l in layouts) library.Items.Add(LibItem(l));
            rebuilding = false;
            int sel = -1; bool gotFresh = false;
            if (preferDir != null && after != null)
            {
                var fresh = layouts.Where(l => l.Path.StartsWith(preferDir, StringComparison.OrdinalIgnoreCase) && l.Time >= after.Value.AddSeconds(-2)).FirstOrDefault();
                if (fresh != null) { sel = layouts.IndexOf(fresh); gotFresh = true; }
            }
            if (sel < 0 && keep != null) sel = layouts.FindIndex(l => l.Path == keep);
            if (sel < 0) sel = layouts.FindIndex(l => l.Rel == "protium/final.txt");
            if (sel < 0 && layouts.Count > 0) sel = 0;
            library.SelectedIndex = sel; if (sel >= 0) { var it = library.Items[sel]; Dispatcher.BeginInvoke(new Action(() => library.ScrollIntoView(it)), DispatcherPriority.Loaded); }
            if (layouts.Count == 0 || sel < 0) ShowLayout();
            return gotFresh;
        }
        UIElement LibItem(Layout l)
        {
            var g = new DockPanel();
            var mini = new MiniGrid(l) { Margin = new Thickness(0, 0, 10, 0) }; g.Children.Add(mini);
            var sp = new StackPanel { VerticalAlignment = VerticalAlignment.Center };
            sp.Children.Add(new TextBlock { Text = (Favs().Contains(l.Rel) ? "\u2605 " : "") + Path.GetFileNameWithoutExtension(l.Path), FontWeight = FontWeights.SemiBold, TextTrimming = TextTrimming.CharacterEllipsis });
            var std = LayoutFile.Std(l); string folder = l.Rel.Contains('/') ? l.Rel.Substring(0, l.Rel.LastIndexOf('/')) : "";
            string sub = folder + ",  " + l.Time.ToString("dd.MM HH:mm") + (std != null ? ",  " + (std[0] / 1000).ToString("0.###") + " T/тик" : "");
            sp.Children.Add(new TextBlock { Text = sub, FontSize = 12, Foreground = B("TextMuted"), TextTrimming = TextTrimming.CharacterEllipsis });
            g.Children.Add(sp); g.ToolTip = l.Rel;
            return g;
        }
        HashSet<string> Favs() { return new HashSet<string>(val["uiFav"].Split(new[] { '|' }, StringSplitOptions.RemoveEmptyEntries)); }
        void SetFav(bool on)
        {
            var l = Sel(); if (l == null || rebuilding) return; var f = Favs(); if (on) f.Add(l.Rel); else f.Remove(l.Rel);
            val["uiFav"] = string.Join("|", f); SaveIni(); RefreshItem(l);
        }
        void RefreshItem(Layout l) { int i = layouts.IndexOf(l); if (i < 0) return; rebuilding = true; library.Items[i] = LibItem(l); library.SelectedIndex = i; rebuilding = false; }
        void StartRename()
        {
            var l = Sel(); if (l == null) return;
            renameBox.Text = Path.GetFileNameWithoutExtension(l.Path); viewTitle.Visibility = Visibility.Collapsed; renameBox.Visibility = Visibility.Visible;
            renameBox.Focus(); renameBox.SelectAll();
        }
        void CommitRename()
        {
            var l = Sel(); renameBox.Visibility = Visibility.Collapsed; viewTitle.Visibility = Visibility.Visible; if (l == null) return;
            var name = new string(renameBox.Text.Trim().Where(c => Array.IndexOf(Path.GetInvalidFileNameChars(), c) < 0).ToArray());
            if (name.Length == 0 || name == Path.GetFileNameWithoutExtension(l.Path)) return;
            var target = Path.Combine(Path.GetDirectoryName(l.Path), name + ".txt");
            if (File.Exists(target)) { status.Text = "Схема с именем «" + name + "» уже есть в этой папке."; return; }
            try { File.Move(l.Path, target); } catch (Exception ex) { status.Text = "Не удалось переименовать: " + ex.Message; return; }
            var newRel = target.Substring(Results.Length + 1).Replace('\\', '/'); var f = Favs();
            if (f.Remove(l.Rel)) { f.Add(newRel); val["uiFav"] = string.Join("|", f); SaveIni(); }
            selPath = target; forceScan = true; ScanLayouts(null, null);
        }
        // Буфер обмена бывает занят другим приложением: несколько попыток.
        static bool Retry(Action a) { for (int i = 0; i < 8; i++) { try { a(); return true; } catch { System.Threading.Thread.Sleep(60); } } return false; }
        string CopyText() { var l = Sel(); return l == null ? null : File.ReadAllText(l.Path); }
        void CopyLayout()
        {
            var t = CopyText(); if (t == null) return;
            status.Text = Retry(() => Clipboard.SetText(t)) ? "Схема «" + Path.GetFileNameWithoutExtension(Sel().Path) + "» скопирована в буфер." : "Буфер обмена занят другой программой, попробуйте ещё раз.";
        }
        void SaveAs()
        {
            var l = Sel(); if (l == null) return;
            var d = new Microsoft.Win32.SaveFileDialog { FileName = Path.GetFileNameWithoutExtension(l.Path), Filter = "Схема (*.txt)|*.txt|Картинка схемы (*.png)|*.png" };
            if (d.ShowDialog(this) != true) return;
            try
            {
                if (d.FileName.EndsWith(".png", StringComparison.OrdinalIgnoreCase)) SavePicture(l, d.FileName); else File.Copy(l.Path, d.FileName, true);
                status.Text = "Сохранено: " + d.FileName;
            }
            catch (Exception ex) { status.Text = "Не удалось сохранить: " + ex.Message; }
        }
        // Картинка схемы в духе draw.py / draw-results.py архива: заголовок, цифры, сетка с иконками, состав, результаты проверки.
        public void SavePicture(Layout l, string file)
        {
            var root = new StackPanel { Background = B("Bg"), Width = 1400 };
            var pad = new StackPanel { Margin = new Thickness(40, 32, 40, 32) }; root.Children.Add(pad);
            pad.Children.Add(new TextBlock { Text = Path.GetFileNameWithoutExtension(l.Path), FontSize = 30, FontWeight = FontWeights.SemiBold, Foreground = B("Text") });
            pad.Children.Add(Text(l.Rel + ",  " + l.W + "×" + l.H + (l.Fuel == "early" ? ",  ранняя модель (Nefelium)" : ",  топливо " + FuelCatalog.All.First(f => f.Id == l.Fuel).Name), 16, "TextMuted"));
            var sd = LayoutFile.Std(l); var nums = new WrapPanel { Margin = new Thickness(0, 18, 0, 18) };
            Action<string, string> stat = (k, v) => { var sp = new StackPanel { Margin = new Thickness(0, 0, 40, 0) }; sp.Children.Add(Text(k, 15, "TextMuted")); sp.Children.Add(new TextBlock { Text = v, FontSize = 24, FontFamily = Res<FontFamily>("Mono"), Foreground = B("Text") }); nums.Children.Add(sp); };
            if (sd != null)
            {
                stat("Мощность", (sd[0] / 1000).ToString("0.###") + " T/тик"); stat("Тепло", (sd[1] / 1000).ToString("0.###") + " T/тик"); stat("Охлаждение", (sd[2] / 1000).ToString("0.###") + " T/тик"); stat("Цена", sd[3].ToString("0.###") + " Qa");
                string[] up = { "Active Venting", "Heat Sinks", "Heat Exch.", "Reinforced", "Chrono" }; var b = Enumerable.Range(0, 5).Where(i => sd[4 + i] == 1).Select(i => up[i]).ToList();
                stat("Купить +1", b.Count == 0 ? "ничего" : string.Join(", ", b));
            }
            else if (l.Header.Length > 0) nums.Children.Add(Text(l.Header, 15, "TextMuted"));
            pad.Children.Add(nums);
            var gv = new GridView { L = l, Width = 1320, Height = 1320.0 / l.W * l.H }; pad.Children.Add(gv);
            var cnt = new WrapPanel { Margin = new Thickness(0, 20, 0, 0) };
            foreach (char c in Parts.Chars)
            {
                int n = l.G.Count(x => x == c); if (n == 0) continue;
                var sp = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 28, 10) };
                sp.Children.Add(Parts.Badge(c, 34, l.Fuel)); sp.Children.Add(new TextBlock { Text = n + " × " + Parts.Name[c], FontSize = 18, Foreground = B("Text"), Margin = new Thickness(10, 0, 0, 0), VerticalAlignment = VerticalAlignment.Center });
                cnt.Children.Add(sp);
            }
            pad.Children.Add(cnt);
            List<string> lines; if (testCache.TryGetValue(CacheKey(l), out lines))
                pad.Children.Add(Text("Проверка модели: " + lines.Count(x => x.EndsWith("PASS")) + " из " + lines.Count(x => x.EndsWith("PASS") || x.EndsWith("FAIL")) + " сценариев прошли (2 порядка обмена × штатно, Outlet off, Vent off, замена топлива).", 16, "TextMuted"));
            pad.Children.Add(Text("Модель архива, не игровой движок. Reactor Lab.", 14, "TextMuted"));
            root.Measure(new Size(1400, double.PositiveInfinity)); root.Arrange(new Rect(root.DesiredSize)); root.UpdateLayout();
            var rtb = new RenderTargetBitmap((int)root.ActualWidth, (int)root.ActualHeight, 96, 96, PixelFormats.Pbgra32); rtb.Render(root);
            var enc = new PngBitmapEncoder(); enc.Frames.Add(BitmapFrame.Create(rtb)); using (var f = File.Create(file)) enc.Save(f);
        }
        void PasteLayout()
        {
            string t = ""; if (!Retry(() => t = Clipboard.GetText())) { status.Text = "Буфер обмена занят другой программой, попробуйте ещё раз."; return; }
            PasteText(t);
        }
        void PasteText(string t)
        {
            var dir = Path.Combine(Results, "imported"); Directory.CreateDirectory(dir);
            var tmp = Path.Combine(dir, "imported-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".txt");
            File.WriteAllText(tmp, t ?? "");
            if (LayoutFile.Parse(tmp) == null) { File.Delete(tmp); status.Text = "В буфере обмена нет схемы: нужны строки одинаковой длины из букв 1 2 4 E V O I C P K R."; return; }
            selPath = tmp; forceScan = true; ScanLayouts(null, null); status.Text = "Схема добавлена из буфера.";
        }
        void DeleteLayout()
        {
            var l = Sel(); if (l == null) return; int idx = library.SelectedIndex;
            var trash = Path.Combine(Results, ".trash"); Directory.CreateDirectory(trash);
            var to = Path.Combine(trash, DateTime.Now.ToString("yyyyMMdd-HHmmss-fff") + "__" + l.Rel.Replace("/", "__"));
            try { File.Move(l.Path, to); } catch (Exception ex) { status.Text = "Не удалось удалить: " + ex.Message; return; }
            undoFrom = l.Path; undoTo = to; undoText.Text = "Схема «" + Path.GetFileNameWithoutExtension(l.Path) + "» удалена.";
            undoBar.Visibility = Visibility.Visible; undoTimer.Stop(); undoTimer.Start();
            selPath = idx + 1 < layouts.Count ? layouts[idx + 1].Path : idx > 0 ? layouts[idx - 1].Path : null; forceScan = true; ScanLayouts(null, null);
        }
        void UndoDelete()
        {
            undoBar.Visibility = Visibility.Collapsed; undoTimer.Stop(); if (undoTo == null || !File.Exists(undoTo)) return;
            try { Directory.CreateDirectory(Path.GetDirectoryName(undoFrom)); File.Move(undoTo, undoFrom); } catch (Exception ex) { status.Text = "Не удалось вернуть: " + ex.Message; return; }
            selPath = undoFrom; undoTo = null; forceScan = true; ScanLayouts(null, null);
        }
        Layout Sel() { int i = library == null ? -1 : library.SelectedIndex; return i >= 0 && i < layouts.Count ? layouts[i] : null; }
        void OnKey(object s, KeyEventArgs e)
        {
            if (Keyboard.FocusedElement is TextBox || layouts.Count == 0) return;
            if (e.Key == Key.Escape) { if (compareWith != null) SetCompare(null); else if (runBest.Count > 0) { runBest.Clear(); ShowRunBar(); } e.Handled = true; return; }
            if ((e.Key == Key.Left || e.Key == Key.Right) && runBest.Count > 0 && !(Keyboard.FocusedElement is ComboBox)) { RunStep(e.Key == Key.Left ? -1 : 1); e.Handled = true; return; }
            int d = e.Key == Key.Up || e.Key == Key.Left ? -1 : e.Key == Key.Down || e.Key == Key.Right ? 1 : 0;
            if (d == 0 || Keyboard.FocusedElement is ComboBox) return;
            library.SelectedIndex = Math.Max(0, Math.Min(layouts.Count - 1, library.SelectedIndex + d)); library.ScrollIntoView(library.SelectedItem); e.Handled = true;
        }

    }
}
