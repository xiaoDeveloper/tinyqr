$ErrorActionPreference = 'Stop'
$output = Join-Path $PSScriptRoot 'scan_result_test.exe'
& gcc -std=c11 `
    "$PSScriptRoot/scan_result_test.c" `
    "$PSScriptRoot/../app/src/main/c/scan/scan_result.c" `
    "$PSScriptRoot/../app/src/main/c/render/result_ui.c" `
    -I "$PSScriptRoot/../app/src/main/c" -o $output
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
try { & $output } finally { if (Test-Path $output) { Remove-Item $output } }
