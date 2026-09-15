#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "UVCStreamingControl.h"

int main(void)
{
    static const uint8_t coldProbe[CE_UVC_STREAMING_CONTROL_SIZE] = {
        0x01, 0x00, 0x01, 0x03,
        0x15, 0x16, 0x05, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
        0x00, 0x58, 0x02, 0x00,
        0x20, 0x03, 0x00, 0x00
    };
    uint8_t encoded[CE_UVC_STREAMING_CONTROL_SIZE];
    CEUVCStreamingControl control;
    const CEUVCProfile *profile;

    assert(ce_uvc_decode(coldProbe, sizeof(coldProbe), &control));
    assert(control.bFormatIndex == 1);
    assert(control.bFrameIndex == 3);
    assert(control.dwFrameInterval == 333333u);
    assert(control.dwMaxVideoFrameSize == 153600u);
    assert(control.dwMaxPayloadTransferSize == 800u);

    memset(encoded, 0xff, sizeof(encoded));
    assert(ce_uvc_encode(&control, encoded, sizeof(encoded)));
    assert(memcmp(encoded, coldProbe, sizeof(encoded)) == 0);

    profile = ce_uvc_profile_named("640x480@30");
    assert(profile != NULL);
    ce_uvc_apply_profile(&control, profile);
    assert(control.bFrameIndex == 1);
    assert(control.dwFrameInterval == 333333u);
    assert(control.dwMaxVideoFrameSize == 614400u);
    assert(control.dwMaxPayloadTransferSize == 2400u);
    assert(ce_uvc_profile_named("invalid") == NULL);

    puts("UVC streaming-control tests passed");
    return 0;
}

