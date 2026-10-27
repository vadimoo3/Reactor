// Калькулятор в углу: перенос tools/reactor_calculator.py (площадь и тепловой баланс).
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace ReactorLab
{
    partial class MainWindow
    {
        static readonly string[][] CalcFields = {
            new[] { "width", "Field width", "19" }, new[] { "height", "Field height", "20" },
            new[] { "reserved", "Reserved / unavailable cells", "30" },
            new[] { "groups", "Outlet + 4 Vent groups (5 cells/group)", "36" },
            new[] { "capacitors", "Capacitors (1 cell each)", "160" },
            new[] { "small", "Small fuel rods (1 cell each)", "1" },
            new[] { "medium", "Medium fuel rods (1 cell each)", "2" },
            new[] { "large", "Large fuel rods (1 cell each)", "1" },
            new[] { "extra", "EXTRA occupied cells (reflectors, etc.)", "3" },
            new[] { "group_base", "Base cooling / group", "72.8" },
            new[] { "cap_bonus", "Bonus / capacitor / group", "22.4" },
            new[] { "small_heat", "Heat / small rod", "1250" },
            new[] { "medium_heat", "Heat / medium rod", "20000" },
            new[] { "large_heat", "Heat / large rod", "90000" } };
        Dictionary<string, TextBox> calcIn = new Dictionary<string, TextBox>(); TextBlock calcOut, calcStatus;

        UIElement BuildCalculator()
        {
            var host = new StackPanel { HorizontalAlignment = HorizontalAlignment.Right, VerticalAlignment = VerticalAlignment.Bottom, Margin = new Thickness(0, 0, 18, 18) };
            var card = new Border { Background = B("Card"), BorderBrush = B("Line"), BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(8), Padding = new Thickness(14), Width = 420, Visibility = Visibility.Collapsed, Margin = new Thickness(0, 0, 0, 8) };
            card.Effect = new System.Windows.Media.Effects.DropShadowEffect { BlurRadius = 24, ShadowDepth = 0, Opacity = 0.5 };
            var body = new StackPanel(); card.Child = body;
            var grid = new Grid();
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(110) });
            for (int i = 0; i < CalcFields.Length; i++)
            {
                grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
                var label = Text(CalcFields[i][1], 12.5); label.VerticalAlignment = VerticalAlignment.Center; label.Margin = new Thickness(0, 2, 12, 2);
                var tb = new TextBox { Text = CalcFields[i][2], Margin = new Thickness(0, 2, 0, 2) }; tb.TextChanged += (s, e) => CalcUpdate();
                Grid.SetRow(label, i); Grid.SetRow(tb, i); Grid.SetColumn(tb, 1); grid.Children.Add(label); grid.Children.Add(tb);
                calcIn[CalcFields[i][0]] = tb;
            }
            body.Children.Add(grid);
            body.Children.Add(Divider(new Thickness(0, 10, 0, 10)));
            calcOut = new TextBlock { FontFamily = Res<System.Windows.Media.FontFamily>("Mono"), FontSize = 12.5, Foreground = B("Text") };
            body.Children.Add(calcOut);
            calcStatus = Text("", 12, "TextMuted"); calcStatus.Margin = new Thickness(0, 10, 0, 0); body.Children.Add(calcStatus);
            host.Children.Add(card);
            var toggle = Btn("Калькулятор", "Ghost"); toggle.HorizontalAlignment = HorizontalAlignment.Right;
            toggle.Click += (s, e) => card.Visibility = card.Visibility == Visibility.Visible ? Visibility.Collapsed : Visibility.Visible;
            host.Children.Add(toggle);
            CalcUpdate();
            return host;
        }

        void CalcUpdate()
        {
            if (calcOut == null || calcIn.Count < CalcFields.Length) return;
            var v = new Dictionary<string, double>();
            foreach (var kv in calcIn)
            {
                double d; if (!double.TryParse(kv.Value.Text.Trim().Replace(',', '.'), NumberStyles.Float, CultureInfo.InvariantCulture, out d) || double.IsInfinity(d) || double.IsNaN(d))
                { calcOut.Text = "Enter valid numbers in all fields."; calcStatus.Text = "could not convert string to float: '" + kv.Value.Text.Trim() + "'"; return; }
                v[kv.Key] = d;
            }
            string[] integers = { "width", "height", "reserved", "groups", "capacitors", "small", "medium", "large", "extra" };
            if (integers.Any(k => v[k] < 0 || v[k] != Math.Floor(v[k]))) { calcOut.Text = "Enter valid numbers in all fields."; calcStatus.Text = "Cell and component counts must be nonnegative integers."; return; }
            if (v.Values.Any(x => x < 0)) { calcOut.Text = "Enter valid numbers in all fields."; calcStatus.Text = "Values cannot be negative."; return; }
            long total = (long)(v["width"] * v["height"]);
            long usable = total - (long)v["reserved"];
            long fuelCells = (long)(v["small"] + v["medium"] + v["large"]);
            long used = (long)(5 * v["groups"] + v["capacitors"] + fuelCells + v["extra"]);
            long free = usable - used;
            double cooling = v["groups"] * (v["group_base"] + v["cap_bonus"] * v["capacitors"]);
            double heat = v["small"] * v["small_heat"] + v["medium"] * v["medium_heat"] + v["large"] * v["large_heat"];
            double margin = cooling - heat;
            Func<double, string> f = x => x.ToString("#,0.##", CultureInfo.InvariantCulture);
            Func<long, string> n = x => x.ToString("#,0", CultureInfo.InvariantCulture);
            calcOut.Text =
                "Field size:        " + n(total) + " cells\n" +
                "Usable:            " + n(usable) + " cells\n" +
                "Cooling groups:    " + n((long)v["groups"]) + " × 5 = " + n((long)(5 * v["groups"])) + " cells\n" +
                "Capacitors:        " + n((long)v["capacitors"]) + " cells\n" +
                "Fuel (all types):  " + n(fuelCells) + " cells\n" +
                "Additional cells:  " + n((long)v["extra"]) + " cells\n" +
                "Used / usable:     " + n(used) + " / " + n(usable) + "\n" +
                "Free:              " + n(free) + " cells\n\n" +
                "Generated heat:    " + f(heat) + " / tick\n" +
                "Cooling capacity:  " + f(cooling) + " / tick\n" +
                "Cooling reserve:   " + f(margin) + " / tick";
            var problems = new List<string>();
            if (usable < 0 || free < 0) problems.Add("OVER CAPACITY: Not enough free cells.");
            if (margin < 0) problems.Add("OVERHEATING: Heat exceeds theoretical cooling.");
            if (problems.Count == 0) problems.Add("Theoretical balance passes area and heat checks.");
            problems.Add("Note: geometry, Outlet throughput, adjacency and real game tick behavior are not simulated.");
            calcStatus.Text = string.Join("  ", problems);
        }
    }
}
