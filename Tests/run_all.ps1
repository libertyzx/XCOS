<#
    XCOS test entry point  (Tests/run_all.ps1)

    Runs all channels and prints one consolidated verdict:
        1) host functional regression   -> Tests/run_tests.ps1
        2) size / volume baseline       -> Tests/run_size.ps1
        3) cycle baseline (simulator)   -> Tests/run_cycles.ps1
        3b) yield round-trip bench      -> Tests/run_yield.ps1  (hottest-path cost)

    NOTE: ASCII-only on purpose (PS 5.1 decodes BOM-less UTF-8 scripts as ANSI).

    Usage:
        powershell -File Tests/run_all.ps1
        powershell -File Tests/run_all.ps1 -SkipSize -SkipCycles   # host regression only
        powershell -File Tests/run_all.ps1 -Mdk C:\Software\Keil\MDK5 -Gcc <path>
    Exit code: 0 = all channels pass, 1 = failure, 2 = toolchain missing.
#>
[CmdletBinding()]
param(
    [switch] $SkipSize,
    [switch] $SkipCycles,
    [string] $Gcc       = '',
    [string] $Mdk       = 'C:\Software\Keil\MDK5',
    [string] $ExtraPath = 'C:\Software\qalculate'
)

$ErrorActionPreference = 'Continue'
$testsDir = $PSScriptRoot
$ps = 'powershell'

Write-Host '################ channel 1/3: host functional regression ################'
$hostArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $testsDir 'run_tests.ps1'))
if ($Gcc -ne '')       { $hostArgs += @('-Gcc', $Gcc) }
if ($ExtraPath -ne '') { $hostArgs += @('-ExtraPath', $ExtraPath) }
& $ps @hostArgs
$rcHost = $LASTEXITCODE

$rcSize = 0
if ($SkipSize) {
    Write-Host ''
    Write-Host '################ channel 2/3: size baseline - SKIPPED (-SkipSize) ################'
}
else {
    Write-Host ''
    Write-Host '################ channel 2/3: size / volume baseline ################'
    $sizeArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $testsDir 'run_size.ps1'))
    if ($Mdk -ne '') { $sizeArgs += @('-Mdk', $Mdk) }
    & $ps @sizeArgs
    $rcSize = $LASTEXITCODE
}

$rcCyc = 0
$rcYield = 0
if ($SkipCycles) {
    Write-Host ''
    Write-Host '################ channel 3/3: cycle baseline - SKIPPED (-SkipCycles) ################'
}
else {
    Write-Host ''
    Write-Host '################ channel 3/3: cycle baseline (uVision simulator) ################'
    $cycArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $testsDir 'run_cycles.ps1'))
    if ($Mdk -ne '') { $cycArgs += @('-Mdk', $Mdk) }
    & $ps @cycArgs
    $rcCyc = $LASTEXITCODE

    Write-Host ''
    Write-Host '################ channel 3b/3: yield round-trip bench (hottest path) ################'
    $yieldArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $testsDir 'run_yield.ps1'))
    if ($Mdk -ne '') { $yieldArgs += @('-Mdk', $Mdk) }
    & $ps @yieldArgs
    $rcYield = $LASTEXITCODE
}

Write-Host ''
Write-Host '################ summary ################'
Write-Host ('  host regression : {0}' -f $(if ($rcHost -eq 0) { 'PASS' } else { ('FAIL (rc={0})' -f $rcHost) }))
Write-Host ('  size baseline   : {0}' -f $(if ($SkipSize) { 'SKIPPED' } elseif ($rcSize -eq 2) { 'SKIPPED (no MDK)' } elseif ($rcSize -eq 0) { 'PASS' } else { ('FAIL (rc={0})' -f $rcSize) }))
Write-Host ('  cycle baseline  : {0}' -f $(if ($SkipCycles) { 'SKIPPED' } elseif ($rcCyc -eq 2) { 'SKIPPED (no MDK)' } elseif ($rcCyc -eq 0) { 'PASS' } else { ('FAIL (rc={0})' -f $rcCyc) }))
Write-Host ('  yield bench     : {0}' -f $(if ($SkipCycles) { 'SKIPPED' } elseif ($rcYield -eq 2) { 'SKIPPED (no MDK)' } elseif ($rcYield -eq 0) { 'PASS' } else { ('FAIL (rc={0})' -f $rcYield) }))
$okSize = ($rcSize -eq 0 -or $rcSize -eq 2)
$okCyc  = ($rcCyc -eq 0 -or $rcCyc -eq 2)
$okYield = ($rcYield -eq 0 -or $rcYield -eq 2)
if ($rcHost -eq 0 -and $okSize -and $okCyc -and $okYield) { Write-Host '  => ALL PASS'; exit 0 }
Write-Host '  => FAIL'; exit 1
