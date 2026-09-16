<#
    XCOS host regression runner  (Tests/run_tests.ps1)

    NOTE: this script is intentionally ASCII-only -- Windows PowerShell 5.1
          decodes BOM-less UTF-8 script files as ANSI, which would corrupt
          non-ASCII string literals inside the *script*. All Chinese text
          lives in Tests/manifest.txt / Tests/README.md (read with explicit
          UTF-8) instead.

    Usage:
        powershell -File Tests/run_tests.ps1                    # run all
        powershell -File Tests/run_tests.ps1 -Filter B4         # name filter (regex)
        powershell -File Tests/run_tests.ps1 -UpdateSnapshots   # refresh snapshots/*
        powershell -File Tests/run_tests.ps1 -Gcc <path> -ExtraPath <dir>

    Exit code: 0 = all executed cases pass, 1 = at least one failed, 2 = gcc missing.
#>
[CmdletBinding()]
param(
    [string] $Filter        = '',
    [switch] $UpdateSnapshots,
    [string] $Gcc           = '',
    [string] $ExtraPath     = 'C:\Software\qalculate',
    [int]    $TimeoutSec    = 30,
    [string] $Manifest      = '',
    [string] $OutDir        = ''
)

$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root     = Split-Path -Parent $testsDir
if ([string]::IsNullOrEmpty($Manifest)) { $Manifest = Join-Path $testsDir 'manifest.txt' }
if ([string]::IsNullOrEmpty($OutDir))   { $OutDir   = Join-Path $env:TEMP ('xcos_tests_' + (Get-Date -Format 'yyyyMMdd_HHmmss')) }
$snapDir  = Join-Path $testsDir 'snapshots'
[void][System.IO.Directory]::CreateDirectory($OutDir)
[void][System.IO.Directory]::CreateDirectory($snapDir)

# ---------- locate compiler ----------
function Find-Gcc([string] $explicit) {
    if ($explicit -ne '' -and (Test-Path $explicit)) { return (Resolve-Path $explicit).Path }
    $cands = @('C:\Software\CLion\bin\mingw\bin\gcc.exe',
               'C:\mingw64\bin\gcc.exe',
               'C:\msys64\mingw64\bin\gcc.exe',
               'C:\TDM-GCC-64\bin\gcc.exe')
    foreach ($c in $cands) { if (Test-Path $c) { return $c } }
    $cmd = Get-Command gcc -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return ''
}
$gccExe = Find-Gcc $Gcc
if ($gccExe -eq '') {
    Write-Host 'SKIP: gcc not found. Pass -Gcc <path> (e.g. C:\Software\CLion\bin\mingw\bin\gcc.exe).' -ForegroundColor Yellow
    exit 2
}
# ---------- make the toolchain self-sufficient on PATH ----------
# The bundled mingw's cc1.exe needs runtime DLLs from two places:
#   1) the compiler's OWN bin dir  -> libssp-0.dll / libgcc_s_*.dll / libwinpthread-1.dll / libstdc++-6.dll
#   2) -ExtraPath                  -> libgmp / libmpfr / zlib (cc1 links against them)
# If the caller's shell happens to lack them, cc1 dies with rc=0xC0000135 (DLL not found) and
# EMPTY output - which looks exactly like "the compiler is broken". Prepending both dirs here
# keeps this channel independent of the caller's environment.
$pathDirs = @()
$gccDir = Split-Path -Parent $gccExe
if ($gccDir -ne '' -and (Test-Path $gccDir))     { $pathDirs += $gccDir }
if ($ExtraPath -ne '' -and (Test-Path $ExtraPath)) { $pathDirs += $ExtraPath }
if ($pathDirs.Count -gt 0) { $env:PATH = ($pathDirs -join ';') + ';' + $env:PATH }

# ---------- smoke test: make sure the compiler can actually run ----------
# (a missing cc1 runtime DLL shows up as rc!=0 with EMPTY stdout/stderr, which is
#  very confusing; this turns it into an explicit, actionable message)
$smokeSrc = Join-Path $OutDir '_smoke.c'
$smokeObj = Join-Path $OutDir '_smoke.o'
[System.IO.File]::WriteAllText($smokeSrc, "int main(void){return 0;}`r`n", (New-Object System.Text.UTF8Encoding($false)))
$smokeArgs = @('-c', $smokeSrc, '-o', $smokeObj)
$smoke = Start-Process -FilePath $gccExe -ArgumentList $smokeArgs -NoNewWindow -Wait -PassThru `
                       -RedirectStandardOutput (Join-Path $OutDir '_smoke.out') `
                       -RedirectStandardError  (Join-Path $OutDir '_smoke.err')
if ($smoke.ExitCode -ne 0) {
    $rcHex = ('0x{0:X8}' -f [uint32]$smoke.ExitCode)
    Write-Host ('FAIL: compiler cannot run (rc={0} / {1})' -f $smoke.ExitCode, $rcHex) -ForegroundColor Red
    Write-Host ('  gcc      : {0}' -f $gccExe)
    Write-Host ('  PATH add : {0}' -f (($pathDirs -join ';')))
    Write-Host '  hint     : rc=0xC0000135 (DLL not found) with empty output => cc1 is missing runtime DLLs.'
    Write-Host '             Need libssp-0.dll / libgcc_s_*.dll from the gcc dir, plus libgmp/libmpfr/zlib'
    Write-Host '             (the latter normally comes from -ExtraPath, e.g. C:\Software\qalculate).'
    Get-Content (Join-Path $OutDir '_smoke.err') -ErrorAction SilentlyContinue | Select-Object -First 5 | ForEach-Object { Write-Host ('  ' + $_) }
    exit 2
}

# ---------- load manifest ----------
$utf8   = New-Object System.Text.UTF8Encoding($false)
$lines  = [System.IO.File]::ReadAllLines($Manifest, $utf8)
$src    = @()
$incDir = Join-Path $root 'Code\Inc'
$cases  = @()
foreach ($ln in $lines) {
    $t = $ln.Trim()
    if ($t -eq '' -or $t.StartsWith('#')) { continue }
    $f = $t.Split("`t")
    if ($f[0] -eq 'SRC')    { $src   += (Join-Path $root $f[1]); continue }
    if ($f[0] -eq 'INCDIR') { $incDir = Join-Path $root $f[1];   continue }
    while ($f.Count -lt 7) { $f += '' }
    $cases += [pscustomobject]@{
        Name = $f[0]; Kind = $f[1]; File = (Join-Path $root $f[2])
        Defines = $f[3]; Expect = $f[4]; Snapshot = $f[5]; Note = $f[6]
    }
}
Write-Host ("XCOS host regression: gcc = {0}" -f $gccExe)
Write-Host ("  manifest : {0} ({1} cases, {2} kernel sources)" -f $Manifest, $cases.Count, $src.Count)
Write-Host ("  artifacts: {0}" -f $OutDir)
Write-Host ''

# ---------- run ----------
$results = @()
$idx = 0
foreach ($c in $cases) {
    $idx++
    if ($Filter -ne '' -and ($c.Name -notmatch $Filter)) { continue }

    if ($c.Kind -eq 'skip') {
        $results += [pscustomobject]@{ Name=$c.Name; Verdict='SKIP'; Detail=$c.Note }
        continue
    }
    if (-not (Test-Path $c.File)) {
        $results += [pscustomobject]@{ Name=$c.Name; Verdict='FAIL'; Detail='case file not found' }
        continue
    }

    $tag     = ('{0:d2}_{1}' -f $idx, ($c.Name -replace '[^\w\-]', '_'))
    $exe     = Join-Path $OutDir ($tag + '.exe')
    $ccLog   = Join-Path $OutDir ($tag + '.cc.log')
    $outLog  = Join-Path $OutDir ($tag + '.out.txt')
    $defArgs = @()
    if ($c.Defines.Trim() -ne '') { $defArgs = $c.Defines.Trim().Split(' ') }

    # compile (Start-Process + file redirection: avoids PowerShell's error-stream
    # semantics on native stderr under $ErrorActionPreference = 'Stop')
    $ccArgs = @('-std=gnu99', '-O1', '-Wall', '-Wextra') + $defArgs + @('-I', $incDir, '-o', $exe) + $src + @($c.File)
    $ccErr  = $ccLog + '.err'
    $ccProc = Start-Process -FilePath $gccExe -ArgumentList $ccArgs -NoNewWindow -Wait -PassThru `
                            -RedirectStandardOutput $ccLog -RedirectStandardError $ccErr
    $ccRc   = $ccProc.ExitCode
    $ccWarn = 0
    foreach ($lf in @($ccLog, $ccErr)) {
        if ((Test-Path $lf) -and ((Get-Item $lf).Length -gt 0)) {
            $ccWarn += (@(Select-String -Path $lf -Pattern 'warning:' -ErrorAction SilentlyContinue)).Count
        }
    }
    if ($ccRc -ne 0) {
        $first = (Get-Content $ccLog -ErrorAction SilentlyContinue | Select-Object -First 1)
        $results += [pscustomobject]@{ Name=$c.Name; Verdict='FAIL'; Detail=('compile rc={0}: {1}' -f $ccRc, $first) }
        continue
    }

    # run (with timeout)
    $proc = Start-Process -FilePath $exe -NoNewWindow -PassThru -RedirectStandardOutput $outLog -RedirectStandardError ($outLog + '.err')
    if (-not $proc.WaitForExit($TimeoutSec * 1000)) {
        try { $proc.Kill() } catch { }
        $results += [pscustomobject]@{ Name=$c.Name; Verdict='FAIL'; Detail=('timeout > {0}s' -f $TimeoutSec) }
        continue
    }
    $rc  = $proc.ExitCode
    $out = if (Test-Path $outLog) { [System.IO.File]::ReadAllText($outLog, $utf8) } else { '' }

    # judge
    $verdict = 'FAIL'; $detail = ''
    if ($c.Kind -eq 'marker') {
        if ($out -match $c.Expect) { $verdict = 'PASS'; $detail = $c.Expect }
        else { $detail = ('marker not found: {0}' -f $c.Expect) }
    }
    elseif ($c.Kind -eq 'snapshot') {
        $snapName = if ($c.Snapshot.Trim() -ne '') { $c.Snapshot.Trim() } else { $c.Expect.Trim() }
        $snapPath = Join-Path $snapDir $snapName
        $norm = ($out -replace "`r`n", "`n").TrimEnd("`n")
        if ($UpdateSnapshots) {
            [System.IO.File]::WriteAllText($snapPath, $norm, $utf8)
            $verdict = 'PASS'; $detail = ('snapshot updated: {0}' -f $snapName)
        }
        elseif (-not (Test-Path $snapPath)) {
            $detail = ('snapshot missing: {0} (run -UpdateSnapshots)' -f $snapName)
        }
        else {
            $exp = ([System.IO.File]::ReadAllText($snapPath, $utf8) -replace "`r`n", "`n").TrimEnd("`n")
            if ($norm -eq $exp) { $verdict = 'PASS'; $detail = ('snapshot ok: {0}' -f $snapName) }
            else {
                $na = $norm.Split("`n"); $ea = $exp.Split("`n")
                $diffAt = -1
                for ($i = 0; $i -lt [Math]::Max($na.Count, $ea.Count); $i++) {
                    $a = if ($i -lt $na.Count) { $na[$i] } else { '<missing>' }
                    $b = if ($i -lt $ea.Count) { $ea[$i] } else { '<missing>' }
                    if ($a -ne $b) { $diffAt = $i + 1; break }
                }
                $detail = ('snapshot differs at line {0}' -f $diffAt)
            }
        }
    }
    else { $detail = ('unknown kind: {0}' -f $c.Kind) }
    if ($verdict -eq 'PASS' -and $ccWarn -gt 0) { $detail += (' [warnings: {0}]' -f $ccWarn) }
    $results += [pscustomobject]@{ Name=$c.Name; Verdict=$verdict; Detail=$detail }
}

# ---------- report ----------
Write-Host '==== results ===='
foreach ($r in $results) {
    $color = switch ($r.Verdict) { 'PASS' { 'Green' } 'SKIP' { 'DarkGray' } default { 'Red' } }
    Write-Host ('  {0,-6} {1,-26} {2}' -f $r.Verdict, $r.Name, $r.Detail) -ForegroundColor $color
}
$pass = (@($results | Where-Object { $_.Verdict -eq 'PASS' })).Count
$fail = (@($results | Where-Object { $_.Verdict -eq 'FAIL' })).Count
$skip = (@($results | Where-Object { $_.Verdict -eq 'SKIP' })).Count
Write-Host ''
Write-Host ('==== host regression: PASS {0} / FAIL {1} / SKIP {2} ====' -f $pass, $fail, $skip)
if ($fail -gt 0) { exit 1 } else { exit 0 }

