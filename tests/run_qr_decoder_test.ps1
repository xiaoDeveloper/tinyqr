param([switch]$Diagnostics, [switch]$Benchmark, [switch]$BenchmarkBlank, [switch]$BenchmarkStylized)
$ErrorActionPreference = 'Stop'
if ($Diagnostics -and ($Benchmark -or $BenchmarkBlank -or $BenchmarkStylized)) { throw 'Benchmark without diagnostic I/O.' }
if ((@($Benchmark, $BenchmarkBlank, $BenchmarkStylized) | Where-Object { $_ }).Count -gt 1) { throw 'Select one benchmark.' }
$output = Join-Path $PSScriptRoot 'qr_decoder_test.exe'
$defines = @()
if ($Diagnostics) { $defines += '-DTINYQR_DEBUG' }
& gcc -std=c11 -O2 @defines `
    "$PSScriptRoot/qr_decoder_test.c" `
    "$PSScriptRoot/../app/src/main/c/qr/qr.c" `
    "$PSScriptRoot/../app/src/main/c/third_party/quirc/quirc.c" `
    "$PSScriptRoot/../app/src/main/c/third_party/quirc/identify.c" `
    "$PSScriptRoot/../app/src/main/c/third_party/quirc/decode.c" `
    "$PSScriptRoot/../app/src/main/c/third_party/quirc/version_db.c" `
    -I "$PSScriptRoot/../app/src/main/c/third_party/quirc" -lm -o $output
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
try {
    $info = [System.Diagnostics.ProcessStartInfo]::new($output)
    $info.WorkingDirectory = Split-Path $PSScriptRoot -Parent
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    if ($Benchmark) { $info.Arguments = '--benchmark' }
    elseif ($BenchmarkBlank) { $info.Arguments = '--benchmark-blank' }
    elseif ($BenchmarkStylized) { $info.Arguments = '--benchmark-stylized' }
    $process = [System.Diagnostics.Process]::Start($info)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $outText = $stdout.Result
    $errText = $stderr.Result
    Write-Output $outText
    if ($errText) { Write-Output $errText }
    if ($process.ExitCode -ne 0) { throw "Decoder test failed: $($process.ExitCode)" }
    if ($Diagnostics) {
        foreach ($message in @(
            'frame=8x8 candidates=0',
            'frame=232x232 candidates=1',
            'candidate=0/1 frame=232x232 decode failed: ECC failure',
            'candidate=0/1 frame=232x232 flipped decode failed: ECC failure',
            'stylized fallback frame=227x192 candidates=1'
        )) {
            if (-not $errText.Contains($message)) { throw "Missing diagnostic: $message" }
        }
    } elseif ($errText) { throw 'Non-diagnostic host build unexpectedly logged.' }
} finally {
    if ($process) { $process.Dispose() }
    if (Test-Path $output) { Remove-Item -LiteralPath $output }
}
