#import "internal.h"
#include <stdio.h>

static NSStatusItem *g_status_item = nil;

@interface VyomTrayTarget : NSObject
@end

@implementation VyomTrayTarget

- (void)onMenuItemClick:(NSMenuItem *)sender {
    NSString *itemId = sender.representedObject;
    if (itemId) {
        char payload[128];
        snprintf(payload, sizeof(payload), "{\"id\":\"%s\"}", [itemId UTF8String]);
        platform_app_emit("tray:menu_click", payload);
    }
}

@end

static VyomTrayTarget *g_tray_target = nil;

void platform_notification_send(const char *title, const char *body, const char *icon)
{
    (void)icon;
    NSString *t = [NSString stringWithUTF8String:(title && *title) ? title : "Vyom"];
    NSString *b = [NSString stringWithUTF8String:body ? body : ""];

    dispatch_async(dispatch_get_main_queue(), ^{
        NSTask *task = [[NSTask alloc] init];
        task.launchPath = @"/usr/bin/osascript";
        NSString *escT = [t stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
        NSString *escB = [b stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
        NSString *script = [NSString stringWithFormat:@"display notification \"%@\" with title \"%@\"", escB, escT];
        task.arguments = @[@"-e", script];
        [task launch];
    });
}

void platform_tray_create(const char *tooltip, const char *icon)
{
    (void)icon;
    NSString *tip = [NSString stringWithUTF8String:(tooltip && *tooltip) ? tooltip : "Vyom"];
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!g_tray_target) g_tray_target = [[VyomTrayTarget alloc] init];
        if (!g_status_item) {
            g_status_item = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
            g_status_item.button.title = @"V";
        }
        g_status_item.button.toolTip = tip;
    });
}

void platform_tray_set_tooltip(const char *tooltip)
{
    if (!tooltip) return;
    NSString *tip = [NSString stringWithUTF8String:tooltip];
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_status_item) g_status_item.button.toolTip = tip;
    });
}

void platform_tray_set_menu(const tray_menu_item_t *items, int count)
{
    NSMutableArray *copied = [NSMutableArray arrayWithCapacity:(NSUInteger)count];
    for (int i = 0; i < count; i++) {
        [copied addObject:@{
            @"id": [NSString stringWithUTF8String:items[i].id],
            @"label": [NSString stringWithUTF8String:items[i].label],
            @"separator": @(items[i].separator)
        }];
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!g_status_item) return;
        if (!g_tray_target) g_tray_target = [[VyomTrayTarget alloc] init];

        NSMenu *menu = [[NSMenu alloc] init];
        for (NSDictionary *entry in copied) {
            if ([entry[@"separator"] boolValue]) {
                [menu addItem:[NSMenuItem separatorItem]];
            } else {
                NSMenuItem *mi = [[NSMenuItem alloc] initWithTitle:entry[@"label"]
                                                            action:@selector(onMenuItemClick:)
                                                     keyEquivalent:@""];
                mi.target = g_tray_target;
                mi.representedObject = entry[@"id"];
                [menu addItem:mi];
            }
        }
        g_status_item.menu = menu;
    });
}

void platform_tray_destroy(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_status_item) {
            [[NSStatusBar systemStatusBar] removeStatusItem:g_status_item];
            g_status_item = nil;
        }
    });
}

