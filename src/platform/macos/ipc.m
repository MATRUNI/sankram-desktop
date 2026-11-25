#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include "ipc.h"

// Concrete implementation of ipc_reply_t for macOS
struct ipc_reply 
{
    void (^replyHandler)(id _Nullable reply, NSString * _Nullable errorMessage);
};

void ipc_reply_send(ipc_reply_t *reply, const char *json_response) 
{
    if (!reply || !reply->replyHandler) return;
    NSString *respStr = json_response ? [NSString stringWithUTF8String:json_response] : @"";
    reply->replyHandler(respStr, nil);
}

void ipc_reply_error(ipc_reply_t *reply, const char *error_message) 
{
    if (!reply || !reply->replyHandler) return;
    NSString *errStr = error_message ? [NSString stringWithUTF8String:error_message] : @"Unknown error";
    reply->replyHandler(nil, errStr);
}

// macOS Script Message Handler conforming to WKScriptMessageHandlerWithReply
@interface VyomScriptMessageHandler : NSObject <WKScriptMessageHandlerWithReply>
@end

@implementation VyomScriptMessageHandler

- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message
                 replyHandler:(void (^)(id _Nullable reply, NSString * _Nullable errorMessage))replyHandler 
{
    if (![message.body isKindOfClass:[NSDictionary class]]) {
        replyHandler(nil, @"Invalid message payload");
        return;
    }

    NSDictionary *dict = (NSDictionary *)message.body;
    NSString *cmdStr = dict[@"cmd"];
    if (!cmdStr) {
        replyHandler(nil, @"Missing command");
        return;
    }

    const char *cmd = [cmdStr UTF8String];
    app_command_fn handler = lookup_command(cmd);

    if (!handler) {
        replyHandler(nil, @"Command not found");
        return;
    }

    // Convert args dictionary to JSON string for C handler
    id argsObj = dict[@"args"];
    NSString *argsJsonStr = @"{}";
    if (argsObj && [NSJSONSerialization isValidJSONObject:argsObj]) {
        NSError *err = nil;
        NSData *jsonData = [NSJSONSerialization dataWithJSONObject:argsObj options:0 error:&err];
        if (!err && jsonData) {
            argsJsonStr = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
        }
    }

    ipc_reply_t reply = {
        .replyHandler = replyHandler
    };

    handler([argsJsonStr UTF8String], &reply);
}

@end

void macos_ipc_init(WKUserContentController *controller) 
{
    VyomScriptMessageHandler *handler = [[VyomScriptMessageHandler alloc] init];
    [controller addScriptMessageHandlerWithReply:handler contentWorld:[WKContentWorld pageWorld] name:@"desktop"];
}