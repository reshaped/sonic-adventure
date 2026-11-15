// capture.exe OUT_DIR [SECONDS] [HZ]: records xemu's window, its client
// area, HZ times a second (default 60) as OUT_DIR\cNNNNNN.jpg, with
// OUT_DIR\index.txt giving each picture's milliseconds from the first.
// (no keyboard polling: Device Guard blocks a build that reads the keys)
// waits up to a minute for xemu's window (capture.sh starts it once run.sh
// has started xemu); stops when xemu exits, after
// SECONDS (default 1200), or when OUT_DIR\stop appears. the window is read
// from the screen, so it must stay uncovered. built and run by
// tools/xemu/capture.sh with the .NET Framework's csc.exe
using System;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;

static class Capture
{
    [StructLayout(LayoutKind.Sequential)] struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] struct POINT { public int X, Y; }
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr w, out RECT r);
    [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr w, ref POINT p);
    [DllImport("user32.dll")] static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] static extern bool IsIconic(IntPtr w);
    [DllImport("winmm.dll")] static extern uint timeBeginPeriod(uint ms);

    class Shot { public Bitmap Picture; public int Number; public string Dir; }

    static int Main(string[] args)
    {
        if (args.Length < 1) { Console.Error.WriteLine("capture.exe OUT_DIR [SECONDS] [HZ]"); return 2; }
        string dir = args[0];
        double seconds = args.Length > 1 ? double.Parse(args[1]) : 1200;
        double hz = args.Length > 2 ? double.Parse(args[2]) : 60;

        SetProcessDPIAware();
        timeBeginPeriod(1);
        Directory.CreateDirectory(dir);
        string stop = Path.Combine(dir, "stop");
        if (File.Exists(stop)) File.Delete(stop);

        Process xemu = null;
        for (int i = 0; i < 600 && xemu == null; i++) {
            xemu = Process.GetProcessesByName("xemu").FirstOrDefault(p => p.MainWindowHandle != IntPtr.Zero);
            if (xemu == null) Thread.Sleep(100);
        }
        if (xemu == null) { Console.Error.WriteLine("capture: no xemu window"); return 1; }
        IntPtr window = xemu.MainWindowHandle;
        Console.WriteLine("capture: xemu's window found");

        var jpeg = ImageCodecInfo.GetImageEncoders().First(c => c.MimeType == "image/jpeg");
        var quality = new EncoderParameters(1);
        quality.Param[0] = new EncoderParameter(System.Drawing.Imaging.Encoder.Quality, 90L);
        var queue = new BlockingCollection<Shot>(64);
        var index = new StreamWriter(Path.Combine(dir, "index.txt"));
        var workers = Enumerable.Range(0, 3).Select(_ => new Thread(() => {
            foreach (var shot in queue.GetConsumingEnumerable()) {
                shot.Picture.Save(Path.Combine(shot.Dir, string.Format("c{0:D6}.jpg", shot.Number)), jpeg, quality);
                shot.Picture.Dispose();
            }
        })).ToArray();
        foreach (var t in workers) t.Start();

        var clock = Stopwatch.StartNew();
        double period = 1000.0 / hz;
        int number = 0, dropped = 0;
        double next = 0;
        while (clock.Elapsed.TotalSeconds < seconds && !xemu.HasExited && !File.Exists(stop)) {
            double now = clock.Elapsed.TotalMilliseconds;
            if (now < next) { Thread.Sleep(Math.Max(0, (int)(next - now) - 1)); continue; }
            next = Math.Max(next + period, now - period);
            RECT r; POINT at = new POINT();
            if (IsIconic(window) || !GetClientRect(window, out r) || !ClientToScreen(window, ref at) ||
                r.Right <= 0 || r.Bottom <= 0) continue;
            var picture = new Bitmap(r.Right, r.Bottom, PixelFormat.Format24bppRgb);
            using (var g = Graphics.FromImage(picture)) {
                try { g.CopyFromScreen(at.X, at.Y, 0, 0, new Size(r.Right, r.Bottom)); }
                catch (Exception) { picture.Dispose(); continue; } // the screen locked, say
            }
            var shot = new Shot { Picture = picture, Number = number, Dir = dir };
            index.WriteLine("{0} {1:F1}", number, now);
            number++;
            if (!queue.TryAdd(shot, 200)) { picture.Dispose(); dropped++; continue; }
        }
        queue.CompleteAdding();
        foreach (var t in workers) t.Join();
        index.Close();
        Console.WriteLine("capture: {0} pictures in {1:F0} s, {2} dropped", number, clock.Elapsed.TotalSeconds, dropped);
        return 0;
    }
}
