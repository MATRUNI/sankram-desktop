#import "internal.h"

void platform_window_minimize(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_active_window miniaturize:nil];
    });
}

void platform_window_toggle_maximize(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_active_window zoom:nil];
    });
}

void platform_window_close(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_active_window close];
    });
}

void platform_window_start_drag(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSEvent *event = [NSApp currentEvent];
        if (event && g_active_window) {
            [g_active_window performWindowDragWithEvent:event];
        }
    });
}

void platform_window_set_size(int width, int height)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!g_active_window) return;
        NSRect frame = [g_active_window frame];
        frame.size = NSMakeSize(width, height);
        [g_active_window setFrame:frame display:YES animate:YES];
    });
}

void platform_window_get_size(int *width, int *height)
{
    if (!g_active_window) { *width = 0; *height = 0; return; }
    NSRect frame = [g_active_window frame];
    *width = (int)frame.size.width;
    *height = (int)frame.size.height;
}

void platform_window_set_position(int x, int y)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!g_active_window) return;
        NSScreen *screen = [g_active_window screen] ?: [NSScreen mainScreen];
        CGFloat screenHeight = screen ? screen.frame.size.height : 1080;
        NSRect frame = [g_active_window frame];
        NSPoint origin = NSMakePoint(x, screenHeight - y - frame.size.height);
        [g_active_window setFrameOrigin:origin];
    });
}

void platform_window_get_position(int *x, int *y)
{
    if (!g_active_window) { *x = 0; *y = 0; return; }
    NSScreen *screen = [g_active_window screen] ?: [NSScreen mainScreen];
    CGFloat screenHeight = screen ? screen.frame.size.height : 1080;
    NSRect frame = [g_active_window frame];
    *x = (int)frame.origin.x;
    *y = (int)(screenHeight - frame.origin.y - frame.size.height);
}

void platform_window_center(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_active_window) [g_active_window center];
    });
}

void platform_window_set_always_on_top(bool always_on_top)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!g_active_window) return;
        [g_active_window setLevel:always_on_top ? NSFloatingWindowLevel : NSNormalWindowLevel];
    });
}

void platform_window_set_resizable(bool resizable)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!g_active_window) return;
        NSWindowStyleMask mask = [g_active_window styleMask];
        if (resizable) mask |= NSWindowStyleMaskResizable;
        else mask &= ~NSWindowStyleMaskResizable;
        [g_active_window setStyleMask:mask];
    });
}

void platform_window_set_min_size(int width, int height)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_active_window) [g_active_window setMinSize:NSMakeSize(width, height)];
    });
}

void platform_window_set_max_size(int width, int height)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_active_window) [g_active_window setMaxSize:NSMakeSize(width, height)];
    });
}

void platform_window_set_title(const char *title)
{
    if (!title) return;
    NSString *nsTitle = [NSString stringWithUTF8String:title];
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_active_window && nsTitle) [g_active_window setTitle:nsTitle];
    });
}

bool platform_window_is_maximized(void)
{
    if (!g_active_window) return false;
    return [g_active_window isZoomed];
}

bool platform_window_is_minimized(void)
{
    if (!g_active_window) return false;
    return [g_active_window isMiniaturized];
}

