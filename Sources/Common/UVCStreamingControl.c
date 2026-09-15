#include "UVCStreamingControl.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void write_le16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8) & 0xffu);
}

static void write_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8) & 0xffu);
    p[2] = (uint8_t)((value >> 16) & 0xffu);
    p[3] = (uint8_t)((value >> 24) & 0xffu);
}

int ce_uvc_decode(const uint8_t *bytes, size_t length,
                  CEUVCStreamingControl *control)
{
    if (bytes == NULL || control == NULL ||
        length < CE_UVC_STREAMING_CONTROL_SIZE) {
        return 0;
    }

    control->bmHint = read_le16(bytes + 0);
    control->bFormatIndex = bytes[2];
    control->bFrameIndex = bytes[3];
    control->dwFrameInterval = read_le32(bytes + 4);
    control->wKeyFrameRate = read_le16(bytes + 8);
    control->wPFrameRate = read_le16(bytes + 10);
    control->wCompQuality = read_le16(bytes + 12);
    control->wCompWindowSize = read_le16(bytes + 14);
    control->wDelay = read_le16(bytes + 16);
    control->dwMaxVideoFrameSize = read_le32(bytes + 18);
    control->dwMaxPayloadTransferSize = read_le32(bytes + 22);
    return 1;
}

int ce_uvc_encode(const CEUVCStreamingControl *control, uint8_t *bytes,
                  size_t length)
{
    if (bytes == NULL || control == NULL ||
        length < CE_UVC_STREAMING_CONTROL_SIZE) {
        return 0;
    }

    memset(bytes, 0, CE_UVC_STREAMING_CONTROL_SIZE);
    write_le16(bytes + 0, control->bmHint);
    bytes[2] = control->bFormatIndex;
    bytes[3] = control->bFrameIndex;
    write_le32(bytes + 4, control->dwFrameInterval);
    write_le16(bytes + 8, control->wKeyFrameRate);
    write_le16(bytes + 10, control->wPFrameRate);
    write_le16(bytes + 12, control->wCompQuality);
    write_le16(bytes + 14, control->wCompWindowSize);
    write_le16(bytes + 16, control->wDelay);
    write_le32(bytes + 18, control->dwMaxVideoFrameSize);
    write_le32(bytes + 22, control->dwMaxPayloadTransferSize);
    return 1;
}

const CEUVCProfile *ce_uvc_profile_named(const char *name)
{
    static const CEUVCProfile profiles[] = {
        { "320x240@30", 3, 333333u, 153600u, 800u },
        { "640x480@15", 1, 666666u, 614400u, 1600u },
        { "640x480@30", 1, 333333u, 614400u, 2400u }
    };
    size_t i;

    if (name == NULL) {
        return NULL;
    }
    for (i = 0; i < sizeof(profiles) / sizeof(profiles[0]); ++i) {
        if (strcmp(name, profiles[i].name) == 0) {
            return &profiles[i];
        }
    }
    return NULL;
}

void ce_uvc_apply_profile(CEUVCStreamingControl *control,
                          const CEUVCProfile *profile)
{
    if (control == NULL || profile == NULL) {
        return;
    }
    control->bmHint |= 1u;
    control->bFormatIndex = 1;
    control->bFrameIndex = profile->frameIndex;
    control->dwFrameInterval = profile->frameInterval;
    control->dwMaxVideoFrameSize = profile->maxVideoFrameSize;
    control->dwMaxPayloadTransferSize = profile->maxPayloadTransferSize;
}

void ce_uvc_print(FILE *stream, const char *label,
                  const CEUVCStreamingControl *control,
                  const uint8_t *rawBytes)
{
    size_t i;
    double fps;

    if (stream == NULL || control == NULL) {
        return;
    }
    fps = control->dwFrameInterval == 0 ? 0.0 :
          10000000.0 / (double)control->dwFrameInterval;
    fprintf(stream, "%s\n", label == NULL ? "UVC control" : label);
    if (rawBytes != NULL) {
        fprintf(stream, "  raw:");
        for (i = 0; i < CE_UVC_STREAMING_CONTROL_SIZE; ++i) {
            fprintf(stream, " %02x", (unsigned int)rawBytes[i]);
        }
        fprintf(stream, "\n");
    }
    fprintf(stream, "  bmHint:                     0x%04x\n",
            (unsigned int)control->bmHint);
    fprintf(stream, "  bFormatIndex:               %u\n",
            (unsigned int)control->bFormatIndex);
    fprintf(stream, "  bFrameIndex:                %u\n",
            (unsigned int)control->bFrameIndex);
    fprintf(stream, "  dwFrameInterval:            %lu (%.3f fps)\n",
            (unsigned long)control->dwFrameInterval, fps);
    fprintf(stream, "  dwMaxVideoFrameSize:        %lu bytes\n",
            (unsigned long)control->dwMaxVideoFrameSize);
    fprintf(stream, "  dwMaxPayloadTransferSize:   %lu bytes/microframe\n",
            (unsigned long)control->dwMaxPayloadTransferSize);
}

