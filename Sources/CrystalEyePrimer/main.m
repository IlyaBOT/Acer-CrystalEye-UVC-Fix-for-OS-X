#import <Foundation/Foundation.h>
#import <QTKit/QTKit.h>
#import <CoreVideo/CoreVideo.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/usb/IOUSBLib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CRYSTALEYE_VENDOR_ID  0x064e
#define CRYSTALEYE_PRODUCT_ID 0xa101

@interface CEFrameCounter : NSObject {
    NSUInteger frameCount;
    BOOL printedFirstFrame;
}
- (NSUInteger)frameCount;
@end

@implementation CEFrameCounter
- (NSUInteger)frameCount
{
    @synchronized(self) {
        return frameCount;
    }
}

- (void)captureOutput:(QTCaptureOutput *)captureOutput
  didOutputVideoFrame:(CVImageBufferRef)videoFrame
     withSampleBuffer:(QTSampleBuffer *)sampleBuffer
       fromConnection:(QTCaptureConnection *)connection
{
    size_t width;
    size_t height;
    (void)captureOutput;
    (void)sampleBuffer;
    (void)connection;

    @synchronized(self) {
        ++frameCount;
        if (!printedFirstFrame) {
            width = CVPixelBufferGetWidth(videoFrame);
            height = CVPixelBufferGetHeight(videoFrame);
            fprintf(stdout, "first frame: %lux%lu\n",
                    (unsigned long)width, (unsigned long)height);
            fflush(stdout);
            printedFirstFrame = YES;
        }
    }
}
@end

static UInt32 target_location_id(void)
{
    CFMutableDictionaryRef matching;
    CFNumberRef number;
    CFTypeRef locationValue;
    io_service_t service;
    SInt32 vendor;
    SInt32 product;
    SInt32 location;

    matching = IOServiceMatching(kIOUSBDeviceClassName);
    if (matching == NULL) {
        return 0;
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
        return 0;
    }

    location = 0;
    locationValue = IORegistryEntryCreateCFProperty(
        service, CFSTR("locationID"), kCFAllocatorDefault, 0);
    IOObjectRelease(service);
    if (locationValue == NULL || CFGetTypeID(locationValue) != CFNumberGetTypeID()) {
        if (locationValue != NULL) {
            CFRelease(locationValue);
        }
        return 0;
    }
    CFNumberGetValue((CFNumberRef)locationValue, kCFNumberSInt32Type, &location);
    CFRelease(locationValue);
    return (UInt32)location;
}

static UInt32 location_from_unique_id(NSString *uniqueID)
{
    unsigned int location;
    if (uniqueID == nil ||
        sscanf([uniqueID UTF8String], "0x%8x", &location) != 1) {
        return 0;
    }
    return (UInt32)location;
}

static QTCaptureDevice *copy_target_capture_device(UInt32 locationID)
{
    NSArray *devices;
    NSEnumerator *enumerator;
    QTCaptureDevice *device;

    devices = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
    enumerator = [devices objectEnumerator];
    while ((device = [enumerator nextObject]) != nil) {
        if (location_from_unique_id([device uniqueID]) == locationID) {
            return [device retain];
        }
    }
    return nil;
}

static void usage(const char *program)
{
    fprintf(stderr,
            "usage: %s [--width 320] [--height 240] [--seconds 10]\n",
            program);
}

static const char *error_text(NSError *error)
{
    return error == nil ? "unknown QTKit error" :
           [[error description] UTF8String];
}

int main(int argc, char **argv)
{
    NSAutoreleasePool *pool;
    UInt32 locationID;
    QTCaptureDevice *device;
    QTCaptureDeviceInput *input;
    QTCaptureDecompressedVideoOutput *output;
    QTCaptureSession *session;
    CEFrameCounter *counter;
    NSDictionary *attributes;
    NSError *error;
    NSDate *deadline;
    int width;
    int height;
    int seconds;
    int i;
    int result;

    width = 320;
    height = 240;
    seconds = 10;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            seconds = atoi(argv[++i]);
        } else {
            usage(argv[0]);
            return 2;
        }
    }
    if (width <= 0 || height <= 0 || seconds <= 0 || seconds > 120) {
        usage(argv[0]);
        return 2;
    }

    pool = [[NSAutoreleasePool alloc] init];
    locationID = target_location_id();
    if (locationID == 0) {
        fprintf(stderr, "target USB camera 064e:a101 was not found\n");
        [pool drain];
        return 1;
    }

    device = copy_target_capture_device(locationID);
    if (device == nil) {
        fprintf(stderr,
                "QTKit did not expose the 064e:a101 device at location 0x%08x\n",
                (unsigned int)locationID);
        [pool drain];
        return 1;
    }

    fprintf(stdout,
            "target: %s, location 0x%08x, requested output %dx%d for %d s\n",
            [[device localizedDisplayName] UTF8String],
            (unsigned int)locationID, width, height, seconds);
    fflush(stdout);

    error = nil;
    if (![device open:&error]) {
        fprintf(stderr, "cannot open QTKit camera: %s\n", error_text(error));
        [device release];
        [pool drain];
        return 1;
    }

    input = [[QTCaptureDeviceInput alloc] initWithDevice:device];
    output = [[QTCaptureDecompressedVideoOutput alloc] init];
    session = [[QTCaptureSession alloc] init];
    counter = [[CEFrameCounter alloc] init];

    attributes = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithInt:width], (id)kCVPixelBufferWidthKey,
        [NSNumber numberWithInt:height], (id)kCVPixelBufferHeightKey,
        nil];
    [output setPixelBufferAttributes:attributes];
    [output setAutomaticallyDropsLateVideoFrames:YES];
    [output setDelegate:counter];

    error = nil;
    if (![session addInput:input error:&error]) {
        fprintf(stderr, "cannot add QTKit input: %s\n", error_text(error));
        result = 1;
        goto cleanup;
    }
    error = nil;
    if (![session addOutput:output error:&error]) {
        fprintf(stderr, "cannot add QTKit output: %s\n", error_text(error));
        result = 1;
        goto cleanup;
    }

    [session startRunning];
    deadline = [NSDate dateWithTimeIntervalSinceNow:(NSTimeInterval)seconds];
    while ([deadline timeIntervalSinceNow] > 0.0) {
        NSAutoreleasePool *iterationPool;
        iterationPool = [[NSAutoreleasePool alloc] init];
        [[NSRunLoop currentRunLoop]
            runMode:NSDefaultRunLoopMode
            beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
        [iterationPool drain];
    }
    [session stopRunning];

    fprintf(stdout, "frames received: %lu\n",
            (unsigned long)[counter frameCount]);
    fprintf(stdout, "%s\n", [counter frameCount] > 0 ?
            "PRIMER RESULT: video stream started" :
            "PRIMER RESULT: no frames before timeout");
    result = [counter frameCount] > 0 ? 0 : 1;

cleanup:
    [output setDelegate:nil];
    [device close];
    [counter release];
    [session release];
    [output release];
    [input release];
    [device release];
    [pool drain];
    return result;
}
