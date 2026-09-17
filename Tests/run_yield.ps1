<#
    XCOS yield round-trip bench runner  (Tests/run_yield.ps1)

    Sub-step of channel 3 (cycle baseline): runs the *yield* bench
    (Tests/bench/xc_yield_bench.c) through Tests/run_cycles.ps1 and compares its two
    metrics with the "cycles" section of Tests/baseline.json:

        yield_roundtrip : task that only yields        (scheduler "re-queue" path: self-loop)
        delay_roundtrip : task that blocks on Delay(1) (scheduler "re-queue" path: not self-loop)

    Why: that re-queue decision sits on the hottest path (every task call/yield) - these two
         numbers show whether a change made it cheaper or more expensive.

    Usage:
        powershell -File Tests/run_yield.ps1                  # compare with baseline
        powershell -File Tests/run_yield.ps1 -WriteBaseline   # merge these 2 keys only
        powershell -File Tests/run_yield.ps1 -Mdk <path> -TolPercent 5

    Ordering note: run this AFTER `run_cycles.ps1 -WriteBaseline` - that one rebuilds the
    whole "cycles" section (dropping these two keys); -WriteBaseline here merges them back.

    Exit code: 0 = within tolerance, 1 = drift / measurement failed, 2 = toolchain missing.

    NOTE: ASCII-only on purpose (PS 5.1 decodes BOM-less UTF-8 scripts as ANSI).
#>
[CmdletBinding()]
param(
    [string] $Mdk        = 'C:\Software\Keil\MDK5',
    [string] $Baseline   = '',
    [switch] $WriteBaseline,
    [int]    $TolPercent = 5,
    [int]    $WaitSec    = 25,
    [string] $OutDir     = ''
)

$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
if ([string]::IsNullOrEmpty($Baseline)) { $Baseline = Join-Path $testsDir 'baseline.json' }
$enc   = New-Object System.Text.UTF8Encoding($false)
$ps    = 'powershell'
$bench = Join-Path $testsDir 'bench\xc_yield_bench.c'
$ini   = Join-Path $testsDir 'bench\xc_yield_bench.ini'
$keys  = @('yield_roundtrip', 'delay_roundtrip')

if (-not (Test-Path $bench)) { Write-Host ('SKIP: yield bench not found: {0}' -f $bench) -ForegroundColor Yellow; exit 2 }
if (-not (Test-Path $ini))   { Write-Host ('SKIP: yield ini not found: {0}' -f $ini) -ForegroundColor Yellow; exit 2 }

Write-Host ''
Write-Host '################ channel 3b/3: yield round-trip bench (uVision simulator) ################'
$cycArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $testsDir 'run_cycles.ps1'),
             '-BenchFile', $bench, '-IniFile', $ini, '-ConfigTickSource', 'TestTick',
             '-Mdk', $Mdk, '-WaitSec', "$WaitSec")
if ($OutDir -ne '') { $cycArgs += @('-OutDir', $OutDir) }
$out = & $ps @cycArgs 2>&1
$rc  = $LASTEXITCODE
foreach ($ln in $out) { Write-Host $ln }
if ($rc -eq 2) { Write-Host ('SKIP: yield bench (toolchain missing: {0})' -f $Mdk) -ForegroundColor Yellow; exit 2 }

# ---------------------------------------------------------------- metrics
$cur = @{}
foreach ($k in $keys) {
    $hit = ($out | Select-String -Pattern ('^\s*{0}\s*=\s*(\d+)' -f $k) | Select-Object -First 1)
    if ($null -ne $hit) { $cur[$k] = [int]$hit.Matches[0].Groups[1].Value }
}
if ($cur.Count -eq 0) {
    Write-Host 'FAIL: no yield metrics parsed from sim log (session did not reach SimDone?)' -ForegroundColor Red
    exit 1
}

# ---------------------------------------------------------------- baseline merge
if ($WriteBaseline) {
    if (-not (Test-Path $Baseline)) { Write-Host ('FAIL: baseline not found: {0}' -f $Baseline) -ForegroundColor Red; exit 1 }
    $obj = Get-Content $Baseline -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($null -eq $obj.cycles) { Write-Host 'FAIL: baseline has no "cycles" section (run run_cycles.ps1 -WriteBaseline first)' -ForegroundColor Red; exit 1 }
    foreach ($k in $cur.Keys) { $obj.cycles | Add-Member -Force -MemberType NoteProperty -Name $k -Value $cur[$k] }
    [System.IO.File]::WriteAllText($Baseline, ($obj | ConvertTo-Json -Depth 8), $enc)
    Write-Host ''
    Write-Host ('baseline written (yield keys merged): {0}' -f $Baseline) -ForegroundColor Green
    exit 0
}

# ---------------------------------------------------------------- compare
if (-not (Test-Path $Baseline)) { Write-Host ('FAIL: baseline not found: {0} (-WriteBaseline first)' -f $Baseline) -ForegroundColor Red; exit 1 }
$base = Get-Content $Baseline -Raw -Encoding UTF8 | ConvertFrom-Json
if ($null -eq $base.cycles) { Write-Host 'FAIL: baseline has no "cycles" section' -ForegroundColor Red; exit 1 }
Write-Host ''
Write-Host ('==== yield bench vs baseline (tol +/-{0}%) ====' -f $TolPercent)
$drift = 0
foreach ($k in $keys) {
    if (-not $cur.ContainsKey($k)) { continue }
    $v   = $cur[$k]
    $ref = $base.cycles.$k
    if ($null -eq $ref) { Write-Host ('  ?    {0,-18} cur={1,-6} (no baseline)' -f $k, $v) -ForegroundColor Yellow; continue }
    $d   = $v - $ref
    $lim = [Math]::Max(1, [int][Math]::Round($ref * $TolPercent / 100.0))
    $ok  = ([Math]::Abs($d) -le $lim)
    if (-not $ok) { $drift++ }
    $verdict = if ($ok) { 'PASS' } else { 'FAIL' }
    $color   = if ($ok) { 'Green' } else { 'Red' }
    Write-Host ('  {0} {1,-18} cur={2,-6} base={3,-6} delta={4,5}  tol=+/-{5}' -f $verdict, $k, $v, $ref, $d, $lim) -ForegroundColor $color
}
Write-Host ''
if ($drift -gt 0) {
    Write-Host ('==== yield bench: FAIL ({0} metric(s) out of tolerance) ====' -f $drift) -ForegroundColor Red
    exit 1
}
Write-Host '==== yield bench: PASS (all metrics within tolerance) ====' -ForegroundColor Green
exit 0
