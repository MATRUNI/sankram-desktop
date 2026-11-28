#import "internal.h"
#include <sys/stat.h>
#include "bridge.h"
#include "memi.h"

NSWindow *g_active_window = nil;
WKWebView *g_active_webview = nil;

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
    (void)webView;
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
    (void)webView;
    (void)urlSchemeTask;
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
    (void)notification;
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    NSImage *appIcon = [[NSImage alloc] initWithContentsOfFile:@"assets/icon.png"];
    if (appIcon) [NSApp setApplicationIconImage:appIcon];

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

    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled |
                                  NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable;

    if (self.config->window->resizable) {
        styleMask |= NSWindowStyleMaskResizable;
    }
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

    WKWebViewConfiguration *webConfig = [[WKWebViewConfiguration alloc] init];
    VyomSchemeHandler *schemeHandler = [[VyomSchemeHandler alloc] initWithRootDir:@"./dist"];
    [webConfig setURLSchemeHandler:schemeHandler forURLScheme:@"app"];

    macos_ipc_init(webConfig.userContentController);

    NSString *bridgeSource = [NSString stringWithUTF8String:bridge_js];
    WKUserScript *script = [[WKUserScript alloc] initWithSource:bridgeSource
                                                  injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                               forMainFrameOnly:NO];
    [webConfig.userContentController addUserScript:script];

    if (self.config->webview->developer_extras) {
        [webConfig.preferences setValue:@YES forKey:@"developerExtrasEnabled"];
    }

    WKWebView *webView = [[WKWebView alloc] initWithFrame:window.contentView.bounds configuration:webConfig];
    g_active_webview = webView;
    webView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [window.contentView addSubview:webView];

    NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:self.config->webview->url ? self.config->webview->url : "app://localhost/index.html"]];
    [webView loadRequest:[NSURLRequest requestWithURL:url]];

    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    (void)sender;
    return YES;
}

@end

// -----------------------------------------------------------------------------
// 3. PLATFORM RUNNER & EVENT EMITTER
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
