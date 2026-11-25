#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include <sys/stat.h>
#include "app.h"
#include "bridge.h"
#include "platform.h"
#include "memi.h"

// Forward declaration from ipc.m
void macos_ipc_init(WKUserContentController *controller);

static NSWindow *g_active_window = nil;
static WKWebView *g_active_webview = nil;

// -----------------------------------------------------------------------------
// 1. CUSTOM URI SCHEME HANDLER (app://localhost/ -> ./dist/)
// -----------------------------------------------------------------------------
@interface VyomSchemeHandler : NSObject <WKURLSchemeHandler>
@property (nonatomic, copy) NSString *rootDir;
@end

@implementation VyomSchemeHandler

- (instancetype)initWithRootDir:(NSString *)dir {
    self = [super init];
    if (self) {
        _rootDir = [dir copy];
    }
    return self;
}

- (void)webView:(WKWebView *)webView startURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask {
    NSURL *url = urlSchemeTask.request.URL;
    NSString *path = url.path;

    if (!path || [path isEqualToString:@"/"] || [path length] == 0) {
        path = @"/index.html";
    }

    NSString *fullPath = [self.rootDir stringByAppendingPathComponent:path];
    NSData *data = [NSData dataWithContentsOfFile:fullPath];

    // Fallback to index.html for SPA routing
    if (!data) {
        fullPath = [self.rootDir stringByAppendingPathComponent:@"index.html"];
        data = [NSData dataWithContentsOfFile:fullPath];
    }

    if (!data) {
        NSError *err = [NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorResourceUnavailable userInfo:nil];
        [urlSchemeTask didFailWithError:err];
        return;
    }

    const char *mime = get_memi_type([fullPath UTF8String]);
    NSString *mimeStr = [NSString stringWithUTF8String:mime];

    NSHTTPURLResponse *response = [[NSHTTPURLResponse alloc] initWithURL:url
                                                              statusCode:200
                                                             HTTPVersion:@"HTTP/1.1"
                                                            headerFields:@{
                                                                @"Content-Type": mimeStr,
                                                                @"Access-Control-Allow-Origin": @"*",
                                                                @"Content-Length": [NSString stringWithFormat:@"%lu", (unsigned long)[data length]]
                                                            }];

    [urlSchemeTask didReceiveResponse:response];
    [urlSchemeTask didReceiveData:data];
    [urlSchemeTask didFinish];
}

- (void)webView:(WKWebView *)webView stopURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask {
    // Task cancelled
}

@end

// -----------------------------------------------------------------------------
// 2. WINDOW DELEGATE & APPLICATION LIFECYCLE
// -----------------------------------------------------------------------------
@interface VyomAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property (nonatomic, assign) const app_config_t *config;
@end

@implementation VyomAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Transform process into a regular GUI foreground app with Dock icon
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Main App Menu bar (Quit Vyom shortcut: Cmd+Q)
    NSMenu *menubar = [[NSMenu alloc] init];
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];

    NSMenu *appMenu = [[NSMenu alloc] init];
    NSString *appName = [[NSProcessInfo processInfo] processName];
    NSString *quitTitle = [NSString stringWithFormat:@"Quit %@", appName];
    NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle
                                                          action:@selector(terminate:)
                                                   keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];

    // Window style configuration
    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled |
                                  NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable;

    if (self.config->window->resizable) {
        styleMask |= NSWindowStyleMaskResizable;
    }

    // Frameless mode: hides standard titlebar while preserving macOS traffic lights
    if (!self.config->window->decorations) {
        styleMask |= NSWindowStyleMaskFullSizeContentView;
    }

    CGFloat w = self.config->window->width > 0 ? self.config->window->width : 1000;
    CGFloat h = self.config->window->height > 0 ? self.config->window->height : 700;
    NSRect frame = NSMakeRect(0, 0, w, h);

    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:styleMask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];

    g_active_window = window;
    window.delegate = self;
    window.title = [NSString stringWithUTF8String:self.config->window->title ? self.config->window->title : "Vyom"];

    if (!self.config->window->decorations) {
        window.titlebarAppearsTransparent = YES;
        window.titleVisibility = NSWindowTitleHidden;
    }

    if (self.config->window->min_width > 0 && self.config->window->min_height > 0) {
        window.minSize = NSMakeSize(self.config->window->min_width, self.config->window->min_height);
    }

    [window center];

    // Configure WebKit
    WKWebViewConfiguration *webConfig = [[WKWebViewConfiguration alloc] init];
    
    // Register custom scheme "app"
    VyomSchemeHandler *schemeHandler = [[VyomSchemeHandler alloc] initWithRootDir:@"./dist"];
    [webConfig setURLSchemeHandler:schemeHandler forURLScheme:@"app"];

    // Initialize IPC bridge
    macos_ipc_init(webConfig.userContentController);

    // Inject bridge.js script into webview
    NSString *bridgeSource = [NSString stringWithUTF8String:bridge_js];
    WKUserScript *script = [[WKUserScript alloc] initWithSource:bridgeSource
                                                  injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                               forMainFrameOnly:NO];
    [webConfig.userContentController addUserScript:script];

    // Developer Tools
    if (self.config->webview->developer_extras) {
        [webConfig.preferences setValue:@YES forKey:@"developerExtrasEnabled"];
    }

    WKWebView *webView = [[WKWebView alloc] initWithFrame:window.contentView.bounds configuration:webConfig];
    g_active_webview = webView;
    webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [window.contentView addSubview:webView];

    // Load initial URL
    NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:self.config->webview->url ? self.config->webview->url : "app://localhost/index.html"]];
    [webView loadRequest:[NSURLRequest requestWithURL:url]];

    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

// -----------------------------------------------------------------------------
// 3. PLATFORM RUNNER & WINDOW CONTROLS
// -----------------------------------------------------------------------------
int platform_app_run(const app_config_t *config, int argc, char *argv[]) 
{
    (void)argc; (void)argv;
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        VyomAppDelegate *delegate = [[VyomAppDelegate alloc] init];
        delegate.config = config;
        app.delegate = delegate;
        [app run];
    }
    return 0;
}

void platform_app_emit(const char *event, const char *json_data) 
{
    if (!g_active_webview || !event) return;
    const char *payload = (json_data && *json_data) ? json_data : "null";
    NSString *js = [NSString stringWithFormat:@"window.vyom._emit(\"%s\", %s);", event, payload];
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_active_webview evaluateJavaScript:js completionHandler:nil];
    });
}

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
        // In Cocoa, Y=0 is bottom of screen. Translate from top-left:
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

// -----------------------------------------------------------------------------
// 4. NATIVE DIALOGS (Cocoa NSOpenPanel, NSSavePanel, NSAlert)
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 5. NATIVE CLIPBOARD (NSPasteboard)
// -----------------------------------------------------------------------------
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