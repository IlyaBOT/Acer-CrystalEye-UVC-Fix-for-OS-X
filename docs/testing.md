# Snow Leopard test procedure

Use the same SD-free, otherwise stable boot for all runs. Quit every camera
application before each numbered test unless the step says otherwise.

## 1. Build and parser self-test

```sh
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && chmod +x scripts/*.sh && ./scripts/run-tests.sh && ./scripts/build-snowleopard.sh
```

## 2. Cold PROBE dump

```sh
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && sudo ./build/crystaleye-uvc --dump | tee ~/Desktop/crystaleye-cold-probe.txt
```

Record `bFrameIndex`, `dwFrameInterval`, `dwMaxVideoFrameSize`, and
`dwMaxPayloadTransferSize` from `GET_CUR(PROBE)`.

## 3. QTKit priming test

Open Photo Booth first and leave its black preview visible.

```sh
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && ./build/crystaleye-primer --width 320 --height 240 --seconds 10 | tee ~/Desktop/crystaleye-primer-320.txt
```

Record whether the primer received frames and whether Photo Booth woke up.
Repeat once with 640x480.

```sh
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && ./build/crystaleye-primer --width 640 --height 480 --seconds 10 | tee ~/Desktop/crystaleye-primer-640.txt
```

## 4. Explicit negotiation matrix

Reboot or disconnect/reconnect the camera between profiles when possible.

```sh
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && sudo ./build/crystaleye-uvc --negotiate 320x240@30 --apply && ./build/crystaleye-primer --width 320 --height 240 --seconds 10
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && sudo ./build/crystaleye-uvc --negotiate 640x480@15 --apply && ./build/crystaleye-primer --width 640 --height 480 --seconds 10
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && sudo ./build/crystaleye-uvc --negotiate 640x480@30 --apply && ./build/crystaleye-primer --width 640 --height 480 --seconds 10
```

## 5. Collect system evidence

```sh
cd ~/Documents/Acer-CrystalEye-UVC-Fix-for-OS-X && ./scripts/collect-logs.sh
```

The script creates a ZIP on the Desktop. Review it before publishing because
IORegistry and system logs can contain machine names, usernames, and serials.

