#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USB.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UVCStreamingControl.h"

#define CRYSTALEYE_VENDOR_ID  0x064e
#define CRYSTALEYE_PRODUCT_ID 0xa101
#define UVC_VIDEO_CLASS 14
#define UVC_STREAMING_SUBCLASS 2
#define UVC_SET_CUR 0x01
#define UVC_GET_CUR 0x81
#define UVC_GET_MIN 0x82
#define UVC_GET_MAX 0x83
#define UVC_GET_DEF 0x87
#define UVC_VS_PROBE_CONTROL 0x01
#define UVC_VS_COMMIT_CONTROL 0x02

typedef struct Options {
    int dump;
    int includeGetDef;
    int apply;
    const CEUVCProfile *profile;
} Options;

static void usage(const char *program)
{
    fprintf(stderr,
            "usage:\n"
            "  %s --dump [--include-get-def]\n"
            "  %s --negotiate PROFILE --apply [--dump]\n"
            "\n"
            "profiles: 320x240@30, 640x480@15, 640x480@30\n",
            program, program);
}

static int parse_options(int argc, char **argv, Options *options)
{
    int i;
    memset(options, 0, sizeof(*options));

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--dump") == 0) {
            options->dump = 1;
        } else if (strcmp(argv[i], "--include-get-def") == 0) {
            options->includeGetDef = 1;
        } else if (strcmp(argv[i], "--apply") == 0) {
            options->apply = 1;
        } else if (strcmp(argv[i], "--negotiate") == 0 && i + 1 < argc) {
            options->profile = ce_uvc_profile_named(argv[++i]);
            if (options->profile == NULL) {
                fprintf(stderr, "unknown profile: %s\n", argv[i]);
                return 0;
            }
        } else {
            fprintf(stderr, "unknown or incomplete option: %s\n", argv[i]);
            return 0;
        }
    }

    if (!options->dump && options->profile == NULL) {
        return 0;
    }
    if (options->profile != NULL && !options->apply) {
        fprintf(stderr,
                "refusing to change volatile camera state without --apply\n");
        return 0;
    }
    return 1;
}

static IOUSBDeviceInterface **copy_target_device(void)
{
    CFMutableDictionaryRef matching;
    CFNumberRef number;
    io_service_t service;
    IOCFPlugInInterface **plugin;
    IOUSBDeviceInterface **device;
    SInt32 score;
    SInt32 vendor;
    SInt32 product;
    HRESULT result;
    IOReturn kr;

    matching = IOServiceMatching(kIOUSBDeviceClassName);
    if (matching == NULL) {
        return NULL;
    }

    vendor = CRYSTALEYE_VENDOR_ID;
    number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vendor);
    CFDictionarySetValue(matching, CFSTR(kUSBVendorID), number);
    CFRelease(number);

    product = CRYSTALEYE_PRODUCT_ID;
    number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &product);
    CFDictionarySetValue(matching, CFSTR(kUSBProductID), number);
    CFRelease(number);

    service = IOServiceGetMatchingService(kIOMasterPortDefault, matching);
    if (service == IO_OBJECT_NULL) {
        return NULL;
    }

    plugin = NULL;
    device = NULL;
    score = 0;
    kr = IOCreatePlugInInterfaceForService(
        service, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
        &plugin, &score);
    IOObjectRelease(service);
    if (kr != kIOReturnSuccess || plugin == NULL) {
        fprintf(stderr, "cannot create USB device plug-in: 0x%08x\n", kr);
        return NULL;
    }

    result = (*plugin)->QueryInterface(
        plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
        (LPVOID *)&device);
    (*plugin)->Release(plugin);
    if (result != 0 || device == NULL) {
        fprintf(stderr, "cannot create USB device interface: 0x%08x\n",
                (unsigned int)result);
        return NULL;
    }
    return device;
}

static IOUSBInterfaceInterface190 **copy_streaming_interface(
    IOUSBDeviceInterface **device, UInt8 *interfaceNumber)
{
    IOUSBFindInterfaceRequest request;
    io_iterator_t iterator;
    io_service_t service;
    IOCFPlugInInterface **plugin;
    IOUSBInterfaceInterface190 **interface;
    SInt32 score;
    HRESULT result;
    IOReturn kr;

    request.bInterfaceClass = UVC_VIDEO_CLASS;
    request.bInterfaceSubClass = UVC_STREAMING_SUBCLASS;
    request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

    iterator = IO_OBJECT_NULL;
    kr = (*device)->CreateInterfaceIterator(device, &request, &iterator);
    if (kr != kIOReturnSuccess) {
        fprintf(stderr, "cannot enumerate UVC streaming interfaces: 0x%08x\n",
                kr);
        return NULL;
    }

    service = IOIteratorNext(iterator);
    IOObjectRelease(iterator);
    if (service == IO_OBJECT_NULL) {
        fprintf(stderr, "UVC streaming interface was not found\n");
        return NULL;
    }

    plugin = NULL;
    interface = NULL;
    score = 0;
    kr = IOCreatePlugInInterfaceForService(
        service, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID,
        &plugin, &score);
    IOObjectRelease(service);
    if (kr != kIOReturnSuccess || plugin == NULL) {
        fprintf(stderr, "cannot create streaming-interface plug-in: 0x%08x\n",
                kr);
        return NULL;
    }

    result = (*plugin)->QueryInterface(
        plugin, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID190),
        (LPVOID *)&interface);
    (*plugin)->Release(plugin);
    if (result != 0 || interface == NULL) {
        fprintf(stderr, "cannot create IOUSBInterfaceInterface190: 0x%08x\n",
                (unsigned int)result);
        return NULL;
    }

    kr = (*interface)->GetInterfaceNumber(interface, interfaceNumber);
    if (kr != kIOReturnSuccess) {
        fprintf(stderr, "cannot read streaming interface number: 0x%08x\n", kr);
        (*interface)->Release(interface);
        return NULL;
    }

    kr = (*interface)->USBInterfaceOpen(interface);
    if (kr != kIOReturnSuccess) {
        fprintf(stderr,
                "cannot open streaming interface %u: 0x%08x\n"
                "close camera applications and retry; sudo may be required\n",
                (unsigned int)*interfaceNumber, kr);
        (*interface)->Release(interface);
        return NULL;
    }
    return interface;
}

static IOReturn uvc_control_request(IOUSBInterfaceInterface190 **interface,
                                    UInt8 interfaceNumber, UInt8 requestCode,
                                    UInt8 selector, uint8_t *bytes)
{
    IOUSBDevRequest request;
    memset(&request, 0, sizeof(request));
    request.bmRequestType = USBmakebmRequestType(
        requestCode == UVC_SET_CUR ? kUSBOut : kUSBIn,
        kUSBClass, kUSBInterface);
    request.bRequest = requestCode;
    request.wValue = (UInt16)((UInt16)selector << 8);
    request.wIndex = interfaceNumber;
    request.wLength = CE_UVC_STREAMING_CONTROL_SIZE;
    request.pData = bytes;
    return (*interface)->ControlRequest(interface, 0, &request);
}

static int get_and_print(IOUSBInterfaceInterface190 **interface,
                         UInt8 interfaceNumber, UInt8 requestCode,
                         const char *label, int required)
{
    uint8_t bytes[CE_UVC_STREAMING_CONTROL_SIZE];
    CEUVCStreamingControl control;
    IOReturn kr;

    memset(bytes, 0, sizeof(bytes));
    kr = uvc_control_request(interface, interfaceNumber, requestCode,
                             UVC_VS_PROBE_CONTROL, bytes);
    if (kr != kIOReturnSuccess) {
        fprintf(stderr, "%s failed: 0x%08x%s\n", label, kr,
                requestCode == UVC_GET_DEF ?
                " (expected on this non-compliant camera)" : "");
        return required ? 0 : 1;
    }
    if (!ce_uvc_decode(bytes, sizeof(bytes), &control)) {
        fprintf(stderr, "%s returned an invalid control block\n", label);
        return required ? 0 : 1;
    }
    ce_uvc_print(stdout, label, &control, bytes);
    return 1;
}

static int dump_controls(IOUSBInterfaceInterface190 **interface,
                         UInt8 interfaceNumber, int includeGetDef)
{
    int ok;
    ok = get_and_print(interface, interfaceNumber, UVC_GET_CUR,
                       "GET_CUR(PROBE)", 1);
    get_and_print(interface, interfaceNumber, UVC_GET_MIN,
                  "GET_MIN(PROBE)", 0);
    get_and_print(interface, interfaceNumber, UVC_GET_MAX,
                  "GET_MAX(PROBE)", 0);
    if (includeGetDef) {
        get_and_print(interface, interfaceNumber, UVC_GET_DEF,
                      "GET_DEF(PROBE)", 0);
    }
    return ok;
}

static int negotiate(IOUSBInterfaceInterface190 **interface,
                     UInt8 interfaceNumber, const CEUVCProfile *profile)
{
    uint8_t bytes[CE_UVC_STREAMING_CONTROL_SIZE];
    CEUVCStreamingControl control;
    IOReturn kr;

    kr = (*interface)->SetAlternateInterface(interface, 0);
    if (kr != kIOReturnSuccess) {
        fprintf(stderr, "cannot select streaming alternate setting 0: 0x%08x\n",
                kr);
        return 0;
    }

    memset(bytes, 0, sizeof(bytes));
    kr = uvc_control_request(interface, interfaceNumber, UVC_GET_CUR,
                             UVC_VS_PROBE_CONTROL, bytes);
    if (kr == kIOReturnSuccess) {
        ce_uvc_decode(bytes, sizeof(bytes), &control);
    } else {
        memset(&control, 0, sizeof(control));
        fprintf(stderr,
                "GET_CUR(PROBE) failed before negotiation: 0x%08x; using a clean control block\n",
                kr);
    }

    ce_uvc_apply_profile(&control, profile);
    ce_uvc_encode(&control, bytes, sizeof(bytes));
    ce_uvc_print(stdout, "SET_CUR(PROBE) requested", &control, bytes);
    kr = uvc_control_request(interface, interfaceNumber, UVC_SET_CUR,
                             UVC_VS_PROBE_CONTROL, bytes);
    if (kr != kIOReturnSuccess) {
        fprintf(stderr, "SET_CUR(PROBE) failed: 0x%08x\n", kr);
        return 0;
    }

    memset(bytes, 0, sizeof(bytes));
    kr = uvc_control_request(interface, interfaceNumber, UVC_GET_CUR,
                             UVC_VS_PROBE_CONTROL, bytes);
    if (kr != kIOReturnSuccess ||
        !ce_uvc_decode(bytes, sizeof(bytes), &control)) {
        fprintf(stderr, "GET_CUR(PROBE) failed after SET_CUR: 0x%08x\n", kr);
        return 0;
    }
    ce_uvc_print(stdout, "GET_CUR(PROBE) returned", &control, bytes);

    /* Repair values that old Linux UVC drivers also distrust on this camera. */
    ce_uvc_apply_profile(&control, profile);
    ce_uvc_encode(&control, bytes, sizeof(bytes));
    kr = uvc_control_request(interface, interfaceNumber, UVC_SET_CUR,
                             UVC_VS_PROBE_CONTROL, bytes);
    if (kr != kIOReturnSuccess) {
        fprintf(stderr, "final SET_CUR(PROBE) failed: 0x%08x\n", kr);
        return 0;
    }
    kr = uvc_control_request(interface, interfaceNumber, UVC_SET_CUR,
                             UVC_VS_COMMIT_CONTROL, bytes);
    if (kr != kIOReturnSuccess) {
        fprintf(stderr, "SET_CUR(COMMIT) failed: 0x%08x\n", kr);
        return 0;
    }

    ce_uvc_print(stdout, "SET_CUR(COMMIT) sent", &control, bytes);
    fprintf(stdout,
            "volatile negotiation complete; alternate setting remains 0\n");
    return 1;
}

int main(int argc, char **argv)
{
    Options options;
    IOUSBDeviceInterface **device;
    IOUSBInterfaceInterface190 **interface;
    UInt8 interfaceNumber;
    int ok;

    if (!parse_options(argc, argv, &options)) {
        usage(argv[0]);
        return 2;
    }

    fprintf(stdout, "target: SuYin Acer CrystalEye 064e:a101\n");
    device = copy_target_device();
    if (device == NULL) {
        fprintf(stderr, "target USB camera 064e:a101 was not found\n");
        return 1;
    }

    interfaceNumber = 0;
    interface = copy_streaming_interface(device, &interfaceNumber);
    (*device)->Release(device);
    if (interface == NULL) {
        return 1;
    }
    fprintf(stdout, "opened UVC streaming interface %u\n",
            (unsigned int)interfaceNumber);

    ok = 1;
    if (options.profile != NULL) {
        ok = negotiate(interface, interfaceNumber, options.profile);
    }
    if (ok && options.dump) {
        ok = dump_controls(interface, interfaceNumber,
                           options.includeGetDef);
    }

    (*interface)->USBInterfaceClose(interface);
    (*interface)->Release(interface);
    return ok ? 0 : 1;
}

