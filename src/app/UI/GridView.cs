// Крупная сетка выбранной схемы с наведением на клетку.
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
    class GridView : FrameworkElement
    {
        public Layout L, Compare; public int Hover = -1; public event Action<int> HoverChanged;
        public GridView() { RenderOptions.SetBitmapScalingMode(this, BitmapScalingMode.NearestNeighbor); Cursor = Cursors.Cross; }
        double Cell(out double ox, out double oy)
        {
            ox = oy = 0; if (L == null || L.W == 0) return 0;
            double cs = Math.Floor(Math.Min(ActualWidth / L.W, ActualHeight / L.H)); cs = Math.Min(cs, 64);
            ox = Math.Floor((ActualWidth - cs * L.W) / 2); oy = 0; return cs;
        }
        protected override void OnRender(DrawingContext dc)
        {
            dc.DrawRectangle(Brushes.Transparent, null, new Rect(0, 0, ActualWidth, ActualHeight));
            double ox, oy, cs = Cell(out ox, out oy); if (cs < 3) return;
            var tf = new Typeface(new FontFamily("Segoe UI"), FontStyles.Normal, FontWeights.Bold, FontStretches.Normal);
            double dpi = VisualTreeHelper.GetDpi(this).PixelsPerDip;
            var fuel = L.Fuel; var cellBg = new SolidColorBrush(Color.FromRgb(0x0B, 0x12, 0x22));
            for (int i = 0; i < L.W * L.H; i++)
            {
                char c = L.G[i]; var r = new Rect(ox + (i % L.W) * cs, oy + (i / L.W) * cs, cs - 1, cs - 1);
                var img = Parts.Icon(c, fuel);
                if (img != null) { dc.DrawRectangle(cellBg, null, r); dc.DrawImage(img, r); continue; }
                Color col; if (!Parts.Fill.TryGetValue(c, out col)) col = Colors.Gray;
                dc.DrawRoundedRectangle(new SolidColorBrush(col), null, r, 3, 3);
                if (cs >= 12 && c != '.')
                {
                    var ft = new FormattedText(c.ToString(), CultureInfo.InvariantCulture, FlowDirection.LeftToRight, tf, cs * 0.45, c == 'K' ? new SolidColorBrush(Color.FromRgb(0x64, 0x74, 0x8B)) : Brushes.White, dpi);
                    dc.DrawText(ft, new Point(r.X + (r.Width - ft.Width) / 2, r.Y + (r.Height - ft.Height) / 2));
                }
            }
            if (Compare != null && Compare.W == L.W && Compare.H == L.H)
            {
                var pen = new Pen(new SolidColorBrush(Color.FromRgb(0xF5, 0x9E, 0x0B)), Math.Max(2, cs * 0.08));
                for (int i = 0; i < L.W * L.H; i++) if (L.G[i] != Compare.G[i]) dc.DrawRoundedRectangle(null, pen, new Rect(ox + (i % L.W) * cs + 1, oy + (i / L.W) * cs + 1, cs - 3, cs - 3), 3, 3);
            }
            if (Hover >= 0 && Hover < L.W * L.H)
                dc.DrawRoundedRectangle(null, new Pen(Brushes.White, 2), new Rect(ox + (Hover % L.W) * cs - 1, oy + (Hover / L.W) * cs - 1, cs + 1, cs + 1), 4, 4);
        }
        protected override Size MeasureOverride(Size a) { return new Size(0, 0); }
        protected override void OnMouseMove(MouseEventArgs e)
        {
            double ox, oy, cs = Cell(out ox, out oy); int h = -1;
            if (cs > 0) { var p = e.GetPosition(this); int x = (int)((p.X - ox) / cs), y = (int)((p.Y - oy) / cs); if (p.X >= ox && p.Y >= oy && x < L.W && y < L.H) h = y * L.W + x; }
            if (h != Hover) { Hover = h; InvalidateVisual(); if (HoverChanged != null) HoverChanged(h); }
        }
        protected override void OnMouseLeave(MouseEventArgs e) { Hover = -1; InvalidateVisual(); if (HoverChanged != null) HoverChanged(-1); }
    }
}
