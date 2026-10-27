// Топлива архива с известными значениями (README reactor-protium, reactor-search). Факты, не настройки.
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
    class Fuel { public string Id, Name; public string Heat, Pow, F1, F2, F4, Life; }
    static class FuelCatalog
    {
        // Известные значения топлив из README архива (reactor-protium, reactor-search). Это факты, не настройки.
        public static readonly Fuel[] All = {
            new Fuel { Id = "protium", Name = "Protium", Heat = "1250", Pow = "2500", F1 = "3", F2 = "6.6", F4 = "14.52", Life = "3600" },
            new Fuel { Id = "nefastium", Name = "Nefastium", Heat = "51.87", Pow = "51.87", F1 = "3.586", F2 = "7.889", F4 = "17.356", Life = "86400" },
            new Fuel { Id = "dolorium", Name = "Dolorium", Heat = "0.226966", Pow = "0.453932", F1 = null, F2 = null, F4 = "0.018982", Life = "21600" },
            new Fuel { Id = "monastium", Name = "Monastium", Heat = "2000", Pow = "16000", F1 = "288", F2 = "633.6", F4 = "1452", Life = "345600" } };
        public static Dictionary<string, string> Env(string id)
        {
            var f = All.FirstOrDefault(x => x.Id == id) ?? All[0]; var d = new Dictionary<string, string>();
            d["fuelHeat"] = f.Heat; d["fuelPow"] = f.Pow; d["f4"] = f.F4; if (f.F1 != null) d["f1"] = f.F1; if (f.F2 != null) d["f2"] = f.F2;
            d["fuelLife"] = f.Life; d["expStart"] = f.Life; d["expEnd"] = (2 * long.Parse(f.Life)).ToString(); d["fuelName"] = f.Id;
            if (f.F1 == null) d["anA1"] = "0"; if (f.F2 == null) d["anA2"] = "0";
            return d;
        }
    }
}
