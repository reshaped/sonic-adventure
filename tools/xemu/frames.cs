// frames.exe DIR learn | frames.exe DIR sort LABELS: reads the overlay's
// FRM number (xbox/port/port.c, row 14, columns 31 to 36 of Ninja's 16-pixel
// text) off each of capture.exe's pictures DIR\cNNNNNN.jpg. learn gathers
// the digit cells' looks into DIR\digits.png, one numbered tile a kind, and
// DIR\digits.txt; sort, given LABELS (the digit each tile shows, in order,
// as one string: '?' for a tile that isn't a digit), copies the first
// picture of each frame to DIR\frames\fNNNNN.jpg and writes DIR\frames.txt.
// frames.exe DIR flashes [N]: of DIR\frames\fNNNNN.jpg, the N frames
// (default 20) most unlike both neighbours where the neighbours are alike,
// a frame drawn wrong between two right ones, into DIR\flashes.txt.
// the overlay's text is at twice its size in a 1280-wide window
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;

static class Frames
{
    const int Grid = 12;

    static double[] Cell(Bitmap b, double scale, int k)
    {
        // the cell's 16 pixels, less a pixel's edge each side
        double x0 = (31 + k) * 16 * scale, y0 = 14 * 16 * scale, size = 16 * scale;
        var v = new double[Grid * Grid];
        for (int gy = 0; gy < Grid; gy++)
            for (int gx = 0; gx < Grid; gx++) {
                int x = (int)(x0 + (gx + 0.5) * size / Grid), y = (int)(y0 + (gy + 0.5) * size / Grid);
                if (x >= b.Width || y >= b.Height) continue;
                Color c = b.GetPixel(x, y);
                v[gy * Grid + gx] = c.R > 170 && c.G > 170 && c.B < 110 ? 1 : 0;
            }
        return v;
    }

    static double Distance(double[] a, double[] b)
    {
        double d = 0;
        for (int i = 0; i < a.Length; i++) d += Math.Abs(a[i] - b[i]);
        return d;
    }

    static int Main(string[] args)
    {
        string dir = args[0];
        var files = Directory.GetFiles(dir, "c*.jpg").OrderBy(f => f).ToArray();
        var kinds = new List<double[]>();
        var counts = new List<int>();
        if (args[1] == "learn") {
            for (int n = 0; n < files.Length; n += 3)
                using (var b = new Bitmap(files[n])) {
                    double scale = b.Width / 640.0;
                    for (int k = 0; k < 6; k++) {
                        var v = Cell(b, scale, k);
                        int best = -1; double bestD = 1e9;
                        for (int i = 0; i < kinds.Count; i++) { double d = Distance(v, kinds[i]); if (d < bestD) { bestD = d; best = i; } }
                        if (best >= 0 && bestD <= 2) { counts[best]++; continue; }
                        if (kinds.Count < 64) { kinds.Add(v); counts.Add(1); }
                    }
                }
            int tile = 40, cols = 16, rows = (kinds.Count + cols - 1) / cols;
            using (var sheet = new Bitmap(cols * tile, rows * tile))
            using (var g = Graphics.FromImage(sheet)) {
                g.Clear(Color.Gray);
                for (int i = 0; i < kinds.Count; i++) {
                    int ox = (i % cols) * tile, oy = (i / cols) * tile;
                    for (int p = 0; p < Grid * Grid; p++)
                        if (kinds[i][p] > 0) g.FillRectangle(Brushes.Black, ox + 2 + (p % Grid) * 2, oy + 2 + (p / Grid) * 2, 2, 2);
                    g.DrawString(i.ToString(), SystemFonts.DefaultFont, Brushes.Red, ox + 26, oy + 26);
                }
                sheet.Save(Path.Combine(dir, "digits.png"), ImageFormat.Png);
            }
            File.WriteAllLines(Path.Combine(dir, "digits.txt"),
                kinds.Select((v, i) => counts[i] + " " + string.Join("", v.Select(x => x > 0 ? "1" : "0"))));
            Console.WriteLine("frames: {0} kinds of cell from {1} pictures", kinds.Count, files.Length);
            return 0;
        }
        if (args[1] == "flashes") {
            int want = args.Length > 2 ? int.Parse(args[2]) : 20;
            var shots = Directory.GetFiles(Path.Combine(dir, "frames"), "f*.jpg").OrderBy(f => f).ToArray();
            var looks = new double[shots.Length][];
            var numbers = new int[shots.Length];
            for (int n = 0; n < shots.Length; n++)
                using (var b = new Bitmap(shots[n])) {
                    numbers[n] = int.Parse(Path.GetFileNameWithoutExtension(shots[n]).Substring(1));
                    // the picture as 64 by 48 means, the overlay's corner left out
                    var v = new double[64 * 48];
                    for (int y = 0; y < 48; y++)
                        for (int x = 0; x < 64; x++) {
                            if (x >= 42 && y >= 4 && y <= 26) continue;
                            Color c = b.GetPixel((int)((x + 0.5) * b.Width / 64), (int)((y + 0.5) * b.Height / 48));
                            v[y * 64 + x] = (c.R + c.G + c.B) / 3.0;
                        }
                    looks[n] = v;
                }
            var scores = new List<Tuple<double, int>>();
            for (int n = 1; n + 1 < shots.Length; n++) {
                if (numbers[n - 1] != numbers[n] - 1 || numbers[n + 1] != numbers[n] + 1) continue;
                double a = Distance(looks[n], looks[n - 1]), c = Distance(looks[n], looks[n + 1]), around = Distance(looks[n - 1], looks[n + 1]);
                scores.Add(Tuple.Create((Math.Min(a, c) - around) / looks[n].Length, numbers[n]));
            }
            var top = scores.OrderByDescending(t => t.Item1).Take(want)
                .Select(t => string.Format("f{0:D5} {1:F1}", t.Item2, t.Item1)).ToArray();
            File.WriteAllLines(Path.Combine(dir, "flashes.txt"), top);
            foreach (var line in top) Console.WriteLine(line);
            return 0;
        }
        string labels = args[2];
        var lines = File.ReadAllLines(Path.Combine(dir, "digits.txt"));
        var templates = lines.Select(l => l.Split(' ')[1].Select(ch => ch == '1' ? 1.0 : 0.0).ToArray()).ToArray();
        string outDir = Path.Combine(dir, "frames");
        Directory.CreateDirectory(outDir);
        var seen = new HashSet<int>();
        var log = new List<string>();
        int unread = 0;
        foreach (var f in files) {
            int frame = 0; bool ok = true;
            using (var b = new Bitmap(f)) {
                double scale = b.Width / 640.0;
                for (int k = 0; k < 6 && ok; k++) {
                    var v = Cell(b, scale, k);
                    int best = -1; double bestD = 1e9;
                    for (int i = 0; i < templates.Length; i++) { double d = Distance(v, templates[i]); if (d < bestD) { bestD = d; best = i; } }
                    if (best < 0 || best >= labels.Length || labels[best] < '0' || labels[best] > '9' || bestD > 14) ok = false;
                    else frame = frame * 10 + (labels[best] - '0');
                }
            }
            string name = Path.GetFileName(f);
            if (!ok) { unread++; log.Add(name + " ?"); continue; }
            log.Add(name + " " + frame);
            if (seen.Add(frame)) File.Copy(f, Path.Combine(outDir, string.Format("f{0:D5}.jpg", frame)), true);
        }
        File.WriteAllLines(Path.Combine(dir, "frames.txt"), log);
        var list = seen.OrderBy(x => x).ToList();
        int missing = list.Count > 0 ? list.Last() - list.First() + 1 - list.Count : 0;
        Console.WriteLine("frames: {0} frames, {1} to {2}, {3} missing between, {4} pictures unread",
            list.Count, list.FirstOrDefault(), list.LastOrDefault(), missing, unread);
        return 0;
    }
}
