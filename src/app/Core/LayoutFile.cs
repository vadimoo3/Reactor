// Файл схемы: первая строка — числа (мощность, тепло, охлаждение, цена, 5 флагов улучшений, номер), затем строки сетки.
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
    static class LayoutFile
    {
        public static Layout Parse(string path)
        {
            try
            {
                var fi = new FileInfo(path); if (fi.Length > 40000) return null;
                var rows = new List<string>(); string header = null; var profile = new Dictionary<string, string>();
                foreach (var raw in File.ReadAllLines(path))
                {
                    var l = raw.Trim();
                    if (l.StartsWith("@")) { int split = l.IndexOf('='); if (split > 1) profile[l.Substring(1, split - 1)] = l.Substring(split + 1); continue; }
                    if (l.Length >= 3 && l.All(ch => "124FEVOICPKRx.".IndexOf(ch) >= 0)) rows.Add(l);
                    else if (header == null && rows.Count == 0) header = l;
                }
                if (rows.Count < 3 || rows.Any(r => r.Length != rows[0].Length)) return null;
                var g = string.Concat(rows); var hdr = header ?? "";
                var name = Path.GetFileName(path).ToLowerInvariant();
                string fuel = g.IndexOf('F') >= 0 ? "early" : name.Contains("monastium") ? "monastium" : name.Contains("nefastium") ? "nefastium" : name.Contains("dolorium") ? "dolorium" : "protium";
                if (profile.ContainsKey("fuelName")) fuel = profile["fuelName"];
                return new Layout { Path = path, Header = hdr, W = rows[0].Length, H = rows.Count, G = g, Time = fi.LastWriteTime, Fuel = fuel, Profile = profile };
            }
            catch { return null; }
        }
        public static double[] Std(Layout l)
        {
            var nums = new List<double>(); foreach (var tok in l.Header.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries)) { double d; if (double.TryParse(tok, NumberStyles.Float, CultureInfo.InvariantCulture, out d)) nums.Add(d); else break; }
            return nums.Count >= 10 && nums.Skip(4).Take(5).All(x => x == 0 || x == 1) ? nums.ToArray() : null;
        }
    }
}
