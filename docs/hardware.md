# Target hardware

## USB identity

| Field | Value |
| --- | --- |
| Product | Acer CrystalEye webcam |
| Manufacturer | SuYin |
| Vendor ID | `0x064e` |
| Product ID | `0xa101` |
| Device release | `1.00` |
| USB speed | High Speed (480 Mbit/s) |
| UVC version | 1.00 |
| Native pixel format | YUY2, 16 bits/pixel |

The tested unit reports serial `CN0314-OV03-VA-R02.00.00` and location
`0xfd700000`. Location and serial may differ between machines; VID/PID are the
hard match used by the tools.

## Advertised video modes

| Frame index | Resolution | Intervals |
| --- | --- | --- |
| 1 | 640x480 | 30, 20, 15, 10, 5, 1 fps |
| 2 | 352x288 | 30, 20, 15, 10, 5, 1 fps |
| 3 | 320x240 | 30, 20, 15, 10, 5, 1 fps |
| 4 | 176x144 | 30, 20, 15, 10, 5, 1 fps |
| 5 | 160x120 | 30, 20, 15, 10, 5, 1 fps |

The camera exposes one isochronous IN endpoint with alternate settings whose
effective capacities are approximately 128, 256, 800, 1600, 2400, and 3000
bytes per microframe. VGA YUY2 at 30 fps requires an average of 2304 bytes per
microframe, so the physical bus has enough bandwidth.

## Descriptor anomalies

- The static descriptor says frame index 1 (640x480) is the default.
- Historical reports describe the cold runtime state as 320x240.
- The advertised minimum/maximum bitrate fields appear to use bytes per second
  even though UVC defines those fields in bits per second (factor-of-eight
  discrepancy).
- Linux reports for this model show `GET_DEF(PROBE)` as unsupported and enable a
  workaround.

The first development milestone therefore logs the runtime 26-byte PROBE state
instead of assuming the static descriptor is truthful.

