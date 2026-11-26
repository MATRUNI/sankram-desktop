#import "internal.h"
#include <string.h>

char* platform_path_get_special(const char *name)
{
    char *result = NULL;
    NSString *key = [NSString stringWithUTF8String:name];

    if ([key caseInsensitiveCompare:@"home"] == NSOrderedSame || [key caseInsensitiveCompare:@"user"] == NSOrderedSame)
    {
        result = strdup([NSHomeDirectory() UTF8String]);
    }
    else if ([key caseInsensitiveCompare:@"temp"] == NSOrderedSame || [key caseInsensitiveCompare:@"tmp"] == NSOrderedSame)
    {
        NSString *tmp = NSTemporaryDirectory();
        if ([tmp hasSuffix:@"/"]) tmp = [tmp substringToIndex:[tmp length] - 1];
        result = strdup([tmp UTF8String]);
    }
    else
    {
        NSSearchPathDirectory dir;
        BOOL found = NO;

        if ([key caseInsensitiveCompare:@"appdata"] == NSOrderedSame || [key caseInsensitiveCompare:@"localappdata"] == NSOrderedSame)
        {
            dir = NSApplicationSupportDirectory;
            found = YES;
        }
        else if ([key caseInsensitiveCompare:@"desktop"] == NSOrderedSame)
        {
            dir = NSDesktopDirectory;
            found = YES;
        }
        else if ([key caseInsensitiveCompare:@"documents"] == NSOrderedSame)
        {
            dir = NSDocumentDirectory;
            found = YES;
        }
        else if ([key caseInsensitiveCompare:@"downloads"] == NSOrderedSame)
        {
            dir = NSDownloadsDirectory;
            found = YES;
        }

        if (found)
        {
            NSArray *paths = NSSearchPathForDirectoriesInDomains(dir, NSUserDomainMask, YES);
            if ([paths count] > 0) result = strdup([[paths firstObject] UTF8String]);
        }
    }

    return result;
}

char* platform_fs_get_drives(void)
{
    static char result[4096];
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSFileManager *fm = [NSFileManager defaultManager];
        NSArray<NSURLResourceKey> *keys = @[
            NSURLVolumeTotalCapacityKey,
            NSURLVolumeAvailableCapacityForImportantUsageKey,
            NSURLVolumeIsRemovableKey
        ];
        NSArray<NSURL *> *urls = [fm mountedVolumeURLsIncludingResourceValuesForKeys:keys
                                                                             options:NSVolumeEnumerationSkipHiddenVolumes];

        NSMutableString *json = [NSMutableString stringWithString:@"["];
        BOOL first = YES;

        for (NSURL *url in urls)
        {
            NSNumber *total = nil;
            NSNumber *free = nil;
            NSNumber *isRemovable = nil;

            [url getResourceValue:&total forKey:NSURLVolumeTotalCapacityKey error:nil];
            [url getResourceValue:&free forKey:NSURLVolumeAvailableCapacityForImportantUsageKey error:nil];
            [url getResourceValue:&isRemovable forKey:NSURLVolumeIsRemovableKey error:nil];

            if (!first) [json appendString:@","];
            first = NO;

            NSString *path = [url path];
            NSString *type = [isRemovable boolValue] ? @"removable" : @"fixed";
            unsigned long long totalBytes = total ? [total unsignedLongLongValue] : 0;
            unsigned long long freeBytes = free ? [free unsignedLongLongValue] : 0;

            [json appendFormat:@"{\"mount\":\"%@\",\"type\":\"%@\",\"total\":%llu,\"free\":%llu}",
                               path, type, totalBytes, freeBytes];
        }

        [json appendString:@"]"];
        const char *utf8 = [json UTF8String];
        strncpy(result, utf8, sizeof(result) - 1);
        result[sizeof(result) - 1] = '\0';
    });
    return result;
}

