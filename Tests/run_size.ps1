<#
    XCOS size (volume) runner  (Tests/run_size.ps1)

    Two channels:
      * object-level : armcc --c99 -O1 --cpu=Cortex-M3 for each kernel .c,
                       sizes read with fromelf -z, per switch configuration.
      * image-level  : mirrors Code/ + Examples/CortexM3_Test/ into a temp dir,
                       builds with UV4 -b, parses "Program Size:" of the .map/axf log.
                       Two configurations:  default (no switches)  and  stats
                       (XC_CFG_TASK_STATS=1 + a stats query injected into main.c).

    Baseline is Tests/baseline.json (generate/refresh with -WriteBaseline).

    NOTE: ASCII-only on purpose (PS 5.1 decodes BOM-less UTF-8 scripts as ANSI).

    Usage:
        powershell -File Tests/run_size.ps1 -WriteBaseline      # (re)create baseline
        powershell -File Tests/run_size.ps1                     # compare with baseline
        powershell -File Tests/run_size.ps1 -SkipImage          # object level only
        powershell -File Tests/run_size.ps1 -Mdk C:\Software\Keil\MDK5
    Exit code: 0 = within tolerance, 1 = drift detected, 2 = toolchain missing.
#>
[CmdletBinding()]
param(
    [string] $Mdk        = 'C:\Software\Keil\MDK5',
    [string] $Baseline   = '',
    [switch] $WriteBaseline,
    [int]    $TolObject  = 0,
    [int]    $TolImage   = 8,
    [switch] $SkipImage,
    [string] $OutDir     = ''
)

$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root     = Split-Path -Parent $testsDir
if ([string]::IsNullOrEmpty($Baseline)) { $Baseline = Join-Path $testsDir 'baseline.json' }
if ([string]::IsNullOrEmpty($OutDir))   { $OutDir   = Join-Path $env:TEMP ('xcos_size_' + (Get-Date -Format 'yyyyMMdd_HHmmss')) }
[void][System.IO.Directory]::CreateDirectory($OutDir)

$armcc   = Join-Path $Mdk 'ARM\ARMCC\bin\armcc.exe'
$fromelf = Join-Path $Mdk 'ARM\ARMCC\bin\fromelf.exe'
$uv4     = Join-Path $Mdk 'UV4\UV4.exe'
$incDir  = Join-Path $root 'Code\Inc'

function Run-Exe([string] $file, [string[]] $argList, [string] $stdout, [string] $stderr) {
    $p = Start-Process -FilePath $file -ArgumentList $argList -NoNewWindow -Wait -PassThru `
                       -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    return $p.ExitCode
}

function Get-ObjectSizes([string] $obj, [string] $tag) {
    $txt = Join-Path $OutDir ($tag + '.sizes.txt')
    [void](Run-Exe $fromelf @('--text', '-z', '-o', $txt, $obj) (Join-Path $OutDir ($tag + '.fe.out')) (Join-Path $OutDir ($tag + '.fe.err')))
    $line = Get-Content $txt -Encoding UTF8 -ErrorAction SilentlyContinue |
            Where-Object { $_ -match '^\s+\d+\s+\d+\s+\d+\s+\d+\s+\d+\s+\S' } | Select-Object -Last 1
    if (-not $line) { return $null }
    $n = ($line.Trim() -split '\s+')
    return [pscustomobject]@{ Code = [int]$n[0]; RO = [int]$n[1]; RW = [int]$n[2]; ZI = [int]$n[3]; Debug = [int]$n[4] }
}

# ---------------------------------------------------------------- object level
$srcFiles = @('XC_Diag.c', 'XC_List.c', 'XC_Sch.c', 'XC_Task.c', 'XC_Time.c')
$configs  = [ordered]@{
    'off'  = @()
    'diag' = @('-DXC_CFG_ERR_HOOK=1', '-DXC_CFG_DEBUG_CHECK=1', '-DXC_CFG_TASK_STATS=1', '-DXC_CFG_ASSERT=1')
}
$objRes = [ordered]@{}
if (-not (Test-Path $armcc)) {
    Write-Host ('SKIP: armcc not found at {0} (use -Mdk <path>)' -f $armcc) -ForegroundColor Yellow
} else {
    foreach ($cfg in $configs.Keys) {
        $objRes[$cfg] = [ordered]@{}
        foreach ($f in $srcFiles) {
            $obj = Join-Path $OutDir ($cfg + '_' + $f + '.o')
            $log = Join-Path $OutDir ($cfg + '_' + $f + '.cc.log')
            $args = @('--c99', '-O1', '--cpu=Cortex-M3', '-I', $incDir) + $configs[$cfg] + @('-c', (Join-Path $root ('Code\Src\' + $f)), '-o', $obj)
            $rc = Run-Exe $armcc $args $log ($log + '.err')
            if ($rc -ne 0) { Write-Host ('FAIL: armcc {0} {1} rc={2}' -f $cfg, $f, $rc) -ForegroundColor Red; exit 1 }
            $s = Get-ObjectSizes $obj ($cfg + '_' + $f)
            if ($null -eq $s) { Write-Host ('FAIL: could not read sizes of {0}' -f $obj) -ForegroundColor Red; exit 1 }
            $objRes[$cfg][$f] = $s
            Write-Host ('  object {0,-5} {1,-12} Code={2,-5} RO={3,-4} RW={4} ZI={5}' -f $cfg, $f, $s.Code, $s.RO, $s.RW, $s.ZI)
        }
    }
}

# ---------------------------------------------------------------- image level
function Build-Image([string] $mirror, [string] $tag, [string[]] $defines, [switch] $TouchDiag) {
    $proj = Join-Path $mirror 'Examples\CortexM3_Test\CortexM3_Test.uvprojx'
    $enc  = New-Object System.Text.UTF8Encoding($false)

    if ($defines.Count -gt 0) {
        $u = [System.IO.File]::ReadAllText($proj, $enc)
        # idempotent: replace the <Define> element that carries USE_HAL_DRIVER (avoids re-appending / mismatch)
        $newDef = '<Define>USE_HAL_DRIVER,STM32F103xE,' + ($defines -join ',') + '</Define>'
        $u = [regex]::Replace($u, '<Define>[^<]*USE_HAL_DRIVER[^<]*</Define>', $newDef, 1)
        [System.IO.File]::WriteAllText($proj, $u, $enc)
    }
    # clean the output folder: otherwise the incremental build is skipped (Skill trap #2)
    # => "config changed but nothing got rebuilt / no Program Size"
    $dbgDir = Join-Path $mirror 'Examples\CortexM3_Test\Debug'
    if (Test-Path $dbgDir) { Remove-Item $dbgDir -Recurse -Force }
    if ($TouchDiag) {
        # [diag build only, mirror copy] (1) reference the diag APIs (else the linker drops unused diag code);
        #                              (2) provide an empty XC_Err_Hook (required once ERR_HOOK is on)
        $main = Join-Path $mirror 'Examples\CortexM3_Test\Code\Main\main.c'
        $m = [System.IO.File]::ReadAllText($main, $enc)
        $touch = "XC_Sch_Init(&s_hXCOS0);" + "`r`n" +
                 "#if(XC_CFG_DEBUG_CHECK != 0)" + "`r`n" +
                 "    (void)XC_Diag_CheckInvariants(&s_hXCOS0);" + "`r`n" +
                 "#endif" + "`r`n" +
                 "#if(XC_CFG_TASK_STATS != 0)" + "`r`n" +
                 "    (void)XC_Diag_GetRunCnt(&s_hTCBn[0]);" + "`r`n" +
                 "    (void)XC_Diag_GetMaxRunTick(&s_hTCBn[0]);" + "`r`n" +
                 "#endif"
        $m = $m.Replace('XC_Sch_Init(&s_hXCOS0);', $touch)
        $m = $m + "`r`n/* [run_size] diag build: empty hook (mirror copy only, repo untouched) */`r`n" +
                  "#include `"XCOS.h`"`r`n" +
                  "void XC_Err_Hook(XC_TaskHandle_t phTCB, XC_ErrCode_t ErrCode) { (void)phTCB; (void)ErrCode; }`r`n"
        [System.IO.File]::WriteAllText($main, $m, $enc)
    }

    $log = Join-Path $OutDir ('image_' + $tag + '.log')
    $rc  = Run-Exe $uv4 @('-b', $proj, '-j0', '-o', $log) (Join-Path $OutDir ('image_' + $tag + '.out')) (Join-Path $OutDir ('image_' + $tag + '.err'))
    $txt = Get-Content $log -Encoding UTF8 -ErrorAction SilentlyContinue
    $sizeLine = $txt | Select-String -Pattern 'Program Size:\s*Code=(\d+)\s*RO-data=(\d+)\s*RW-data=(\d+)\s*ZI-data=(\d+)' | Select-Object -Last 1
    $errLine  = $txt | Select-String -Pattern '(\d+) Error\(s\), (\d+) Warning\(s\)' | Select-Object -Last 1
    if (-not $sizeLine) {
        Write-Host ('FAIL: image build {0} produced no "Program Size" (UV4 rc={1}); see {2}' -f $tag, $rc, $log) -ForegroundColor Red
        return $null
    }
    $m = $sizeLine.Matches[0]
    $r = [pscustomobject]@{ Code=[int]$m.Groups[1].Value; RO=[int]$m.Groups[2].Value; RW=[int]$m.Groups[3].Value; ZI=[int]$m.Groups[4].Value }
    $warn = ''
    if ($errLine) { $warn = (' | {0}' -f $errLine.Line.Trim()) }
    Write-Host ('  image  {0,-8} Code={1,-5} RO={2,-4} RW={3} ZI={4}{5}' -f $tag, $r.Code, $r.RO, $r.RW, $r.ZI, $warn)
    return $r
}

$imgRes = [ordered]@{}
if ($SkipImage) {
    Write-Host 'SKIP: image channel disabled (-SkipImage)' -ForegroundColor Yellow
} elseif (-not (Test-Path $uv4)) {
    Write-Host ('SKIP: UV4 not found at {0} (use -Mdk <path>)' -f $uv4) -ForegroundColor Yellow
} else {
    $mirror = Join-Path $OutDir 'mirror'
    [void][System.IO.Directory]::CreateDirectory($mirror)
    Copy-Item (Join-Path $root 'Code') (Join-Path $mirror 'Code') -Recurse -Force
    [void][System.IO.Directory]::CreateDirectory((Join-Path $mirror 'Examples'))
    Copy-Item (Join-Path $root 'Examples\CortexM3_Test') (Join-Path $mirror 'Examples\CortexM3_Test') -Recurse -Force
    foreach ($sub in @('Debug', 'Objects', 'Listings')) {
        $p = Join-Path $mirror ('Examples\CortexM3_Test\' + $sub)
        if (Test-Path $p) { Remove-Item $p -Recurse -Force }
    }
    $imgRes['default'] = Build-Image $mirror 'default' @()
    # stats build needs the switch only: the scheduler main loop (XC_Sch_Start) references the stats code
    # => no example main.c edit required
    $imgRes['stats']   = Build-Image $mirror 'stats' @('XC_CFG_TASK_STATS=1')
    # full-diag build: all four switches on + diag API references + empty hook (injected into the mirror copy)
    $imgRes['diag']    = Build-Image $mirror 'diag' @('XC_CFG_ERR_HOOK=1', 'XC_CFG_DEBUG_CHECK=1', 'XC_CFG_TASK_STATS=1', 'XC_CFG_ASSERT=1') -TouchDiag
}

# ---------------------------------------------------------------- baseline
$hasBase = Test-Path $Baseline
if ($WriteBaseline) {
    # Merge write: update only this channel's sections (object/image) and KEEP the rest
    # (e.g. "cycles", written by run_cycles.ps1). The old version overwrote the whole
    # file, which deleted the cycles section and broke the cycle channel.
    $data = [ordered]@{}
    if ($hasBase) {
        $old = Get-Content $Baseline -Raw -Encoding UTF8 | ConvertFrom-Json
        foreach ($p in $old.PSObject.Properties) { $data[$p.Name] = $p.Value }
    }
    $data['object'] = $objRes
    $data['image']  = $imgRes
    [System.IO.File]::WriteAllText($Baseline, ($data | ConvertTo-Json -Depth 8), (New-Object System.Text.UTF8Encoding($false)))
    Write-Host ''
    Write-Host ('baseline written (object/image merged): {0}' -f $Baseline) -ForegroundColor Green
    exit 0
}
if (-not $hasBase) {
    Write-Host ''
    Write-Host ('FAIL: baseline not found ({0}); create it with -WriteBaseline' -f $Baseline) -ForegroundColor Red
    exit 1
}
$base = Get-Content $Baseline -Raw -Encoding UTF8 | ConvertFrom-Json

# ---------------------------------------------------------------- compare
Write-Host ''
Write-Host '==== size vs baseline ===='
$drift = 0
foreach ($cfg in $objRes.Keys) {
    foreach ($f in $objRes[$cfg].Keys) {
        $cur = $objRes[$cfg][$f]
        $ref = $base.object.$cfg.$f
        if ($null -eq $ref) { Write-Host ('  no baseline: object {0} {1}' -f $cfg, $f) -ForegroundColor Yellow; continue }
        foreach ($k in @('Code', 'RO', 'RW', 'ZI')) {
            $d = $cur.$k - $ref.$k
            $ok = ([Math]::Abs($d) -le $TolObject)
            if (-not $ok) { $drift++ }
            $verdict = if ($ok) { 'PASS' } else { 'FAIL' }
            $c = if ($ok) { 'Green' } else { 'Red' }
            Write-Host ('  {0} object {1,-5} {2,-12} {3,-4} cur={4,-5} base={5,-5} delta={6,4}  {7}' -f `
                $verdict, $cfg, $f, $k, $cur.$k, $ref.$k, $d, ('tol=' + $TolObject)) -ForegroundColor $c
        }
    }
}
foreach ($cfg in $imgRes.Keys) {
    $cur = $imgRes[$cfg]
    $ref = $base.image.$cfg
    if ($null -eq $ref) { Write-Host ('  no baseline: image {0}' -f $cfg) -ForegroundColor Yellow; continue }
    foreach ($k in @('Code', 'RO', 'RW', 'ZI')) {
        $d = $cur.$k - $ref.$k
        $ok = ([Math]::Abs($d) -le $TolImage)
        if (-not $ok) { $drift++ }
        $verdict = if ($ok) { 'PASS' } else { 'FAIL' }
        $c = if ($ok) { 'Green' } else { 'Red' }
        Write-Host ('  {0} image  {1,-8} {2,-4} cur={3,-5} base={4,-5} delta={5,4}  {6}' -f `
            $verdict, $cfg, $k, $cur.$k, $ref.$k, $d, ('tol=' + $TolImage)) -ForegroundColor $c
    }
}
Write-Host ''
if ($drift -gt 0) {
    Write-Host ('==== size: FAIL ({0} metric(s) out of tolerance) ====' -f $drift) -ForegroundColor Red
    exit 1
}
Write-Host '==== size: PASS (all metrics within tolerance) ====' -ForegroundColor Green
exit 0

