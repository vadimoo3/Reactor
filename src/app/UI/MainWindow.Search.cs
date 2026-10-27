// Левая колонка: способ, топливо, стержни, улучшения, время, кнопка поиска, журнал, список схем.
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
        // ---------- левая колонка ----------
        UIElement BuildLeft()
        {
            var col = new Grid { Background = B("Side") };
            col.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            col.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) });
            var top = new StackPanel { Margin = new Thickness(20, 16, 20, 12) };
            formHost = new StackPanel(); top.Children.Add(formHost);
            var run = new StackPanel { Margin = new Thickness(0, 4, 0, 0) };
            goBtn = Btn("Найти", "Primary", "\uE768"); goBtn.MinWidth = 150; goBtn.MinHeight = 42; goBtn.HorizontalAlignment = HorizontalAlignment.Left;
            goBtn.Click += (s, e) => { if (stages != null) { if ((DateTime.Now - started).TotalSeconds < 1.5) return; aborted = true; Kill(proc); if (proc == null) Done("Остановлено."); } else StartRun(); };
            status = Text("", 12.5, "TextMuted"); status.Margin = new Thickness(0, 10, 0, 0);
            prog = new ProgressBar { Visibility = Visibility.Collapsed, Margin = new Thickness(0, 8, 0, 0), Minimum = 0, Maximum = 1 };
            run.Children.Add(goBtn); run.Children.Add(status); run.Children.Add(prog); top.Children.Add(run);
            logToggle = new ToggleButton { Style = Res<Style>("Disclosure"), Content = "Журнал расчёта", Margin = new Thickness(0, 10, 0, 0) };
            log = new TextBox { IsReadOnly = true, Height = 160, TextWrapping = TextWrapping.NoWrap, VerticalScrollBarVisibility = ScrollBarVisibility.Auto, HorizontalScrollBarVisibility = ScrollBarVisibility.Auto, FontSize = 12, VerticalContentAlignment = VerticalAlignment.Top, Visibility = Visibility.Collapsed, Margin = new Thickness(0, 4, 0, 0) };
            logToggle.Checked += (s, e) => log.Visibility = Visibility.Visible; logToggle.Unchecked += (s, e) => log.Visibility = Visibility.Collapsed;
            top.Children.Add(logToggle); top.Children.Add(log);
            col.Children.Add(new ScrollViewer { Content = top, VerticalScrollBarVisibility = ScrollBarVisibility.Auto, MaxHeight = 700 });

            var lib = new DockPanel { Margin = new Thickness(10, 0, 10, 10) };
            var libHead = new DockPanel { Margin = new Thickness(10, 12, 0, 6) };
            var paste = Btn("Вставить", "Ghost", "\uE77F"); paste.ToolTip = "Добавить схему из буфера обмена"; paste.MinHeight = 28; paste.Padding = new Thickness(8, 2, 8, 2);
            paste.Click += (s, e) => PasteLayout();
            DockPanel.SetDock(paste, Dock.Right); libHead.Children.Add(paste);
            libHead.Children.Add(Text("Схемы", 15, "Text", FontWeights.SemiBold));
            DockPanel.SetDock(libHead, Dock.Top); lib.Children.Add(libHead);
            var sortSeg = new Border { Background = B("Field"), BorderBrush = B("Line"), BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(7), Margin = new Thickness(10, 0, 0, 8), HorizontalAlignment = HorizontalAlignment.Left };
            var sortRow = new StackPanel { Orientation = Orientation.Horizontal }; sortSeg.Child = sortRow; var sortBtns = new List<ToggleButton>();
            foreach (var m in new[] { new[] { "new", "Новые" }, new[] { "power", "Мощные" }, new[] { "fav", "Избранные" } })
            {
                var id = m[0]; var b = new ToggleButton { Style = Res<Style>("Seg"), Content = m[1], IsChecked = val["uiSort"] == id };
                b.Checked += (s, e) => { val["uiSort"] = id; foreach (var x in sortBtns) if (x != b) x.IsChecked = false; forceScan = true; ScanLayouts(null, null); };
                b.Unchecked += (s, e) => { if (val["uiSort"] == id) b.IsChecked = true; };
                sortBtns.Add(b); sortRow.Children.Add(b);
            }
            DockPanel.SetDock(sortSeg, Dock.Top); lib.Children.Add(sortSeg);
            undoBar = new Border { Background = B("Card"), BorderBrush = B("Border"), BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(6), Padding = new Thickness(10, 6, 6, 6), Margin = new Thickness(10, 0, 0, 8), Visibility = Visibility.Collapsed };
            var ub = new DockPanel(); var undoBtn = Btn("Вернуть", "Ghost"); undoBtn.MinHeight = 26; undoBtn.Padding = new Thickness(8, 2, 8, 2); undoBtn.Foreground = B("Accent");
            undoBtn.Click += (s, e) => UndoDelete(); DockPanel.SetDock(undoBtn, Dock.Right); ub.Children.Add(undoBtn);
            undoText = Text("", 12.5); undoText.VerticalAlignment = VerticalAlignment.Center; ub.Children.Add(undoText); undoBar.Child = ub;
            DockPanel.SetDock(undoBar, Dock.Top); lib.Children.Add(undoBar);
            undoTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(20) }; undoTimer.Tick += (s, e) => { undoTimer.Stop(); undoBar.Visibility = Visibility.Collapsed; };
            library = new ListBox();
            library.PreviewMouseLeftButtonDown += (s, e) =>
            {
                if ((Keyboard.Modifiers & ModifierKeys.Control) == 0) return;
                var item = ItemsControl.ContainerFromElement(library, e.OriginalSource as DependencyObject) as ListBoxItem; if (item == null) return;
                int i = library.ItemContainerGenerator.IndexFromContainer(item); if (i < 0 || i >= layouts.Count || layouts[i] == Sel()) return;
                SetCompare(layouts[i]); e.Handled = true;
            }; library.SelectionChanged += (s, e) => { if (rebuilding) return; var l = Sel(); if (l != null) selPath = l.Path; ShowLayout(); };
            VirtualizingPanel.SetIsVirtualizing(library, true);
            lib.Children.Add(library);
            var libWrap = new Border { BorderBrush = B("Line"), BorderThickness = new Thickness(0, 1, 0, 0), Child = lib }; Grid.SetRow(libWrap, 1); col.Children.Add(libWrap);
            return new Border { BorderBrush = B("Line"), BorderThickness = new Thickness(0, 0, 1, 0), Child = col };
        }

        void BuildForm()
        {
            formHost.Children.Clear(); methodBtn.Clear();
            formHost.Children.Add(Text("Способ поиска", 12.5, "TextMuted"));
            var goal = Text("Цель у всех способов одна: максимум чистой мощности (продажа минус заправка) при прохождении аварий и остывания.", 12, "TextMuted"); goal.Margin = new Thickness(0, 2, 0, 0); goal.Opacity = 0.8;
            formHost.Children.Add(goal);
            var cards = new StackPanel { Margin = new Thickness(-8, 4, -8, 12) };
            foreach (var m in Methods)
            {
                var sp = new StackPanel();
                sp.Children.Add(new TextBlock { Text = m[1], FontSize = 15, FontWeight = FontWeights.SemiBold });
                sp.Children.Add(Text(m[2], 12.5, "TextMuted"));
                var id = m[0]; var tb = new ToggleButton { Style = Res<Style>("Nav"), Content = sp, IsChecked = val["uiMethod"] == id };
                tb.Checked += (s, e) => { val["uiMethod"] = id; foreach (var kv in methodBtn) if (kv.Key != id) kv.Value.IsChecked = false; status.Text = ""; };
                tb.Unchecked += (s, e) => { if (val["uiMethod"] == id) tb.IsChecked = true; };
                methodBtn[id] = tb; cards.Children.Add(tb);
            }
            formHost.Children.Add(cards);
            formHost.Children.Add(Text("Топливо", 12.5, "TextMuted"));
            var fuelRow = new WrapPanel { Margin = new Thickness(0, 4, 0, 10) }; var fuelBtns = new List<ToggleButton>();
            foreach (var f in FuelCatalog.All)
            {
                var id = f.Id; var c = new StackPanel { Orientation = Orientation.Horizontal };
                var bd = Parts.Badge('1', 22, id); bd.Margin = new Thickness(0, 0, 8, 0); c.Children.Add(bd); c.Children.Add(new TextBlock { Text = f.Name, VerticalAlignment = VerticalAlignment.Center });
                var tb = new ToggleButton { Style = Res<Style>("Chip"), Content = c, IsChecked = val["uiFuel"] == id,
                    ToolTip = "Тепло " + f.Heat + " B, мощность " + f.Pow + " B на импульс; горит " + f.Life + " тиков" };
                tb.Checked += (s, e) => { val["uiFuel"] = id; foreach (var x in fuelBtns) if (x != tb) x.IsChecked = false; Dispatcher.BeginInvoke(new Action(BuildForm)); };
                tb.Unchecked += (s, e) => { if (val["uiFuel"] == id) tb.IsChecked = true; };
                fuelBtns.Add(tb); fuelRow.Children.Add(tb);
            }
            formHost.Children.Add(fuelRow);
            formHost.Children.Add(Chips("Стержни", pars.Where(p => p.Grp == "an" && p.Kind == 't' && !string.IsNullOrEmpty(p.Opts)).ToList()));
            var countRow = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 10) };
            var countBox = new TextBox { Text = val["uiCount"], Width = 56, IsEnabled = val["uiCountOn"] == "1", ToolTip = "Число топливных клеток (Quad = одна клетка)" };
            var countCheck = new CheckBox { Content = "Точное количество стержней", IsChecked = val["uiCountOn"] == "1", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 10, 0) };
            countCheck.Checked += (s, e) => { val["uiCountOn"] = "1"; countBox.IsEnabled = true; }; countCheck.Unchecked += (s, e) => { val["uiCountOn"] = "0"; countBox.IsEnabled = false; };
            countBox.TextChanged += (s, e) => { int v; bool ok = int.TryParse(countBox.Text, out v) && v > 0 && v <= 304; countBox.Tag = ok ? null : "err"; if (ok) val["uiCount"] = v.ToString(); };
            countRow.Children.Add(countCheck); countRow.Children.Add(countBox); formHost.Children.Add(countRow);
            formHost.Children.Add(Chips("Можно купить +1 уровень, поиск сам решает что выгодно", pars.Where(p => p.Grp == "an" && p.Kind == 't' && string.IsNullOrEmpty(p.Opts)).ToList()));
            formHost.Children.Add(Text("Время поиска", 12.5, "TextMuted"));
            var seg = new Border { Background = B("Field"), BorderBrush = B("Line"), BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(7), Margin = new Thickness(0, 4, 0, 14), HorizontalAlignment = HorizontalAlignment.Left };
            var row = new StackPanel { Orientation = Orientation.Horizontal }; seg.Child = row; var tbs = new List<ToggleButton>();
            foreach (var t in TimePresets)
            {
                var sec = t.ToString(); var b = new ToggleButton { Style = Res<Style>("Seg"), Content = t < 60 ? t + " сек" : (t / 60) + " мин", IsChecked = val["uiTime"] == sec };
                b.Checked += (s, e) => { val["uiTime"] = sec; foreach (var x in tbs) if (x != b) x.IsChecked = false; };
                b.Unchecked += (s, e) => { if (val["uiTime"] == sec) b.IsChecked = true; };
                tbs.Add(b); row.Children.Add(b);
            }
            if (!tbs.Any(x => x.IsChecked == true)) { val["uiTime"] = "60"; tbs[0].IsChecked = true; }
            formHost.Children.Add(seg);
        }
        UIElement Chips(string title, List<Param> ps)
        {
            var sp = new StackPanel { Margin = new Thickness(0, 0, 0, 8) };
            sp.Children.Add(Text(title, 12.5, "TextMuted"));
            var wp = new WrapPanel { Margin = new Thickness(0, 4, 0, 0) };
            foreach (var p in ps)
            {
                var c = new StackPanel { Orientation = Orientation.Horizontal };
                if (!string.IsNullOrEmpty(p.Opts)) { var bd = Parts.Badge(p.Opts[0], 22, val["uiFuel"]); bd.Margin = new Thickness(0, 0, 8, 0); c.Children.Add(bd); }
                c.Children.Add(new TextBlock { Text = p.Label.Replace("+1 ", ""), VerticalAlignment = VerticalAlignment.Center });
                bool off = FuelCatalog.Env(val["uiFuel"]).ContainsKey(p.Key);
                var tb = new ToggleButton { Style = Res<Style>("Chip"), Content = c, IsChecked = !off && val[p.Key] == "1" };
                if (off) { tb.IsEnabled = false; tb.Opacity = 0.45; tb.ToolTip = "Цены этого размера для " + FuelCatalog.All.First(f => f.Id == val["uiFuel"]).Name + " в архиве нет"; }
                else { tb.Checked += (s, e) => { val[p.Key] = "1"; RunEvalSoon(); }; tb.Unchecked += (s, e) => { val[p.Key] = "0"; RunEvalSoon(); }; }
                wp.Children.Add(tb);
            }
            sp.Children.Add(wp); return sp;
        }
        static string Tip(Param p) { return p.Key + ", по умолчанию " + (p.Def == "" ? "пусто" : p.Def); }
        FrameworkElement Field(Param p, double width = 158)
        {
            var sp = new StackPanel { Width = width, Margin = new Thickness(0, 0, 12, 12) };
            var lbl = Text(p.Label, 12.5, "TextMuted"); lbl.Margin = new Thickness(0, 0, 0, 4); lbl.ToolTip = Tip(p); sp.Children.Add(lbl);
            var tb = new TextBox { Text = val[p.Key], ToolTip = Tip(p) };
            var err = Text("Нужно число", 11.5, "Danger"); err.Visibility = Visibility.Collapsed; err.Margin = new Thickness(0, 3, 0, 0);
            Action check = () => { bool ok = IsNum(tb.Text); tb.Tag = ok ? null : "err"; err.Visibility = ok ? Visibility.Collapsed : Visibility.Visible; };
            tb.TextChanged += (s, e) => { val[p.Key] = tb.Text; check(); }; check();
            sp.Children.Add(tb); sp.Children.Add(err);
            return sp;
        }
        static bool IsNum(string s) { double d; return double.TryParse(s.Replace(',', '.'), NumberStyles.Float, CultureInfo.InvariantCulture, out d); }

    }
}
