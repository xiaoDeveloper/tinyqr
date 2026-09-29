#!/usr/bin/env python3
"""Print a ZIP-backed APK component breakdown using actual archive entries."""
import argparse
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument("apk")
args = parser.parse_args()

groups = {"AndroidManifest.xml": 0, "resources.arsc": 0, "classes.dex": 0,
          "lib/arm64-v8a/libtinyqr.so": 0, "res": 0, "assets": 0, "META-INF": 0, "other": 0}
with zipfile.ZipFile(args.apk) as apk:
    for entry in apk.infolist():
        name = entry.filename
        if name in groups: group = name
        elif name.startswith("res/"): group = "res"
        elif name.startswith("assets/"): group = "assets"
        elif name.startswith("META-INF/"): group = "META-INF"
        else: group = "other"
        groups[group] += entry.compress_size
    total = sum(entry.compress_size for entry in apk.infolist())

print("TinyQR APK size report\n")
print(f"{'APK':32} {total:>10,} B")
for name, size in groups.items(): print(f"{name:32} {size:>10,} B")
