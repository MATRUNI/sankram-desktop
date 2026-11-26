#ifndef MACOS_INTERNAL_H
#define MACOS_INTERNAL_H

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "platform.h"

extern NSWindow *g_active_window;
extern WKWebView *g_active_webview;

void macos_ipc_init(WKUserContentController *controller);

#endif

