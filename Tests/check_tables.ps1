<#
    XCOS table alignment checker   (Tests/check_tables.ps1)

    Why this exists
    ---------------
    Code editors do NOT render Markdown, so a table written inside a source
    comment shows up as raw text -- it must already line up *in the source*.
    The same convention is applied to the doc tables this repo owns.

    Width rule (matches Docs/Skill/XCOS_Dev_Agent_Skill.md section 3, item 6)
    ------------------------------------------------------------------------
    ASCII 0x20..0x7E counts as 1 display column; every other character
    (CJK, full-width punctuation, arrows, circles, emoji) counts as 2 columns
    -- i.e. "2 English characters == 1 Chinese character".

    A "table block" = consecutive lines whose first non-blank text is '|'
    (an optional comment leader '*', '//' or '/*' is allowed in front).
    Blocks inside Markdown code fences are skipped (they are examples).

    Usage
    -----
        powershell -File Tests/check_tables.ps1
        powershell -File Tests/check_tables.ps1 -Roots Code,Docs -Verbose
        powershell -File Tests/check_tables.ps1 -Roots . -RootsExclude Examples

    Exit code: 0 = every block aligned; 1 = at least one block misaligned.
#>
param(
    [string[]]$Roots = @('Code', 'Tests', 'Docs', 'Examples', 'README.md'),
    [string[]]$RootsExclude = @('Lib'),
    [switch]$Verbose
)
$ErrorActionPreference = 'Stop'

function Get-DisplayWidth([string]$s) {
    $w = 0
    foreach ($ch in $s.ToCharArray()) {
        $c = [int][char]$ch
        if ($c -ge 0x20 -and $c -le 0x7E) { $w += 1 } else { $w += 2 }
    }
    return $w
}

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$files = @()
foreach ($r in $Roots) {
    $p = $r
    if (-not [System.IO.Path]::IsPathRooted($p)) { $p = Join-Path $root $p }
    if (-not (Test-Path -LiteralPath $p)) { continue }
    if ((Get-Item -LiteralPath $p).PSIsContainer) {
        $files += Get-ChildItem -LiteralPath $p -Recurse -File -Include *.c, *.h, *.md
    } else {
        $files += Get-Item -LiteralPath $p
    }
}
if ($RootsExclude.Count -gt 0) {
    $files = $files | Where-Object { $t = $_.FullName; -not ($RootsExclude | Where-Object { $t -like ('*\' + $_ + '\*') }) }
}

$blocks = 0
$bad = 0
foreach ($f in $files) {
    $lines = [System.IO.File]::ReadAllLines($f.FullName, (New-Object System.Text.UTF8Encoding($false)))
    $isMd = ($f.Extension -eq '.md')
    $inFence = $false
    $i = 0
    while ($i -lt $lines.Count) {
        if ($isMd -and $lines[$i] -match '^\s*```') { $inFence = -not $inFence; $i++; continue }
        if ($lines[$i] -notmatch '^\s*(?:\*|//|/\*)?\s*\|') { $i++; continue }
        $start = $i
        while ($i -lt $lines.Count -and $lines[$i] -match '^\s*(?:\*|//|/\*)?\s*\|') { $i++ }
        if ($inFence) { continue }
        # a real (Markdown) table must contain a separator row: | --- | --- |
        # (guards against ASCII-art/diagram blocks that merely start with '|')
        $hasSep = $false
        for ($k = $start; $k -lt $i; $k++) {
            if ($lines[$k] -match '^[\s|:\-]+$' -and $lines[$k] -match '-{3,}') { $hasSep = $true; break }
        }
        if (-not $hasSep) { continue }
        $blocks++
        # rows whose pipe count differs from the block's modal count contain an
        # unescaped pipe inside a cell (Markdown quirk) -> not comparable, only noted
        $cnts = @{}
        for ($k = $start; $k -lt $i; $k++) {
            $c = ([regex]::Matches($lines[$k], '(?<!\\)\|')).Count
            if (-not $cnts.ContainsKey($c)) { $cnts[$c] = 0 }
            $cnts[$c]++
        }
        $modal = ($cnts.GetEnumerator() | Sort-Object -Property Value -Descending | Select-Object -First 1).Key
        $ws = @()
        $noted = 0
        for ($k = $start; $k -lt $i; $k++) {
            if (([regex]::Matches($lines[$k], '(?<!\\)\|')).Count -ne $modal) {
                $noted++
                if ($Verbose) {
                    $rel = $f.FullName.Substring($root.Length + 1)
                    Write-Host ('NOTE  {0}  L{1}  unescaped-pipe row (skipped)' -f $rel, ($k + 1))
                }
                continue
            }
            $ws += (Get-DisplayWidth $lines[$k])
        }
        $mn = ($ws | Measure-Object -Minimum).Minimum
        $mx = ($ws | Measure-Object -Maximum).Maximum
        if ($mn -ne $mx) {
            $bad++
            $rel = $f.FullName.Substring($root.Length + 1)
            Write-Host ('BAD   {0}  L{1}-{2}  width={3}..{4}' -f $rel, ($start + 1), $i, $mn, $mx)
        } elseif ($Verbose) {
            $rel = $f.FullName.Substring($root.Length + 1)
            Write-Host ('ok    {0}  L{1}-{2}  width={3}' -f $rel, ($start + 1), $i, $mn)
        }
    }
}

if ($bad -gt 0) {
    Write-Host ('==== check_tables: FAIL ({0} of {1} table blocks misaligned) ====' -f $bad, $blocks)
    exit 1
}
Write-Host ('==== check_tables: PASS ({0} table blocks, width rule: ASCII=1 / CJK=2) ====' -f $blocks)
exit 0
