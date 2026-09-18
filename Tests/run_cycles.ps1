<#
    XCOS cycle (performance) runner  (Tests/run_cycles.ps1)

    Channel 4 of the test harness: key-path **cycle counts** measured in the
    uVision simulator (Cortex-M3), following Docs/Skill/MDK_Sim_Agent_Skill.md:

        mirror workspace  ->  replace main.c with the bench
        ->  point .uvoptx at our .ini (sSim/uSim/sGomain/sIfile)
        ->  build (uVision.com -b, clean Debug/ first)
        ->  run (UV4 -d -j0) with timeout + kill
        ->  parse sim_log (r<idx> <name> = <cycles>)
        ->  compare with Tests/baseline.json (cycles section), tolerance -TolPercent

    Usage:
        powershell -File Tests/run_cycles.ps1                  # compare with baseline
        powershell -File Tests/run_cycles.ps1 -WriteBaseline   # (re)create baseline
        powershell -File Tests/run_cycles.ps1 -TolPercent 5 -WaitSec 25 -Mdk <path>

        # run an ALTERNATIVE sim case (same mirror/build/run/parse flow; results are
        # printed, unknown metric names are shown as "?" and do NOT fail the channel):
        powershell -File Tests/run_cycles.ps1 `
            -BenchFile "<path to bench .c, replaces main.c>" `
            -IniFile   "<path to .ini template (placeholder __SIM_LOG__)>" `
            -ProjectDefine "XC_CFG_TICKS_PER_SEC=100000U"     # optional, mirror-only
    Exit code: 0 = within tolerance, 1 = drift / measurement failed, 2 = toolchain missing.

    NOTE: ASCII-only on purpose (PS 5.1 decodes BOM-less UTF-8 scripts as ANSI).
#>
[CmdletBinding()]
param(
    [string] $Mdk         = 'C:\Software\Keil\MDK5',
    [string] $Baseline    = '',
    [switch] $WriteBaseline,
    [int]    $TolPercent  = 5,
    [int]    $WaitSec     = 25,
    [string] $OutDir      = '',
    [string] $BenchFile     = '',
    [string] $IniFile       = '',
    [string] $ProjectDefine = '',
    [string] $ConfigTickSource = ''
)

$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root     = Split-Path -Parent $testsDir
if ([string]::IsNullOrEmpty($Baseline)) { $Baseline = Join-Path $testsDir 'baseline.json' }
if ([string]::IsNullOrEmpty($OutDir))   { $OutDir   = Join-Path $env:TEMP ('xcos_cycles_' + (Get-Date -Format 'yyyyMMdd_HHmmss')) }
[void][System.IO.Directory]::CreateDirectory($OutDir)

$uv4   = Join-Path $Mdk 'UV4\UV4.exe'
$uvcom = Join-Path $Mdk 'UV4\uVision.com'
if (-not (Test-Path $uv4)) {
    Write-Host ('SKIP: UV4 not found at {0} (use -Mdk <path>)' -f $uv4) -ForegroundColor Yellow
    exit 2
}

function Run-Exe([string] $file, [string[]] $argList, [string] $stdout, [string] $stderr) {
    $p = Start-Process -FilePath $file -ArgumentList $argList -NoNewWindow -Wait -PassThru `
                       -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    return $p.ExitCode
}

# ---------------------------------------------------------------- mirror workspace
$mirror = Join-Path $OutDir 'mirror'
[void][System.IO.Directory]::CreateDirectory($mirror)
Copy-Item (Join-Path $root 'Code') (Join-Path $mirror 'Code') -Recurse -Force
[void][System.IO.Directory]::CreateDirectory((Join-Path $mirror 'Examples'))
Copy-Item (Join-Path $root 'Examples\CortexM3_Test') (Join-Path $mirror 'Examples\CortexM3_Test') -Recurse -Force

$proj    = Join-Path $mirror 'Examples\CortexM3_Test\CortexM3_Test.uvprojx'
$optx    = Join-Path $mirror 'Examples\CortexM3_Test\CortexM3_Test.uvoptx'
$mainC   = Join-Path $mirror 'Examples\CortexM3_Test\Code\Main\main.c'
$iniPath = Join-Path $OutDir 'cycles.ini'
$simLog  = Join-Path $OutDir 'sim_log.txt'
$buildLog= Join-Path $OutDir 'build.log'
$consLog = Join-Path $OutDir 'console.txt'
$enc     = New-Object System.Text.UTF8Encoding($false)

# bench replaces the example's main.c (no .uvprojx edit needed)
# (default = the key-path cycles bench; -BenchFile selects an alternative sim case)
$defaultBench = Join-Path $testsDir 'bench\xc_cycles_bench.c'
$defaultIni   = Join-Path $testsDir 'bench\cycles.ini'
if ([string]::IsNullOrEmpty($BenchFile)) { $BenchFile = $defaultBench }
if ([string]::IsNullOrEmpty($IniFile))   { $IniFile   = $defaultIni }
if (-not (Test-Path $BenchFile)) { Write-Host ('FAIL: bench not found: {0}' -f $BenchFile) -ForegroundColor Red; exit 1 }
if (-not (Test-Path $IniFile))   { Write-Host ('FAIL: ini not found: {0}' -f $IniFile) -ForegroundColor Red; exit 1 }
if ($WriteBaseline -and ($BenchFile -ne $defaultBench)) {
    Write-Host 'FAIL: -WriteBaseline is only allowed with the default cycles bench (use -WriteBaseline on the normal channel)' -ForegroundColor Red
    exit 1
}
Copy-Item $BenchFile $mainC -Force

# optional: append a target define to the MIRROR project (e.g. XC_CFG_TICKS_PER_SEC=100000U)
if ($ProjectDefine -ne '') {
    $p = [System.IO.File]::ReadAllText($proj, $enc)
    $p = [regex]::Replace($p, '<Define>([^<]*)</Define>', ('<Define>$1,' + $ProjectDefine + '</Define>'), 1)
    [System.IO.File]::WriteAllText($proj, $p, $enc)
    Write-Host ('  define  : +{0} (mirror project only)' -f $ProjectDefine)
}

# optional: point the tick source at a sim-bench function (MIRROR XC_Config.h only).
# NOTE: MDK's <Define> field drops "()" => the hook must be written into the header itself.
# NOTE: the hook uses plain uint32_t (== XC_Tick_t) on purpose -- XC_Config.h no longer defines
#       XC_Tick_t (since V2.1.1 it lives in Internal/XC_TypeInternal.h, which XC_Config.h cannot include).
if ($ConfigTickSource -ne '') {
    $cfg  = Join-Path $mirror 'Code\Inc\XC_Config.h'
    $c    = [System.IO.File]::ReadAllText($cfg, $enc)
    $hook = ('uint32_t ' + $ConfigTickSource + '(void); /* [sim bench] scripted tick source */' + "`r`n" +
             '#define XC_SYS_TICK_INT_INC_MODE' + "`r`n" +
             'extern volatile uint32_t g_SysTickCount;' + "`r`n" +
             '#define XC_SYS_TICK_COUNT ' + $ConfigTickSource + '()' + "`r`n")
    $c = [regex]::Replace($c, '(?m)^#ifndef XC_SYS_TICK_COUNT', ($hook + '#ifndef XC_SYS_TICK_COUNT'), 1)
    [System.IO.File]::WriteAllText($cfg, $c, $enc)
    Write-Host ('  tick src: XC_SYS_TICK_COUNT -> {0}() (mirror XC_Config.h)' -f $ConfigTickSource)
}

# .uvoptx: allow the script to own the session (sGomain=0) and point at our .ini
$o = [System.IO.File]::ReadAllText($optx, $enc)
$o = $o.Replace('<sGomain>1</sGomain>', '<sGomain>0</sGomain>')
$o = [regex]::Replace($o, '<sIfile>.*?</sIfile>', ('<sIfile>' + $iniPath + '</sIfile>'), 1)
[System.IO.File]::WriteAllText($optx, $o, $enc)

# .ini: substitute the log path placeholder
$ini = [System.IO.File]::ReadAllText($IniFile, $enc)
[System.IO.File]::WriteAllText($iniPath, $ini.Replace('__SIM_LOG__', $simLog), $enc)

Write-Host ('XCOS cycle channel: mirror = {0}' -f $mirror)
Write-Host ('  ini     : {0}' -f $iniPath)
Write-Host ('  sim log : {0}' -f $simLog)
Write-Host ''

# ---------------------------------------------------------------- build
# (clean Debug/ first: incremental builds would be skipped -> stale image)
$dbgDir = Join-Path $mirror 'Examples\CortexM3_Test\Debug'
if (Test-Path $dbgDir) { Remove-Item $dbgDir -Recurse -Force }
$null = Run-Exe $uvcom @('-b', $proj, '-o', $buildLog) $consLog ($consLog + '.err')
$txt = Get-Content $buildLog -Encoding UTF8 -ErrorAction SilentlyContinue
$sizeLine = $txt | Select-String -Pattern 'Program Size' | Select-Object -Last 1
$errLine  = $txt | Select-String -Pattern '\d+ Error\(s\), \d+ Warning\(s\)' | Select-Object -Last 1
if ($sizeLine) { Write-Host ('  build   : {0}' -f $sizeLine.Line.Trim()) }
if ($errLine)  { Write-Host ('            {0}' -f $errLine.Line.Trim()) }

# ---------------------------------------------------------------- run simulator (timeout + kill)
# the sim session occasionally fails to start (Skill trap #10: stale UV4 / simulator state)
# => clear the process(es) and retry once
function Invoke-Sim([int] $seconds) {
    if (Test-Path $simLog) { Remove-Item $simLog -Force }
    $proc = Start-Process -FilePath $uv4 -ArgumentList @('-d', $proj, '-j0') -PassThru
    $n = 0
    while ($n -lt $seconds) {
        Start-Sleep -Seconds 1
        $n++
        if (Test-Path $simLog) {
            # kill as soon as END shows up in the log; no need to wait for the full -WaitSec
            $done = Select-String -Path $simLog -Pattern '===== END =====' -Encoding UTF8 -ErrorAction SilentlyContinue
            if ($done) { break }
        }
        if ($proc.HasExited) { break }
    }
    if (-not $proc.HasExited) { try { $proc.Kill() } catch { } }
    Get-Process UV4 -ErrorAction SilentlyContinue | Stop-Process -Force
    return $n
}

# the sim session occasionally fails to start or finishes without producing output
# (Skill trap #10: stale UV4 / simulator state) => clear the process(es) and retry up to 3 times
$metrics  = [ordered]@{}
for ($attempt = 1; $attempt -le 3 -and $metrics.Count -eq 0; $attempt++) {
    Get-Process UV4 -ErrorAction SilentlyContinue | Stop-Process -Force
    if ($attempt -gt 1) {
        Write-Host ('  sim     : retry #{0}/3 (previous session produced no results)' -f $attempt) -ForegroundColor Yellow
        Start-Sleep -Seconds 2
    }
    $waited = Invoke-Sim $WaitSec
    Write-Host ('  sim     : waited {0}s; log exists = {1}' -f $waited, (Test-Path $simLog))

    # ------------------------------------------------------------ parse results
    # NOTE: stopping UV4 does not always release the sim log immediately (the process that writes it
    # can outlive Stop-Process) => a plain ReadAllLines fails with "file is in use" and would fail the
    # whole channel. Open with FileShare.ReadWrite instead, and retry a few times.
    $simLines = @()
    if (Test-Path $simLog) {
    for ($try = 1; $try -le 5; $try++) {
        try {
            $fs = [System.IO.File]::Open($simLog, [System.IO.FileMode]::Open,
                                         [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
            $sr = New-Object System.IO.StreamReader($fs, $enc)
            $simLines = @($sr.ReadToEnd() -split "`r?`n")
            $sr.Close(); $fs.Close()
            break
        }
        catch {
            if ($try -eq 5) {
                Write-Host ('  sim log : still locked after {0} tries ({1})' -f $try, $_.Exception.Message) -ForegroundColor Yellow
            }
            Start-Sleep -Milliseconds 400
        }
    }
    foreach ($ln in $simLines) {
        if ($ln -match '^\s*r\d+\s+(\S+)\s*=\s*(\d+)') { $metrics[$Matches[1]] = [int]$Matches[2] }
    }
    }
}
Write-Host ''
if ($metrics.Count -eq 0) {
    Write-Host 'FAIL: no cycle results parsed from sim log after 3 attempts (session did not reach SimDone?)' -ForegroundColor Red
    Write-Host ('  see: {0}' -f $simLog)
    exit 1
}
Write-Host '==== cycle measurements ===='
foreach ($k in $metrics.Keys) { Write-Host ('  {0,-22} = {1}' -f $k, $metrics[$k]) }

# ---------------------------------------------------------------- baseline
if ($WriteBaseline) {
    $cycles = [ordered]@{}
    foreach ($k in $metrics.Keys) { $cycles[$k] = $metrics[$k] }
    $obj = $null
    if (Test-Path $Baseline) { $obj = Get-Content $Baseline -Raw -Encoding UTF8 | ConvertFrom-Json }
    if ($null -eq $obj) { $obj = New-Object psobject }
    $obj | Add-Member -Force -MemberType NoteProperty -Name cycles -Value ([pscustomobject]$cycles)
    [System.IO.File]::WriteAllText($Baseline, ($obj | ConvertTo-Json -Depth 8), $enc)
    Write-Host ''
    Write-Host ('baseline written (cycles): {0}' -f $Baseline) -ForegroundColor Green
    exit 0
}
if (-not (Test-Path $Baseline)) { Write-Host 'FAIL: baseline.json not found (-WriteBaseline first)' -ForegroundColor Red; exit 1 }
$base = Get-Content $Baseline -Raw -Encoding UTF8 | ConvertFrom-Json
if ($null -eq $base.cycles) { Write-Host 'FAIL: baseline has no "cycles" section (run -WriteBaseline)' -ForegroundColor Red; exit 1 }

# ---------------------------------------------------------------- compare
Write-Host ''
Write-Host ('==== cycles vs baseline (tol +/-{0}%) ====' -f $TolPercent)
$drift = 0
foreach ($k in $metrics.Keys) {
    $cur = $metrics[$k]
    $ref = $base.cycles.$k
    if ($null -eq $ref) { Write-Host ('  ?    {0,-22} cur={1,-6} (no baseline)' -f $k, $cur) -ForegroundColor Yellow; continue }
    $d   = $cur - $ref
    $lim = [Math]::Max(1, [int][Math]::Round($ref * $TolPercent / 100.0))
    $ok  = ([Math]::Abs($d) -le $lim)
    if (-not $ok) { $drift++ }
    $verdict = if ($ok) { 'PASS' } else { 'FAIL' }
    $color   = if ($ok) { 'Green' } else { 'Red' }
    Write-Host ('  {0} {1,-22} cur={2,-6} base={3,-6} delta={4,5}  tol=+/-{5}' -f $verdict, $k, $cur, $ref, $d, $lim) -ForegroundColor $color
}
Write-Host ''
if ($drift -gt 0) {
    Write-Host ('==== cycles: FAIL ({0} metric(s) out of tolerance) ====' -f $drift) -ForegroundColor Red
    exit 1
}
Write-Host '==== cycles: PASS (all metrics within tolerance) ====' -ForegroundColor Green
exit 0
