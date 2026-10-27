// Миниатюра схемы в списке.
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
    class MiniGrid : FrameworkElement
    {
        readonly Layout L;
        public MiniGrid(Layout l) { L = l; Width = 57; Height = 48; }
        protected override void OnRender(DrawingContext dc)
        {
            double cs = Math.Min(Width / L.W, Height / L.H), ox = (Width - cs * L.W) / 2, oy = (Height - cs * L.H) / 2;
            dc.DrawRectangle(new SolidColorBrush(Color.FromRgb(0x0B, 0x11, 0x20)), null, new Rect(0, 0, Width, Height));
            for (int i = 0; i < L.W * L.H; i++)
            {
                Color c; if (!Parts.Fill.TryGetValue(L.G[i], out c)) c = Colors.Gray;
                dc.DrawRectangle(new SolidColorBrush(c), null, new Rect(ox + (i % L.W) * cs, oy + (i / L.W) * cs, Math.Max(1, cs - 0.4), Math.Max(1, cs - 0.4)));
            }
        }
    }
}
