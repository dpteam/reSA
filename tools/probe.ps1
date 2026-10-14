<#
.SYNOPSIS
    Unattended engine probe: boots reSA, drives New Game without a human, and reports how far it got.

.DESCRIPTION
    This is the harness half of the `--autostart` / `--quit-after` / `--dump-progress` engine flags
    (see source/extensions/CommandLine.h for why they exist). It exists so the load path can be
    iterated autonomously: no click, no screenshot, no guessing - just a verdict.

    What it reports, and why each part is there:
      * exit code, decoded            - distinguishes a clean quit from NOTSA_UNREACHABLE (4) or an AV.
      * hang vs death                 - a run that never exits is a different bug class from a crash,
                                        and the boot path has at least one real infinite loop in it
                                        (CAudioEngine::InitialisePostLoading waits forever for sound
                                        banks when there is no audio device), so this is measured,
                                        not assumed.
      * load marker count + last one  - the project's yardstick (VERIFY.md section G).
      * the [probe] summary block     - the engine's own counters, which survive a truncated log.
      * crash stack + CRT reports     - logs/log.log holds the symbolised stack; logs/runtime_reports.txt
                                        holds asserts, each with a stack captured in the CRT hook.

    Notes on implementation, all learned the hard way:
      * Logs are read with an explicit ABSOLUTE path and FileShare::ReadWrite. The game holds them open
        with a deny-none share, and .NET's CWD is NOT PowerShell's location, so a relative path silently
        resolves against the wrong directory.
      * The process is killed before starting, because the exe file is locked while running.

.PARAMETER Frames
    How many frames to allow before the engine quits itself. The synchronous load costs only a handful,
    so 240 is plenty to reach the world; raise it to play longer.

.PARAMETER TimeoutSec
    Wall-clock seconds to wait before declaring a hang.

.EXAMPLE
    pwsh -File tools\probe.ps1
    pwsh -File tools\probe.ps1 -Frames 600 -TimeoutSec 180
    pwsh -File tools\probe.ps1 -ExtraArgs @('--idle-frames=10')
#>
[CmdletBinding()]
param(
    [int]   $Frames    = 240,
    [int]   $TimeoutSec = 120,
    [string[]] $ExtraArgs = @(),
    [switch] $NoAutostart
)

$ErrorActionPreference = 'Stop'

$root   = Split-Path -Parent $PSScriptRoot
$binDir = Join-Path $root 'bin\Debug'
$exe    = Join-Path $binDir 'reSA.exe'
$logDir = Join-Path $binDir 'logs'

if (-not (Test-Path $exe)) { throw "reSA.exe not found at $exe - build it first" }

function Read-SharedLogFile {
    param([string] $Path)
    if (-not (Test-Path $Path)) { return @() }
    $fs = [System.IO.File]::Open($Path, 'Open', 'Read', [System.IO.FileShare]::ReadWrite)
    try {
        $sr = New-Object System.IO.StreamReader($fs)
        try { return @($sr.ReadToEnd() -split "`r?`n") } finally { $sr.Dispose() }
    } finally { $fs.Dispose() }
}

function Format-ExitCode {
    param([int] $Code)
    switch ($Code) {
        0           { 'clean exit' }
        4           { 'NOTSA_UNREACHABLE (a [[noreturn]] path was taken)' }
        -1073741819 { '0xC0000005 ACCESS VIOLATION' }
        -1073741795 { '0xC000001D ILLEGAL INSTRUCTION' }
        -1073741571 { '0xC00000FD STACK OVERFLOW' }
        -1073740791 { '0xC000041D STACK BUFFER OVERRUN / unhandled C++ exception' }
        -1073741515 { '0xC0000135 missing DLL' }
        default     { "exit code $Code (0x{0:X8})" -f $Code }
    }
}

# --- start ------------------------------------------------------------------
Get-Process reSA -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

if (Test-Path (Join-Path $logDir 'runtime_reports.txt')) {
    Remove-Item -Force (Join-Path $logDir 'runtime_reports.txt')
}
if (Test-Path (Join-Path $logDir 'log.log')) {
    Move-Item -Force (Join-Path $logDir 'log.log') (Join-Path $logDir 'log.prev.txt')
}

$args = @('--quit-after=' + $Frames, '--dump-progress')
if (-not $NoAutostart) { $args += '--autostart' }
$args += $ExtraArgs

"probe: reSA.exe $($args -join ' ')"
$proc = Start-Process -FilePath $exe -ArgumentList $args -WorkingDirectory $binDir -PassThru

# --- wait, distinguishing death from hang -----------------------------------
if (-not $proc.WaitForExit($TimeoutSec * 1000)) {
    "verdict: HUNG - no exit within $($TimeoutSec)s. Killing."
    try { $proc.Kill() } catch {}
    $exitCode = $null
} else {
    $exitCode = $proc.ExitCode
    "verdict: exited with {0} = {1}" -f $exitCode, (Format-ExitCode $exitCode)
}

# --- report -----------------------------------------------------------------
Start-Sleep -Milliseconds 400
$log     = Read-SharedLogFile (Join-Path $logDir 'log.log')
$reports = Read-SharedLogFile (Join-Path $logDir 'runtime_reports.txt')

# Count progress only up to the crash section, so a re-emitted log tail (spdlog's dump_backtrace) can
# never inflate the yardstick. Everything after the sentinel is post-mortem output, not progress.
$cut = 0..($log.Count - 1) | Where-Object { $log[$_] -match 'UNHANDLED EXCEPTION|runtime report' } |
       Select-Object -First 1
if ($null -eq $cut) { $cut = $log.Count }
$live = $log[0..([Math]::Max(0, $cut - 1))]

$fileMarkers = @($live | Select-String 'Loading the Game -> Loading')
""
"--- progress (pre-crash lines only: $cut of $($log.Count)) ---"
"load file markers : $($fileMarkers.Count)"
if ($fileMarkers.Count) {
    "last file marker  : " + (($fileMarkers | Select-Object -Last 1).Line -replace '^.*\]: ', '')
}
$phase = @($live | Select-String 'Loadingscreen:')
if ($phase.Count) {
    "last phase        : " + (($phase | Select-Object -Last 1).Line -replace '^.*\]: ', '')
}
"fps lines         : " + (@($live | Select-String 'FPS:').Count)

$probeLines = @($live | Select-String '\[probe\]')
if ($probeLines.Count) {
    ""
    "--- engine probe summary ---"
    $probeLines | ForEach-Object { '   ' + ($_.Line -replace '^.*\]: ', '') }
}

$exc = @($log | Select-String 'Exception Address')
if ($exc.Count) {
    ""
    "--- exception ---"
    $exc | ForEach-Object { '   ' + ($_.Line -replace '^.*\]: ', '') }
    $csLine = ($log | Select-String 'CALL STACK').LineNumber | Select-Object -Last 1
    if ($csLine) {
        $log | Select-Object -Skip ($csLine - 1) -First 14 | ForEach-Object {
            $t = $_ -replace '^.*\]: ', ''
            if ($t -match 'END OF UNHANDLED') { return }
            '   ' + $t
        }
    }
}

if ($reports.Count) {
    ""
    "--- CRT reports (asserts / warnings, with stacks) ---"
    $reports | Where-Object { $_.Trim() } | Select-Object -First 24 | ForEach-Object { '   ' + $_ }
}

$unimpl = @($log | Select-String 'unimplemented opcode')
if ($unimpl.Count) {
    ""
    "--- unimplemented script opcodes (distinct, first 20) ---"
    $unimpl | Select-Object -First 20 | ForEach-Object { '   ' + ($_.Line -replace '^.*\]: ', '') }
    "   total hits: $($unimpl.Count)"
}
""
