#!/bin/bash
set -u

STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="${1:-$HOME/Desktop/CrystalEye-SL-$STAMP}"
mkdir -p "$OUT"

{ sw_vers; uname -a; } >"$OUT/system.txt" 2>&1
system_profiler SPUSBDataType >"$OUT/system-profiler-usb.txt" 2>&1
ioreg -p IOUSB -l -w 0 >"$OUT/ioreg-usb.txt" 2>&1
kextstat >"$OUT/kextstat.txt" 2>&1
{ grep -i -E 'CrystalEye|064e|a101|UVC|VDC|USB|Photo Booth|iChat|QTKit' /var/log/system.log 2>/dev/null; grep -i -E 'CrystalEye|064e|a101|UVC|VDC|USB|timeout|stall|error' /var/log/kernel.log 2>/dev/null; } >"$OUT/relevant-logs.txt"

ditto -c -k --sequesterRsrc --keepParent "$OUT" "$OUT.zip"
echo "$OUT.zip"
echo "Review logs for usernames, machine names, and serial numbers before publishing."

