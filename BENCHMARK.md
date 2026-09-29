# TinyQR Release Size Benchmark

## v1.0.0 — Initial Public Baseline

This is the initial public baseline for the TinyQR optimization challenge. TinyQR ships exclusively for `arm64-v8a`; the measurement is an unsigned arm64-v8a release APK. Bytes are authoritative and KiB values are provided for readability. The `libtinyqr.so` column reports uncompressed library bytes; for v1.0.0, the ZIP-entry compressed bytes are identical.

| ABI | APK | `libtinyqr.so` |
|---|---:|---:|
| arm64-v8a | 70,652 B (69.00 KiB) | 50,848 B (49.66 KiB) |

### Provenance
- The original measurement includes recorded working-tree changes and is not a clean-commit build.

- Measurement date: 2026-09-04.

### Reproduction

After creating the repository's first local commit, run [tools/measure-arm64-release.ps1](tools/measure-arm64-release.ps1) from the repository root. The script records `HEAD` and the working-tree status, runs `clean :app:assembleRelease`, and writes the unsigned APK and `size-report.json` under ignored `benchmark-output/`. It reports APK bytes plus compressed and uncompressed `libtinyqr.so` bytes, and rejects APKs missing that library or containing another ABI. A dirty-tree report lists changed paths but does not preserve their contents.

Future versions must append rows using the same format; they must not overwrite the v1.0.0 baseline.
