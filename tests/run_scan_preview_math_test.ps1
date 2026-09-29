$ErrorActionPreference = 'Stop'
$output = Join-Path $PSScriptRoot 'scan_preview_math_test.exe'
& gcc -std=c11 `
    "$PSScriptRoot/scan_preview_math_test.c" `
    "$PSScriptRoot/../app/src/main/c/render/scan_preview_math.c" `
    -I "$PSScriptRoot/../app/src/main/c" -o $output
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
try { & $output } finally { if (Test-Path $output) { Remove-Item $output } }
