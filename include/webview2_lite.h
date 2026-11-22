#ifndef WEBVIEW2_LITE_H
#define WEBVIEW2_LITE_H

#include <windows.h>
#include <unknwn.h>

typedef struct EventRegistrationToken {
    __int64 value;
} EventRegistrationToken;

typedef struct ICoreWebView2 ICoreWebView2;
typedef struct ICoreWebView2Controller ICoreWebView2Controller;
typedef struct ICoreWebView2Environment ICoreWebView2Environment;
typedef struct ICoreWebView2Settings ICoreWebView2Settings;

// Settings interface
typedef struct ICoreWebView2SettingsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreWebView2Settings *This, REFIID riid, void **ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreWebView2Settings *This);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreWebView2Settings *This);
    HRESULT (STDMETHODCALLTYPE *get_IsScriptEnabled)(ICoreWebView2Settings *This, BOOL *isScriptEnabled);
    HRESULT (STDMETHODCALLTYPE *put_IsScriptEnabled)(ICoreWebView2Settings *This, BOOL isScriptEnabled);
    HRESULT (STDMETHODCALLTYPE *get_IsWebMessageEnabled)(ICoreWebView2Settings *This, BOOL *isWebMessageEnabled);
    HRESULT (STDMETHODCALLTYPE *put_IsWebMessageEnabled)(ICoreWebView2Settings *This, BOOL isWebMessageEnabled);
    HRESULT (STDMETHODCALLTYPE *get_AreDefaultScriptDialogsEnabled)(ICoreWebView2Settings *This, BOOL *areDefaultScriptDialogsEnabled);
    HRESULT (STDMETHODCALLTYPE *put_AreDefaultScriptDialogsEnabled)(ICoreWebView2Settings *This, BOOL areDefaultScriptDialogsEnabled);
    HRESULT (STDMETHODCALLTYPE *get_IsStatusBarEnabled)(ICoreWebView2Settings *This, BOOL *isStatusBarEnabled);
    HRESULT (STDMETHODCALLTYPE *put_IsStatusBarEnabled)(ICoreWebView2Settings *This, BOOL isStatusBarEnabled);
    HRESULT (STDMETHODCALLTYPE *get_AreDevToolsEnabled)(ICoreWebView2Settings *This, BOOL *areDevToolsEnabled);
    HRESULT (STDMETHODCALLTYPE *put_AreDevToolsEnabled)(ICoreWebView2Settings *This, BOOL areDevToolsEnabled);
    HRESULT (STDMETHODCALLTYPE *get_AreDefaultContextMenusEnabled)(ICoreWebView2Settings *This, BOOL *enabled);
    HRESULT (STDMETHODCALLTYPE *put_AreDefaultContextMenusEnabled)(ICoreWebView2Settings *This, BOOL enabled);
} ICoreWebView2SettingsVtbl;

struct ICoreWebView2Settings {
    ICoreWebView2SettingsVtbl *lpVtbl;
};

typedef struct ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
typedef struct ICoreWebView2CreateCoreWebView2ControllerCompletedHandler ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
typedef struct ICoreWebView2WebMessageReceivedEventHandler ICoreWebView2WebMessageReceivedEventHandler;
typedef struct ICoreWebView2WebMessageReceivedEventArgs ICoreWebView2WebMessageReceivedEventArgs;
typedef struct ICoreWebView2ExecuteScriptCompletedHandler ICoreWebView2ExecuteScriptCompletedHandler;

// 1. Environment Completed Handler
typedef struct ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *This, REFIID riid, void **ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *This);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *This);
    HRESULT (STDMETHODCALLTYPE *Invoke)(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *This, HRESULT result, ICoreWebView2Environment *env);
} ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl;

struct ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl *lpVtbl;
};

// 2. Controller Completed Handler
typedef struct ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *This, REFIID riid, void **ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *This);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *This);
    HRESULT (STDMETHODCALLTYPE *Invoke)(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *This, HRESULT result, ICoreWebView2Controller *controller);
} ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl;

struct ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {
    ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl *lpVtbl;
};

// 3. Environment
typedef struct ICoreWebView2EnvironmentVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreWebView2Environment *This, REFIID riid, void **ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreWebView2Environment *This);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreWebView2Environment *This);
    HRESULT (STDMETHODCALLTYPE *CreateCoreWebView2Controller)(ICoreWebView2Environment *This, HWND parentWindow, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler);
    HRESULT (STDMETHODCALLTYPE *CreateWebResourceResponse)(ICoreWebView2Environment *This, void *content, int statusCode, LPCWSTR reasonPhrase, LPCWSTR headers, void **response);
} ICoreWebView2EnvironmentVtbl;

struct ICoreWebView2Environment {
    ICoreWebView2EnvironmentVtbl *lpVtbl;
};

// 4. Controller
typedef struct ICoreWebView2ControllerVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreWebView2Controller *This, REFIID riid, void **ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreWebView2Controller *This);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreWebView2Controller *This);
    HRESULT (STDMETHODCALLTYPE *get_IsVisible)(ICoreWebView2Controller *This, BOOL *isVisible);
    HRESULT (STDMETHODCALLTYPE *put_IsVisible)(ICoreWebView2Controller *This, BOOL isVisible);
    HRESULT (STDMETHODCALLTYPE *get_Bounds)(ICoreWebView2Controller *This, RECT *bounds);
    HRESULT (STDMETHODCALLTYPE *put_Bounds)(ICoreWebView2Controller *This, RECT bounds);
    HRESULT (STDMETHODCALLTYPE *get_ZoomFactor)(ICoreWebView2Controller *This, double *zoomFactor);
    HRESULT (STDMETHODCALLTYPE *put_ZoomFactor)(ICoreWebView2Controller *This, double zoomFactor);
    HRESULT (STDMETHODCALLTYPE *add_ZoomFactorChanged)(ICoreWebView2Controller *This, void *eventHandler, EventRegistrationToken *token);
    HRESULT (STDMETHODCALLTYPE *remove_ZoomFactorChanged)(ICoreWebView2Controller *This, EventRegistrationToken token);
    HRESULT (STDMETHODCALLTYPE *SetBoundsAndZoomFactor)(ICoreWebView2Controller *This, RECT bounds, double zoomFactor);
    HRESULT (STDMETHODCALLTYPE *MoveFocus)(ICoreWebView2Controller *This, int reason);
    HRESULT (STDMETHODCALLTYPE *add_MoveFocusRequested)(ICoreWebView2Controller *This, void *eventHandler, EventRegistrationToken *token);
    HRESULT (STDMETHODCALLTYPE *remove_MoveFocusRequested)(ICoreWebView2Controller *This, EventRegistrationToken token);
    HRESULT (STDMETHODCALLTYPE *add_GotFocus)(ICoreWebView2Controller *This, void *eventHandler, EventRegistrationToken *token);
    HRESULT (STDMETHODCALLTYPE *remove_GotFocus)(ICoreWebView2Controller *This, EventRegistrationToken token);
    HRESULT (STDMETHODCALLTYPE *add_LostFocus)(ICoreWebView2Controller *This, void *eventHandler, EventRegistrationToken *token);
    HRESULT (STDMETHODCALLTYPE *remove_LostFocus)(ICoreWebView2Controller *This, EventRegistrationToken token);
    HRESULT (STDMETHODCALLTYPE *add_AcceleratorKeyPressed)(ICoreWebView2Controller *This, void *eventHandler, EventRegistrationToken *token);
    HRESULT (STDMETHODCALLTYPE *remove_AcceleratorKeyPressed)(ICoreWebView2Controller *This, EventRegistrationToken token);
    HRESULT (STDMETHODCALLTYPE *get_ParentWindow)(ICoreWebView2Controller *This, HWND *topLevelWindow);
    HRESULT (STDMETHODCALLTYPE *put_ParentWindow)(ICoreWebView2Controller *This, HWND topLevelWindow);
    HRESULT (STDMETHODCALLTYPE *NotifyParentWindowPositionChanged)(ICoreWebView2Controller *This);
    HRESULT (STDMETHODCALLTYPE *Close)(ICoreWebView2Controller *This);
    HRESULT (STDMETHODCALLTYPE *get_CoreWebView2)(ICoreWebView2Controller *This, ICoreWebView2 **coreWebView2);
} ICoreWebView2ControllerVtbl;

struct ICoreWebView2Controller {
    ICoreWebView2ControllerVtbl *lpVtbl;
};

// 5. WebMessageReceived Event Args
typedef struct ICoreWebView2WebMessageReceivedEventArgsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreWebView2WebMessageReceivedEventArgs *This, REFIID riid, void **ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreWebView2WebMessageReceivedEventArgs *This);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreWebView2WebMessageReceivedEventArgs *This);
    HRESULT (STDMETHODCALLTYPE *get_Source)(ICoreWebView2WebMessageReceivedEventArgs *This, LPWSTR *value);
    HRESULT (STDMETHODCALLTYPE *get_WebMessageAsJson)(ICoreWebView2WebMessageReceivedEventArgs *This, LPWSTR *value);
    HRESULT (STDMETHODCALLTYPE *TryGetWebMessageAsString)(ICoreWebView2WebMessageReceivedEventArgs *This, LPWSTR *value);
} ICoreWebView2WebMessageReceivedEventArgsVtbl;

struct ICoreWebView2WebMessageReceivedEventArgs {
    ICoreWebView2WebMessageReceivedEventArgsVtbl *lpVtbl;
};

// 6. WebMessageReceived Event Handler
typedef struct ICoreWebView2WebMessageReceivedEventHandlerVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreWebView2WebMessageReceivedEventHandler *This, REFIID riid, void **ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreWebView2WebMessageReceivedEventHandler *This);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreWebView2WebMessageReceivedEventHandler *This);
    HRESULT (STDMETHODCALLTYPE *Invoke)(ICoreWebView2WebMessageReceivedEventHandler *This, ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args);
} ICoreWebView2WebMessageReceivedEventHandlerVtbl;

struct ICoreWebView2WebMessageReceivedEventHandler {
    ICoreWebView2WebMessageReceivedEventHandlerVtbl *lpVtbl;
};

// 7. ExecuteScript Completed Handler
typedef struct ICoreWebView2ExecuteScriptCompletedHandlerVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreWebView2ExecuteScriptCompletedHandler *This, REFIID riid, void **ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreWebView2ExecuteScriptCompletedHandler *This);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreWebView2ExecuteScriptCompletedHandler *This);
    HRESULT (STDMETHODCALLTYPE *Invoke)(ICoreWebView2ExecuteScriptCompletedHandler *This, HRESULT errorCode, LPCWSTR resultObjectAsJson);
} ICoreWebView2ExecuteScriptCompletedHandlerVtbl;

struct ICoreWebView2ExecuteScriptCompletedHandler {
    ICoreWebView2ExecuteScriptCompletedHandlerVtbl *lpVtbl;
};

// 8. CoreWebView2
typedef struct ICoreWebView2Vtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreWebView2 *This, REFIID riid, void **ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreWebView2 *This);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreWebView2 *This);
    HRESULT (STDMETHODCALLTYPE *get_Settings)(ICoreWebView2 *This, void **settings);
    HRESULT (STDMETHODCALLTYPE *get_Source)(ICoreWebView2 *This, LPWSTR *uri);
    HRESULT (STDMETHODCALLTYPE *Navigate)(ICoreWebView2 *This, LPCWSTR uri);
    HRESULT (STDMETHODCALLTYPE *NavigateToString)(ICoreWebView2 *This, LPCWSTR htmlContent);
    void *add_NavigationStarting;
    void *remove_NavigationStarting;
    void *add_ContentLoading;
    void *remove_ContentLoading;
    void *add_SourceChanged;
    void *remove_SourceChanged;
    void *add_HistoryChanged;
    void *remove_HistoryChanged;
    void *add_NavigationCompleted;
    void *remove_NavigationCompleted;
    void *add_FrameNavigationStarting;
    void *remove_FrameNavigationStarting;
    void *add_FrameNavigationCompleted;
    void *remove_FrameNavigationCompleted;
    void *add_ScriptDialogOpening;
    void *remove_ScriptDialogOpening;
    void *add_PermissionRequested;
    void *remove_PermissionRequested;
    void *add_ProcessFailed;
    void *remove_ProcessFailed;
    HRESULT (STDMETHODCALLTYPE *AddScriptToExecuteOnDocumentCreated)(ICoreWebView2 *This, LPCWSTR javaScript, void *handler);
    void *remove_AddScriptToExecuteOnDocumentCreated;
    HRESULT (STDMETHODCALLTYPE *ExecuteScript)(ICoreWebView2 *This, LPCWSTR javaScript, ICoreWebView2ExecuteScriptCompletedHandler *handler);
    void *CapturePreview;
    void *Reload;
    HRESULT (STDMETHODCALLTYPE *PostWebMessageAsJson)(ICoreWebView2 *This, LPCWSTR webMessageAsJson);
    HRESULT (STDMETHODCALLTYPE *PostWebMessageAsString)(ICoreWebView2 *This, LPCWSTR webMessageAsString);
    HRESULT (STDMETHODCALLTYPE *add_WebMessageReceived)(ICoreWebView2 *This, ICoreWebView2WebMessageReceivedEventHandler *handler, EventRegistrationToken *token);
    HRESULT (STDMETHODCALLTYPE *remove_WebMessageReceived)(ICoreWebView2 *This, EventRegistrationToken token);
} ICoreWebView2Vtbl;

struct ICoreWebView2 {
    ICoreWebView2Vtbl *lpVtbl;
};

// 9. ICoreWebView2_3 for Virtual Host Mapping
typedef struct ICoreWebView2_3 ICoreWebView2_3;
typedef enum COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND {
    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY = 0,
    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW = 1,
    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS = 2
} COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND;

static const IID IID_ICoreWebView2_3 = { 0xa0d6df20, 0x3b92, 0x416d, { 0xaa, 0x0c, 0x43, 0x7a, 0x9c, 0x72, 0x78, 0x57 } };

typedef struct ICoreWebView2_3Vtbl {
    // IUnknown
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreWebView2_3 *This, REFIID riid, void **ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreWebView2_3 *This);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreWebView2_3 *This);
    // ICoreWebView2 base methods
    HRESULT (STDMETHODCALLTYPE *get_Settings)(ICoreWebView2_3 *This, void **settings);
    HRESULT (STDMETHODCALLTYPE *get_Source)(ICoreWebView2_3 *This, LPWSTR *uri);
    HRESULT (STDMETHODCALLTYPE *Navigate)(ICoreWebView2_3 *This, LPCWSTR uri);
    HRESULT (STDMETHODCALLTYPE *NavigateToString)(ICoreWebView2_3 *This, LPCWSTR htmlContent);
    void *add_NavigationStarting;
    void *remove_NavigationStarting;
    void *add_ContentLoading;
    void *remove_ContentLoading;
    void *add_SourceChanged;
    void *remove_SourceChanged;
    void *add_HistoryChanged;
    void *remove_HistoryChanged;
    void *add_NavigationCompleted;
    void *remove_NavigationCompleted;
    void *add_FrameNavigationStarting;
    void *remove_FrameNavigationStarting;
    void *add_FrameNavigationCompleted;
    void *remove_FrameNavigationCompleted;
    void *add_ScriptDialogOpening;
    void *remove_ScriptDialogOpening;
    void *add_PermissionRequested;
    void *remove_PermissionRequested;
    void *add_ProcessFailed;
    void *remove_ProcessFailed;
    HRESULT (STDMETHODCALLTYPE *AddScriptToExecuteOnDocumentCreated)(ICoreWebView2_3 *This, LPCWSTR javaScript, void *handler);
    void *remove_AddScriptToExecuteOnDocumentCreated;
    HRESULT (STDMETHODCALLTYPE *ExecuteScript)(ICoreWebView2_3 *This, LPCWSTR javaScript, ICoreWebView2ExecuteScriptCompletedHandler *handler);
    void *CapturePreview;
    void *Reload;
    HRESULT (STDMETHODCALLTYPE *PostWebMessageAsJson)(ICoreWebView2_3 *This, LPCWSTR webMessageAsJson);
    HRESULT (STDMETHODCALLTYPE *PostWebMessageAsString)(ICoreWebView2_3 *This, LPCWSTR webMessageAsString);
    HRESULT (STDMETHODCALLTYPE *add_WebMessageReceived)(ICoreWebView2_3 *This, ICoreWebView2WebMessageReceivedEventHandler *handler, EventRegistrationToken *token);
    HRESULT (STDMETHODCALLTYPE *remove_WebMessageReceived)(ICoreWebView2_3 *This, EventRegistrationToken token);
    void *CallDevToolsProtocolMethod;
    void *get_BrowserProcessId;
    void *get_CanGoBack;
    void *get_CanGoForward;
    void *GoBack;
    void *GoForward;
    void *GetDevToolsProtocolEventReceiver;
    void *Stop;
    void *add_NewWindowRequested;
    void *remove_NewWindowRequested;
    void *add_DocumentTitleChanged;
    void *remove_DocumentTitleChanged;
    void *get_DocumentTitle;
    void *AddHostObjectToScript;
    void *RemoveHostObjectFromScript;
    void *OpenDevToolsWindow;
    void *add_ContainsFullScreenElementChanged;
    void *remove_ContainsFullScreenElementChanged;
    void *get_ContainsFullScreenElement;
    void *add_WebResourceRequested;
    void *remove_WebResourceRequested;
    void *AddWebResourceRequestedFilter;
    void *RemoveWebResourceRequestedFilter;
    void *add_WindowCloseRequested;
    void *remove_WindowCloseRequested;
    // ICoreWebView2_2 methods
    void *add_WebResourceResponseReceived;
    void *remove_WebResourceResponseReceived;
    void *NavigateWithWebResourceRequest;
    void *add_DOMContentLoaded;
    void *remove_DOMContentLoaded;
    void *get_CookieManager;
    void *get_Environment;
    // ICoreWebView2_3 methods
    void *TrySuspend;
    void *Resume;
    void *get_IsSuspended;
    HRESULT (STDMETHODCALLTYPE *SetVirtualHostNameToFolderMapping)(ICoreWebView2_3 *This, LPCWSTR hostName, LPCWSTR folderPath, COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND accessKind);
    HRESULT (STDMETHODCALLTYPE *ClearVirtualHostNameToFolderMapping)(ICoreWebView2_3 *This, LPCWSTR hostName);
} ICoreWebView2_3Vtbl;

struct ICoreWebView2_3 {
    ICoreWebView2_3Vtbl *lpVtbl;
};

// 10. Function pointer for CreateCoreWebView2EnvironmentWithOptions
typedef HRESULT (STDAPICALLTYPE *CreateCoreWebView2EnvironmentWithOptionsFn)(
    PCWSTR browserExecutableFolder,
    PCWSTR userDataFolder,
    void *environmentOptions,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *environmentCreatedHandler
);

#endif
