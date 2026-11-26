#import "internal.h"

char* platform_dialog_open_file(const char *title, const char *filter)
{
    (void)filter;
    __block char *resultPath = NULL;
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        if (title && *title) panel.title = [NSString stringWithUTF8String:title];

        if ([panel runModal] == NSModalResponseOK) {
            NSURL *url = panel.URL;
            if (url) {
                const char *path = [url.path UTF8String];
                resultPath = strdup(path);
            }
        }
    });
    return resultPath;
}

char* platform_dialog_save_file(const char *title, const char *filter, const char *default_name)
{
    (void)filter;
    __block char *resultPath = NULL;
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSSavePanel *panel = [NSSavePanel savePanel];
        if (title && *title) panel.title = [NSString stringWithUTF8String:title];
        if (default_name && *default_name) panel.nameFieldStringValue = [NSString stringWithUTF8String:default_name];

        if ([panel runModal] == NSModalResponseOK) {
            NSURL *url = panel.URL;
            if (url) {
                const char *path = [url.path UTF8String];
                resultPath = strdup(path);
            }
        }
    });
    return resultPath;
}

char* platform_dialog_open_directory(const char *title)
{
    __block char *resultPath = NULL;
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = NO;
        panel.canChooseDirectories = YES;
        panel.allowsMultipleSelection = NO;
        if (title && *title) panel.title = [NSString stringWithUTF8String:title];

        if ([panel runModal] == NSModalResponseOK) {
            NSURL *url = panel.URL;
            if (url) {
                const char *path = [url.path UTF8String];
                resultPath = strdup(path);
            }
        }
    });
    return resultPath;
}

void platform_dialog_message(const char *title, const char *message, const char *type)
{
    (void)type;
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = [NSString stringWithUTF8String:title ? title : "Alert"];
        if (message && *message) {
            alert.informativeText = [NSString stringWithUTF8String:message];
        }
        [alert runModal];
    });
}

bool platform_dialog_confirm(const char *title, const char *message)
{
    __block BOOL confirmed = NO;
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = [NSString stringWithUTF8String:title ? title : "Confirm"];
        if (message && *message) {
            alert.informativeText = [NSString stringWithUTF8String:message];
        }
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Cancel"];

        NSModalResponse response = [alert runModal];
        confirmed = (response == NSAlertFirstButtonReturn);
    });
    return confirmed;
}

