// Компоненты: названия, цвета, спрайты (Ultimate = T5; Protium = Fuel7, Nefastium = Fuel6, Dolorium = Fuel5).
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
    static class Parts
    {
        public const string Chars = "124FEVOICPKR";
        public static readonly Dictionary<char, string> Name = new Dictionary<char, string> {
            {'1',"Single"},{'2',"Double"},{'4',"Quad"},{'F',"Nefelium / Dolorium"},{'E',"Exchanger"},{'V',"Vent"},{'O',"Outlet"},
            {'I',"Inlet"},{'C',"Coolant"},{'P',"Plating"},{'K',"Capacitor"},{'R',"Reflector"},{'.',"пусто"} };
        public static readonly Dictionary<char, Color> Fill = new Dictionary<char, Color> {
            {'1',C("#B4405A")},{'2',C("#C2603F")},{'4',C("#C98A2E")},{'F',C("#8A5A35")},{'E',C("#1F7A8C")},{'V',C("#2F7D5B")},{'O',C("#2E6E9E")},
            {'I',C("#9A7B2C")},{'C',C("#2B8FA8")},{'P',C("#4C5671")},{'K',C("#222B40")},{'R',C("#6D5BA8")},{'.',C("#0B1222")} };
        public static Color C(string h) { return (Color)ColorConverter.ConvertFromString(h); }
        public static Dictionary<string, ImageSource> Sprites = new Dictionary<string, ImageSource>(StringComparer.OrdinalIgnoreCase);
        public static readonly Dictionary<char, string> SpriteBase = new Dictionary<char, string> {
            {'E',"Exchanger"},{'V',"Vent"},{'O',"Outlet"},{'I',"Inlet"},{'C',"Coolant"},{'P',"Plate"},{'K',"Capacitor"},{'R',"Reflector"} };
        public static Dictionary<char, int> Tier = SpriteBase.Keys.ToDictionary(c => c, c => 5);
        public static int FuelMain = 7, FuelEarly = 6, FuelF = 5;
        // топливо архива -> номер спрайта: Protium = Fuel7, Nefastium = Fuel6, Dolorium = Fuel5
        public static int FuelSprite(string fuel) { return fuel == "monastium" ? 8 : fuel == "nefastium" || fuel == "early" ? 6 : fuel == "dolorium" ? 5 : 7; }
        public static ImageSource Icon(char c, string fuel) { return "124".IndexOf(c) >= 0 ? Sprite("Fuel" + FuelSprite(fuel) + "-" + c) : Icon(c, false); }
        public static FrameworkElement Badge(char c, double size, string fuel) { var img = Icon(c, fuel); return img != null ? SpriteBox(img, size) : Badge(c, size, false); }
        public static string SpriteName(char c, bool early)
        {
            if ("124".IndexOf(c) >= 0) return "Fuel" + (early ? FuelEarly : FuelMain) + "-" + c;
            if (c == 'F') return "Fuel" + FuelF + "-4";
            string b; return SpriteBase.TryGetValue(c, out b) ? b + Tier[c] : null;
        }
        public static ImageSource Sprite(string name) { ImageSource i; return name != null && Sprites.TryGetValue(name, out i) ? i : null; }
        public static ImageSource Icon(char c, bool early = false) { return Sprite(SpriteName(c, early)); }
        public static void LoadIcons(string dir)
        {
            Sprites.Clear();
            if (!Directory.Exists(dir)) return;
            foreach (var f in Directory.GetFiles(dir, "*.png"))
            {
                try
                {
                    var bi = new BitmapImage(); bi.BeginInit(); bi.CacheOption = BitmapCacheOption.OnLoad; bi.CreateOptions = BitmapCreateOptions.IgnoreImageCache;
                    bi.UriSource = new Uri(f); bi.EndInit(); bi.Freeze(); Sprites[Path.GetFileNameWithoutExtension(f)] = bi;
                }
                catch { }
            }
        }
        public static FrameworkElement SpriteBox(ImageSource img, double size)
        {
            var g = new Grid { Width = size, Height = size };
            g.Children.Add(new Border { Background = new SolidColorBrush(Color.FromRgb(0x0B, 0x12, 0x22)), CornerRadius = new CornerRadius(4) });
            var im = new Image { Source = img }; RenderOptions.SetBitmapScalingMode(im, BitmapScalingMode.NearestNeighbor); g.Children.Add(im);
            return g;
        }
        public static FrameworkElement Badge(char c, double size, bool early = false)
        {
            var img = Icon(c, early); if (img != null) return SpriteBox(img, size);
            var g = new Grid { Width = size, Height = size };
            g.Children.Add(new Border { Background = new SolidColorBrush(Fill.ContainsKey(c) ? Fill[c] : Colors.Gray), CornerRadius = new CornerRadius(4) });
            g.Children.Add(new TextBlock { Text = c.ToString(), FontWeight = FontWeights.Bold, FontSize = size * 0.55, HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center });
            return g;
        }
    }
}
