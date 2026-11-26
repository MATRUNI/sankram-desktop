#import "internal.h"

void platform_clipboard_write_text(const char *text)
{
    if (!text) return;
    dispatch_async(dispatch_get_main_queue(), ^{
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        [pasteboard setString:[NSString stringWithUTF8String:text] forType:NSPasteboardTypeString];
    });
}

bool platform_clipboard_has_text(void)
{
    __block BOOL hasText = NO;
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        hasText = ([pasteboard stringForType:NSPasteboardTypeString] != nil);
    });
    return hasText;
}

char* platform_clipboard_read_text(void)
{
    __block char *result = NULL;
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        NSString *str = [pasteboard stringForType:NSPasteboardTypeString];
        if (str) {
            result = strdup([str UTF8String]);
        }
    });
    return result;
}

void platform_clipboard_clear(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
    });
}

