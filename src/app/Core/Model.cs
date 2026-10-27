// Модель данных: параметр движка, инструмент, схема.
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
    class Param { public string Grp, Key, Label, Def, Opts; public char Kind; }
    class Tool { public string Id, Title, Dir, Desc; public string[] Groups; }
    class Layout { public string Path, Rel, Header, Fuel = "protium"; public int W, H; public string G = ""; public DateTime Time; public Dictionary<string, string> Profile = new Dictionary<string, string>(); }
}
