#ifndef CE_UVC_STREAMING_CONTROL_H
#define CE_UVC_STREAMING_CONTROL_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define CE_UVC_STREAMING_CONTROL_SIZE 26u

typedef struct CEUVCStreamingControl {
    uint16_t bmHint;
    uint8_t bFormatIndex;
    uint8_t bFrameIndex;
    uint32_t dwFrameInterval;
    uint16_t wKeyFrameRate;
    uint16_t wPFrameRate;
    uint16_t wCompQuality;
    uint16_t wCompWindowSize;
    uint16_t wDelay;
    uint32_t dwMaxVideoFrameSize;
    uint32_t dwMaxPayloadTransferSize;
} CEUVCStreamingControl;

typedef struct CEUVCProfile {
    const char *name;
    uint8_t frameIndex;
    uint32_t frameInterval;
    uint32_t maxVideoFrameSize;
    uint32_t maxPayloadTransferSize;
} CEUVCProfile;

int ce_uvc_decode(const uint8_t *bytes, size_t length,
                  CEUVCStreamingControl *control);
int ce_uvc_encode(const CEUVCStreamingControl *control, uint8_t *bytes,
                  size_t length);
const CEUVCProfile *ce_uvc_profile_named(const char *name);
void ce_uvc_apply_profile(CEUVCStreamingControl *control,
                          const CEUVCProfile *profile);
void ce_uvc_print(FILE *stream, const char *label,
                  const CEUVCStreamingControl *control,
                  const uint8_t *rawBytes);

#endif

