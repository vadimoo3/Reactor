// Выдвижная панель: уровни улучшений, бюджет, цены, константы игры (свёрнуты).
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
        // ---------- выдвижная панель сейва ----------
        Border BuildDrawer()
        {
            var d = new Border { Width = 520, HorizontalAlignment = HorizontalAlignment.Right, Background = B("Card"), BorderBrush = B("Line"), BorderThickness = new Thickness(1, 0, 0, 0), Visibility = Visibility.Collapsed };
            d.Effect = new System.Windows.Media.Effects.DropShadowEffect { BlurRadius = 30, ShadowDepth = 0, Opacity = 0.5 };
            var dock = new DockPanel(); d.Child = dock;
            var head = new DockPanel { Margin = new Thickness(24, 18, 16, 8) };
            var close = Btn(null, "Ghost", ""); close.ToolTip = "Закрыть"; close.Click += (s, e) => ToggleDrawer(); DockPanel.SetDock(close, Dock.Right); head.Children.Add(close);
            head.Children.Add(Text("Сейв и цены", 17, "Text", FontWeights.SemiBold));
            DockPanel.SetDock(head, Dock.Top); dock.Children.Add(head);
            drawerBody = new StackPanel { Margin = new Thickness(24, 0, 24, 24) };
            dock.Children.Add(new ScrollViewer { Content = drawerBody, VerticalScrollBarVisibility = ScrollBarVisibility.Auto });
            return d;
        }
        void ToggleDrawer()
        {
            if (drawer.Visibility == Visibility.Visible) { drawer.Visibility = Visibility.Collapsed; UpdateSaveSummary(); testCache.Clear(); RunTestsSoon(); RunEvalSoon(); return; }
            drawerBody.Children.Clear();
            drawerBody.Children.Add(Text("Текущие уровни из вашей игры. Поиск сам решает, купить ли +1 уровень, и считает эффект от уровня.", 12.5, "TextMuted"));
            AddGroup("Уровни улучшений", "save"); AddGroup("Бюджет и цены следующего уровня", "price");
            drawerBody.Children.Add(Divider(new Thickness(0, 4, 0, 8)));
            var dis = new ToggleButton { Style = Res<Style>("Disclosure"), Content = "Константы игры. Не меняются от сейва, трогать только если изменилась игра" };
            var body = new StackPanel { Visibility = Visibility.Collapsed, Margin = new Thickness(0, 8, 0, 0) };
            dis.Checked += (s, e) => body.Visibility = Visibility.Visible; dis.Unchecked += (s, e) => body.Visibility = Visibility.Collapsed;
            drawerBody.Children.Add(dis); drawerBody.Children.Add(body);
            foreach (var g in new[] { "engine", "early" })
            {
                body.Children.Add(Text(groupTitle.ContainsKey(g) ? groupTitle[g] : g, 13.5, "Text", FontWeights.SemiBold));
                var wp = new WrapPanel { Margin = new Thickness(0, 8, 0, 8) }; foreach (var p in pars.Where(x => x.Grp == g)) wp.Children.Add(Field(p, 140)); body.Children.Add(wp);
            }
            var reset = Btn("Вернуть значения игры", "Ghost", ""); reset.HorizontalAlignment = HorizontalAlignment.Left;
            reset.Click += (s, e) => { foreach (var p in pars.Where(x => x.Grp == "engine" || x.Grp == "early")) val[p.Key] = p.Def; drawer.Visibility = Visibility.Collapsed; ToggleDrawer(); };
            body.Children.Add(reset);
            drawer.Visibility = Visibility.Visible;
        }
        // Иконки = факт модели архива: все компоненты Ultimate (T5), Protium = Fuel7, Nefastium = Fuel6, Quad Dolorium (F) = Fuel5.
        void ApplyIconConfig()
        {
            foreach (var c in Parts.SpriteBase.Keys.ToList()) Parts.Tier[c] = 5;
            Parts.FuelMain = 7; Parts.FuelEarly = 6; Parts.FuelF = 5;
        }
        void AddGroup(string title, string g)
        {
            drawerBody.Children.Add(new TextBlock { Text = title, FontSize = 13.5, FontWeight = FontWeights.SemiBold, Margin = new Thickness(0, 14, 0, 8) });
            var wp = new WrapPanel(); foreach (var p in pars.Where(x => x.Grp == g)) wp.Children.Add(Field(p, 140)); drawerBody.Children.Add(wp);
        }

    }
}
