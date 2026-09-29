param([Parameter(Mandatory = $true)][string]$ImagePath)
$ErrorActionPreference = 'Stop'
$image = (Resolve-Path -LiteralPath $ImagePath).Path
$output = Join-Path $PSScriptRoot 'qr_inspect.exe'
$quirc = Join-Path $PSScriptRoot '../app/src/main/c/third_party/quirc'
& gcc -std=c11 -O2 "$PSScriptRoot/qr_inspect.c" `
    "$quirc/quirc.c" "$quirc/identify.c" "$quirc/decode.c" "$quirc/version_db.c" `
    -lm -o $output
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
try {
    & $output $image
    $result = $LASTEXITCODE
} finally {
    Remove-Item -LiteralPath $output
}
# 0: decoded; 1: valid image but no decode; 2: invalid input / allocation failure.
exit $result
