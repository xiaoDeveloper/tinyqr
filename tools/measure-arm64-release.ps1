[CmdletBinding()]
param(
    [string]$OutputRoot = "benchmark-output"
)

$ErrorActionPreference = 'Stop'

$abi = 'arm64-v8a'
$projectRoot = Split-Path -Parent $PSScriptRoot
$outputDirectory = Join-Path $projectRoot $OutputRoot
$commit = (git -C $projectRoot rev-parse HEAD).Trim()
$shortCommit = (git -C $projectRoot rev-parse --short HEAD).Trim()
$workingTreeStatus = @(git -C $projectRoot status --porcelain)
$timestamp = Get-Date -Format 'yyyy-MM-ddTHH-mm-ss'
$runDirectory = Join-Path $outputDirectory "$timestamp-$shortCommit"

New-Item -ItemType Directory -Force -Path $runDirectory | Out-Null

$results = @(
    Write-Host "Building release APK for $abi..."
    & (Join-Path $projectRoot 'gradlew.bat') clean ':app:assembleRelease' | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "Release build failed for $abi (exit code $LASTEXITCODE)."
    }

    $sourceApk = Join-Path $projectRoot 'app\build\outputs\apk\release\app-release-unsigned.apk'
    if (-not (Test-Path -LiteralPath $sourceApk)) {
        throw "Expected release APK was not produced for ${abi}: $sourceApk"
    }

    $destinationApk = Join-Path $runDirectory "tinyqr-$abi-release-unsigned.apk"
    Copy-Item -LiteralPath $sourceApk -Destination $destinationApk

    $zip = [System.IO.Compression.ZipFile]::OpenRead($destinationApk)
    try {
        $nativeEntries = @($zip.Entries | Where-Object { $_.FullName -like 'lib/*/*.so' })
        $unexpectedEntries = @($nativeEntries | Where-Object { $_.FullName -notlike "lib/$abi/*" })
        if ($unexpectedEntries.Count -ne 0) {
            throw "APK for $abi contains unexpected native libraries: $($unexpectedEntries.FullName -join ', ')"
        }

        $nativeLibrary = $nativeEntries | Where-Object { $_.FullName -eq "lib/$abi/libtinyqr.so" }
        if ($null -eq $nativeLibrary) {
            throw "APK for $abi does not contain lib/$abi/libtinyqr.so."
        }

        [pscustomobject]@{
            Abi = $abi
            ApkBytes = (Get-Item -LiteralPath $destinationApk).Length
            NativeLibraryBytes = $nativeLibrary.Length
            NativeLibraryCompressedBytes = $nativeLibrary.CompressedLength
            Apk = (Split-Path -Leaf $destinationApk)
        }
    }
    finally {
        $zip.Dispose()
    }
)

$report = [pscustomobject]@{
    benchmark = 'TinyQR single-ABI release APK size benchmark'
    headCommit = $commit
    workingTreeStatus = $workingTreeStatus
    generatedAt = (Get-Date).ToString('o')
    artifactsDirectory = $runDirectory
    results = $results
}

$reportPath = Join-Path $runDirectory 'size-report.json'
$report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $reportPath -Encoding utf8
$results | Format-Table Abi, ApkBytes, NativeLibraryBytes, NativeLibraryCompressedBytes, Apk -AutoSize
Write-Host "Commit: $commit"
if ($workingTreeStatus.Count -ne 0) {
    Write-Host 'Built from HEAD plus the recorded working-tree changes in size-report.json.'
}
Write-Host "Artifacts and report: $runDirectory"
