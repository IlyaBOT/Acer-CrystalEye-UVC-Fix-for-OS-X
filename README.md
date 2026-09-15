# Acer CrystalEye UVC Fix for OS X

Experimental compatibility tools for the SuYin Acer CrystalEye webcam
`064e:a101` on Mac OS X Snow Leopard 10.6.8 (i386).

The target camera works in iChat, but Photo Booth starts with a black preview.
Starting iChat while Photo Booth is open initializes the stream and makes video
appear in both applications. This repository isolates and reproduces that
initialization sequence before attempting a permanent, device-specific patch.

## Current status

- Exact hardware match: USB VID `0x064e`, PID `0xa101`.
- `crystaleye-uvc`: dumps the 26-byte UVC 1.0 PROBE control and can explicitly
  negotiate one of three known-safe YUY2 profiles.
- `crystaleye-primer`: starts an exact-device QTKit capture session, waits while
  frames are delivered, then releases the camera. It is the first replacement
  for the historical CameraControl/iChat priming workaround.
- No KEXT is installed and no system file is modified.
- Apple/iSight identity injection is intentionally deferred; it does not fix
  stream negotiation by itself.

## Build on Snow Leopard

Requirements: Mac OS X 10.6.8, Xcode 3.2.6 in `/Developer`, GCC 4.2, i386 kernel
or an i386-capable userland.

```sh
git clone https://github.com/IlyaBOT/Acer-CrystalEye-UVC-Fix-for-OS-X.git
cd Acer-CrystalEye-UVC-Fix-for-OS-X
git switch initial-uvc-probe 2>/dev/null || git checkout initial-uvc-probe
chmod +x scripts/*.sh
./scripts/build-snowleopard.sh
```

## Test 1: read the current UVC state

Close iChat, Photo Booth, Skype, and other camera applications first.

```sh
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && ./build/crystaleye-uvc --dump
```

If opening the USB streaming interface is denied, repeat that command with
`sudo`. The tool deliberately skips `GET_DEF(PROBE)` by default because this
camera is known to stall that request. Use `--include-get-def` only for a
controlled diagnostic run.

## Test 2: reproduce the Photo Booth priming workaround

Run this from the logged-in GUI user's SSH session:

```sh
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && open -a "Photo Booth" && sleep 3 && ./build/crystaleye-primer --width 320 --height 240 --seconds 10
```

Expected result: the tool prints `first frame`, Photo Booth changes from black
to live video, and video remains after the tool exits. If that fails, repeat
with `--width 640 --height 480`.

## Test 3: explicit UVC negotiation

This changes volatile camera state only. It does not write firmware, EEPROM,
the EFI partition, or `/System/Library/Extensions`.

```sh
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && sudo ./build/crystaleye-uvc --negotiate 320x240@30 --apply && ./build/crystaleye-uvc --dump
```

Available profiles:

- `320x240@30`
- `640x480@15`
- `640x480@30`

Run the QTKit primer after negotiation and record both command outputs. See
[`docs/testing.md`](docs/testing.md) for the test order and log collection.

## Safety

This is diagnostic software for one exact USB device. Write operations require
both `--negotiate` and `--apply`. Disconnecting or rebooting restores the
camera's volatile state. Do not test while an important camera recording is in
progress.

## Research basis

The project builds on the behavior documented by the [historical netkas
CrystalEye workaround](https://web.archive.org/web/20150318100659/http://netkas.org/?p=909)
and the [public-domain CameraControl work](https://phoboslab.org/log/2009/07/uvc-camera-control-for-mac-os-x)
by Dominic Szablewski/PhobosLab. Their workaround started a capture session;
its cosmetic AnyiSightCam injector was not the functional video fix. No
unavailable RGhost binary is redistributed here.

## License

MIT. See [`LICENSE`](LICENSE).
