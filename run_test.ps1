# Run bin/Debug/reSA.exe, auto-dismissing any "Microsoft Visual C++ Runtime Library"
# debug dialogs (clicks "Пропустить"/Ignore = ID 0x00F5), capture a screenshot, print the log tail.
#
# Usage:  powershell -ExecutionPolicy Bypass -File .\run_test.ps1 [-Seconds 60] [-Shot menu.png]
param([int]$Seconds = 60, [string]$Shot = "")

$ErrorActionPreference = 'SilentlyContinue'
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Text;using System.Runtime.InteropServices;
public class Dlg{
 [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr l);
 [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr h, EnumWindowsProc cb, IntPtr l);
 public delegate bool EnumWindowsProc(IntPtr h, IntPtr l);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern int GetWindowTextW(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern int GetClassNameW(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll")] public static extern IntPtr SendMessageW(IntPtr h, uint msg, IntPtr wp, IntPtr lp);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 public struct RECT{public int L,T,R,B;}
 public static IntPtr Dialog=IntPtr.Zero, IgnoreBtn=IntPtr.Zero, Game=IntPtr.Zero; public static RECT GameRect; public static string DlgText="";
 public static void Scan(uint pid){ Dialog=IntPtr.Zero; IgnoreBtn=IntPtr.Zero; Game=IntPtr.Zero;
   EnumWindows((h,l)=>{ uint o; GetWindowThreadProcessId(h,out o); if(o!=pid) return true;
     var cn=new StringBuilder(64); GetClassNameW(h,cn,64); var tt=new StringBuilder(256); GetWindowTextW(h,tt,256);
     if(cn.ToString()=="#32770"){ Dialog=h; DlgText=""; EnumChildWindows(h,(c,l2)=>{ var bs=new StringBuilder(512); GetWindowTextW(c,bs,512); var s=bs.ToString(); if(s.Contains("Пропустить")||s.Contains("Ignore")){ IgnoreBtn=c; } if(s.Contains("Failure")||s.Contains("Expression")||s.Contains("Assertion")) DlgText=s; return true;},IntPtr.Zero); }
     if(tt.ToString().Contains("San Andreas")){ Game=h; GetWindowRect(h,out GameRect); }
     return true; }, IntPtr.Zero);
 }
}
"@

$dir = Join-Path $PSScriptRoot 'bin\Debug'
Remove-Item (Join-Path $dir 'logs') -Recurse -Force -ErrorAction SilentlyContinue
$p = Start-Process -FilePath (Join-Path $dir 'reSA.exe') -WorkingDirectory $dir -PassThru
$ignores = 0
for ($i = 0; $i -lt ($Seconds * 4); $i++) {
    Start-Sleep -Milliseconds 250
    $p.Refresh(); if ($p.HasExited) { break }
    [Dlg]::Scan([uint32]$p.Id)
    if ([Dlg]::IgnoreBtn -ne [IntPtr]::Zero) {
        if ($ignores -lt 5) { Write-Host ("dialog: " + [Dlg]::DlgText.Replace("`n", " ")) }
        [Dlg]::SendMessageW([Dlg]::IgnoreBtn, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
        $ignores++
    }
}
$p.Refresh()
if ($p.HasExited) { Write-Host ("EXITED 0x{0:X8}" -f $p.ExitCode) }
else { Write-Host ("RUNNING; cpu={0:N2}s ignores={1}" -f $p.TotalProcessorTime.TotalSeconds, $ignores) }

if ($Shot -and -not $p.HasExited -and [Dlg]::Game -ne [IntPtr]::Zero) {
    $r = [Dlg]::GameRect; $w = $r.R - $r.L; $h = $r.B - $r.T
    if ($w -gt 0 -and $h -gt 0) {
        $bmp = New-Object System.Drawing.Bitmap $w, $h; $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.CopyFromScreen($r.L, $r.T, 0, 0, (New-Object System.Drawing.Size $w, $h))
        $bmp.Save((Join-Path $PSScriptRoot $Shot), [System.Drawing.Imaging.ImageFormat]::Png)
        Write-Host ("saved " + $Shot); $g.Dispose(); $bmp.Dispose()
    }
}
Write-Host "--- log tail ---"
if (Test-Path (Join-Path $dir 'logs\log.log')) { Get-Content (Join-Path $dir 'logs\log.log') | Select-Object -Last 10 }
if (-not $p.HasExited) { $p.Kill() }
