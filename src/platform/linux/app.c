#include <stdio.h>
#include "bridge.h"
#include "platform.h"

#include <sys/stat.h>
#include "memi.h"

#include "ipc.h"

static GtkWidget *g_active_webview = NULL;
static GtkWidget *g_active_window = NULL;

static void on_uri_scheme_request(WebKitURISchemeRequest *request, gpointer user_data) 
{
    const char *root_dir = (const char *)user_data; // "./dist"
    const char *path = webkit_uri_scheme_request_get_path(request); // e.g. "/assets/main.js"
    if (!path || strcmp(path, "/") == 0 || *path == '\0') {
        path = "/index.html";
    }
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", root_dir, path);

    GFile *file = g_file_new_for_path(full_path);
    GFileInputStream *stream = g_file_read(file, NULL, NULL);

    if (!stream) 
    {
        g_object_unref(file);
        snprintf(full_path,sizeof(full_path),"%s/index.html",root_dir);
        file = g_file_new_for_path(full_path);
        stream = g_file_read(file,NULL,NULL);
        if (!stream) 
        {
            GError *err = g_error_new(G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "Not Found");
            webkit_uri_scheme_request_finish_error(request, err);
            g_error_free(err);
            g_object_unref(file);
            return;
        }
    }

    struct stat st;
    stat(full_path, &st);
    gint64 total_size = (gint64)st.st_size;
    const char *mime = get_memi_type(full_path);

    // Check for HTTP Range header (crucial for 4K/8K video streaming & seeking)
    SoupMessageHeaders *req_headers = webkit_uri_scheme_request_get_http_headers(request);
    const char *range_header = req_headers ? soup_message_headers_get_one(req_headers, "Range") : NULL;
    if (range_header && g_str_has_prefix(range_header, "bytes=")) 
    {
        gint64 start = 0, end = total_size - 1;
        const char *range_val = range_header + 6; // skip "bytes="
        char **parts = g_strsplit(range_val, "-", 2);
        if (parts[0] && parts[0][0] != '\0') {
            start = g_ascii_strtoll(parts[0], NULL, 10);
        }
        if (parts[1] && parts[1][0] != '\0') {
            end = g_ascii_strtoll(parts[1], NULL, 10);
        }
        g_strfreev(parts);
        if (start >= total_size) {
            GError *err = g_error_new(G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Requested range not satisfiable");
            webkit_uri_scheme_request_finish_error(request, err);
            g_error_free(err);
            g_object_unref(stream);
            g_object_unref(file);
            return;
        }
        if (end >= total_size) {
            end = total_size - 1;
        }
        gint64 content_length = (end - start) + 1;
        // Seek file stream to the requested byte position
        if (G_IS_SEEKABLE(stream)) {
            g_seekable_seek(G_SEEKABLE(stream), start, G_SEEK_SET, NULL, NULL);
        }
        // WebKit 206 Partial Content Response
        WebKitURISchemeResponse *response = webkit_uri_scheme_response_new(G_INPUT_STREAM(stream), content_length);
        webkit_uri_scheme_response_set_status(response, 206, "Partial Content");
        webkit_uri_scheme_response_set_content_type(response, mime);
        SoupMessageHeaders *resp_headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
        char content_range[128];
        snprintf(content_range, sizeof(content_range), "bytes %" G_GINT64_FORMAT "-%" G_GINT64_FORMAT "/%" G_GINT64_FORMAT,
                 start, end, total_size);
        soup_message_headers_append(resp_headers, "Accept-Ranges", "bytes");
        soup_message_headers_append(resp_headers, "Content-Range", content_range);
        // WebKit adopts resp_headers (do NOT call soup_message_headers_unref!)
        webkit_uri_scheme_response_set_http_headers(response, resp_headers);
        webkit_uri_scheme_request_finish_with_response(request, response);
        g_object_unref(response);
    }  
    else 
    {
        webkit_uri_scheme_request_finish(request, G_INPUT_STREAM(stream), total_size, mime);
    }

    g_object_unref(stream);
    g_object_unref(file);
}

static void activate_callback(GtkApplication *app,gpointer user_data)
{
    app_config_t *con =(app_config_t *) user_data;

    // -------------------------------------------------------------------------
    // 1. GTK4 WINDOW INITIALIZATION (Title & Dimensions)
    // -------------------------------------------------------------------------
    GtkWidget *window = gtk_application_window_new(app);
    g_active_window = window;
    gtk_window_set_title(GTK_WINDOW(window), con->window->title);
    gtk_window_set_default_size(GTK_WINDOW(window), con->window->width, con->window->height);

    WebKitWebContext *context = webkit_web_context_get_default();
    WebKitSecurityManager *sm = webkit_web_context_get_security_manager(context);
    webkit_security_manager_register_uri_scheme_as_local(sm, "app"); 
    webkit_security_manager_register_uri_scheme_as_cors_enabled(sm, "app");
    webkit_security_manager_register_uri_scheme_as_secure(sm, "app");
    webkit_web_context_register_uri_scheme(context, "app", on_uri_scheme_request, "./dist", NULL);
    
    GtkWidget *webview = webkit_web_view_new();
    g_active_webview = webview;

    gtk_widget_set_hexpand(webview, TRUE);
    gtk_widget_set_vexpand(webview, TRUE);

    // -------------------------------------------------------------------------
    // 2. WEBKIT BROWSER SETTINGS (Developer Tools & Security)
    // -------------------------------------------------------------------------
    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
    webkit_settings_set_disable_web_security(settings, con->webview->disable_web_security);
    webkit_settings_set_enable_media(settings, TRUE);
    webkit_settings_set_media_playback_requires_user_gesture(settings, FALSE);
    webkit_settings_set_media_playback_allows_inline(settings, TRUE);
    if (con->webview->developer_extras)
    {
        // console: Open console by default
        webkit_settings_set_enable_developer_extras(settings, con->webview->developer_extras);
        // developer_extras: Show WebKit Web Inspector
        WebKitWebInspector *inspector = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(webview));
        webkit_web_inspector_show(inspector);
    }

    // -------------------------------------------------------------------------
    // 3. WINDOW BEHAVIOR & CONSTRAINTS (Decorations, Resizing, Min Size)
    // -------------------------------------------------------------------------
    // resizable: Allow or prevent window resizing
    gtk_window_set_resizable(GTK_WINDOW(window), con->window->resizable);
    // min_width & min_height: Enforce minimum size limits
    gtk_widget_set_size_request(window, con->window->min_width, con->window->min_height);
    // decorations: Show or hide window titlebar & system borders (frameless mode)
    gtk_window_set_decorated(GTK_WINDOW(window), con->window->decorations);
    gtk_window_set_child(GTK_WINDOW(window), webview);

    // maximized & fullscreen states
    if (con->window->maximized) {
        gtk_window_maximize(GTK_WINDOW(window));
    }
    if (con->window->fullscreen) {
        gtk_window_fullscreen(GTK_WINDOW(window));
    }

    // icon_name: System icon from GTK icon theme
    if (con->window->icon_name) {
        gtk_window_set_icon_name(GTK_WINDOW(window), con->window->icon_name);
    }

    // -------------------------------------------------------------------------
    // 4. WEBKIT FEATURE FLAGS (User-Agent, JS, Images, Popup Windows)
    // -------------------------------------------------------------------------
    if (con->webview->user_agent) {
        webkit_settings_set_user_agent(settings, con->webview->user_agent);
    }

    webkit_settings_set_enable_javascript(settings, con->webview->javascript_enabled);
    webkit_settings_set_auto_load_images(settings, con->webview->auto_load_images);
    webkit_settings_set_javascript_can_open_windows_automatically(settings, con->webview->javascript_can_open_windows);
    
    // -------------------------------------------------------------------------
    // 5. IPC & SCRIPT INJECTION (Bridge, Startup Scripts, Custom CSS)
    // -------------------------------------------------------------------------
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview));
    ipc_init(manager);
    
    // Inject the Vyom IPC bridge at document start
    WebKitUserScript *bridge_script = webkit_user_script_new(
        bridge_js,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL, NULL
    );
    webkit_user_content_manager_add_script(manager, bridge_script);
    webkit_user_script_unref(bridge_script);

    // Optional user startup script
    if (con->webview->startup_script) {
        WebKitUserScript *script = webkit_user_script_new(
            con->webview->startup_script,
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
            NULL, NULL
        );
        webkit_user_content_manager_add_script(manager, script);
        webkit_user_script_unref(script);
    }

    // Optional user custom stylesheet
    if (con->webview->custom_css) {
        WebKitUserStyleSheet *style = webkit_user_style_sheet_new(
            con->webview->custom_css,
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_STYLE_LEVEL_USER,
            NULL, NULL
        );
        webkit_user_content_manager_add_style_sheet(manager, style);
        webkit_user_style_sheet_unref(style);
    }

    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview),con->webview->url);
    gtk_window_present(GTK_WINDOW(window));
    return;
}

int platform_app_run(const app_config_t *config, int argc, char *argv[]) 
{
    GtkApplication *app = gtk_application_new(NULL,G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app,"activate",G_CALLBACK(activate_callback),(gpointer)config);

    g_application_run(G_APPLICATION(app),argc,argv);
    g_object_unref(app);
    return 0;
}

void platform_app_emit(const char *event, const char *json_data) 
{
    if (!g_active_webview || !event) return;

    const char *payload = (json_data && *json_data) ? json_data : "null";

    char *script = g_strdup_printf("window.vyom._emit(\"%s\", %s);", event, payload);
    webkit_web_view_evaluate_javascript( WEBKIT_WEB_VIEW(g_active_webview),script,-1,NULL, NULL, NULL, NULL, NULL);
    g_free(script);
}

void platform_window_minimize(void) 
{
    if (g_active_window) gtk_window_minimize(GTK_WINDOW(g_active_window));
}

void platform_window_toggle_maximize(void) 
{
    if (!g_active_window) return;
    GtkWindow *win = GTK_WINDOW(g_active_window);
    if (gtk_window_is_maximized(win)) {
        gtk_window_unmaximize(win);
    } else {
        gtk_window_maximize(win);
    }
}

void platform_window_close(void) 
{
    if (g_active_window) gtk_window_close(GTK_WINDOW(g_active_window));
}

void platform_window_start_drag(void) 
{
    if (!g_active_window) return;
    GtkNative *native = GTK_NATIVE(g_active_window);
    GdkSurface *surface = gtk_native_get_surface(native);

    if (surface && GDK_IS_TOPLEVEL(surface)) 
    {
        GdkToplevel *toplevel = GDK_TOPLEVEL(surface);
        GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(g_active_window));
        GdkSeat *seat = gdk_display_get_default_seat(display);
        GdkDevice *device = gdk_seat_get_pointer(seat);

        // Tell Wayland / X11 window manager to begin interactive mouse dragging
        gdk_toplevel_begin_move(toplevel, device, 1, 0, 0, GDK_CURRENT_TIME);
    }
}

void platform_window_set_size(int width, int height) 
{
    if (!g_active_window) return;
    gtk_window_set_default_size(GTK_WINDOW(g_active_window), width, height);
}

void platform_window_get_size(int *width, int *height) 
{
    if (!g_active_window) { *width = 0; *height = 0; return; }
    *width = gtk_widget_get_width(GTK_WIDGET(g_active_window));
    *height = gtk_widget_get_height(GTK_WIDGET(g_active_window));
}

void platform_window_set_position(int x, int y) 
{
    // Wayland security model enforces compositor-managed placement.
    (void)x; (void)y;
}

void platform_window_get_position(int *x, int *y) 
{
    *x = 0; *y = 0;
}

void platform_window_center(void) 
{
    // Centering is managed by compositor on Wayland.
}

void platform_window_set_always_on_top(bool always_on_top) 
{
    // GTK4 delegates window layering to toplevel compositor.
    (void)always_on_top;
}

void platform_window_set_resizable(bool resizable) 
{
    if (g_active_window) gtk_window_set_resizable(GTK_WINDOW(g_active_window), resizable);
}

void platform_window_set_min_size(int width, int height) 
{
    if (g_active_window) gtk_widget_set_size_request(GTK_WIDGET(g_active_window), width, height);
}

void platform_window_set_max_size(int width, int height) 
{
    (void)width; (void)height;
}

void platform_window_set_title(const char *title) 
{
    if (g_active_window && title) gtk_window_set_title(GTK_WINDOW(g_active_window), title);
}

bool platform_window_is_maximized(void) 
{
    if (!g_active_window) return false;
    return gtk_window_is_maximized(GTK_WINDOW(g_active_window));
}

bool platform_window_is_minimized(void) 
{
    return false;
}

// -----------------------------------------------------------------------------
// 4. NATIVE DIALOGS (GTK4 Modern FileDialog)
// -----------------------------------------------------------------------------
typedef struct {
    gboolean done;
    char *selected_path;
} dialog_state_t;

static void on_open_finish(GObject *source, GAsyncResult *result, gpointer user_data) 
{
    dialog_state_t *state = (dialog_state_t *)user_data;
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GError *error = NULL;
    GFile *file = gtk_file_dialog_open_finish(dialog, result, &error);
    if (file) 
    {
        char *path = g_file_get_path(file);
        if (path) 
        {
            state->selected_path = g_strdup(path);
            g_free(path);
        }
        g_object_unref(file);
    }
    if (error) 
    {
        g_error_free(error);
    }
    state->done = TRUE;
}

char* platform_dialog_open_file(const char *title, const char *filter) 
{
    (void)filter;
    GtkWindow *parent = g_active_window ? GTK_WINDOW(g_active_window) : NULL;

    GtkFileDialog *dialog = gtk_file_dialog_new();
    if (title && *title)
        gtk_file_dialog_set_title(dialog, title);

    dialog_state_t state = { .done = FALSE, .selected_path = NULL };
    gtk_file_dialog_open(dialog, parent, NULL, on_open_finish, &state);

    // Pump GLib events while waiting so the UI doesn't freeze
    while (!state.done) 
    {
        g_main_context_iteration(NULL, TRUE);
    }

    g_object_unref(dialog);
    return state.selected_path;
}

static void on_save_finish(GObject *source, GAsyncResult *result, gpointer user_data) 
{
    dialog_state_t *state = (dialog_state_t *)user_data;
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GError *error = NULL;
    GFile *file = gtk_file_dialog_save_finish(dialog, result, &error);
    if (file) 
    {
        char *path = g_file_get_path(file);
        if (path) 
        {
            state->selected_path = g_strdup(path);
            g_free(path);
        }
        g_object_unref(file);
    }
    if (error) 
    {
        g_error_free(error);
    }
    state->done = TRUE;
}

char* platform_dialog_save_file(const char *title, const char *filter, const char *default_name) 
{
    (void)filter;
    GtkWindow *parent = g_active_window ? GTK_WINDOW(g_active_window) : NULL;

    GtkFileDialog *dialog = gtk_file_dialog_new();
    if (title && *title)
        gtk_file_dialog_set_title(dialog, title);

    if (default_name && *default_name)
        gtk_file_dialog_set_initial_name(dialog, default_name);

    dialog_state_t state = { .done = FALSE, .selected_path = NULL };
    gtk_file_dialog_save(dialog, parent, NULL, on_save_finish, &state);

    // Pump GLib events while waiting
    while (!state.done) 
    {
        g_main_context_iteration(NULL, TRUE);
    }

    g_object_unref(dialog);
    return state.selected_path;
}

// 1. Native Folder Picker (GtkFileDialog select_folder)
static void on_select_folder_finish(GObject *source, GAsyncResult *result, gpointer user_data) 
{
    dialog_state_t *state = (dialog_state_t *)user_data;
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GError *error = NULL;
    GFile *file = gtk_file_dialog_select_folder_finish(dialog, result, &error);
    if (file) 
    {
        char *path = g_file_get_path(file);
        if (path) 
        {
            state->selected_path = g_strdup(path);
            g_free(path);
        }
        g_object_unref(file);
    }
    if (error) g_error_free(error);
    
    state->done = TRUE;
}

char* platform_dialog_open_directory(const char *title) 
{
    GtkWindow *parent = g_active_window ? GTK_WINDOW(g_active_window) : NULL;
    GtkFileDialog *dialog = gtk_file_dialog_new();
    if (title && *title) gtk_file_dialog_set_title(dialog, title);

    dialog_state_t state = { .done = FALSE, .selected_path = NULL };
    gtk_file_dialog_select_folder(dialog, parent, NULL, on_select_folder_finish, &state);

    while (!state.done) 
    {
        g_main_context_iteration(NULL, TRUE);
    }

    g_object_unref(dialog);
    return state.selected_path; // Caller frees with free()
}

// 2. Native Alert Message Popup (GtkAlertDialog)
void platform_dialog_message(const char *title, const char *message, const char *type) 
{
    (void)type;
    GtkWindow *parent = g_active_window ? GTK_WINDOW(g_active_window) : NULL;
    GtkAlertDialog *alert = gtk_alert_dialog_new("%s", title ? title : "Alert");
    if (message && *message) 
        gtk_alert_dialog_set_detail(alert, message);

    gtk_alert_dialog_set_modal(alert, TRUE);
    gtk_alert_dialog_show(alert, parent);
    g_object_unref(alert);
}

// 3. Native Confirmation Modal (GtkAlertDialog with Cancel / OK)
typedef struct {
    gboolean done;
    int button_idx;
} alert_choose_state_t;

static void on_confirm_finish(GObject *source, GAsyncResult *result, gpointer user_data) 
{
    alert_choose_state_t *state = (alert_choose_state_t *)user_data;
    GtkAlertDialog *alert = GTK_ALERT_DIALOG(source);
    GError *error = NULL;
    state->button_idx = gtk_alert_dialog_choose_finish(alert, result, &error);
    if (error) 
        g_error_free(error);
    
    state->done = TRUE;
}

bool platform_dialog_confirm(const char *title, const char *message) 
{
    GtkWindow *parent = g_active_window ? GTK_WINDOW(g_active_window) : NULL;
    GtkAlertDialog *alert = gtk_alert_dialog_new("%s", title ? title : "Confirm");
    if (message && *message) 
        gtk_alert_dialog_set_detail(alert, message);

    const char *buttons[] = { "Cancel", "OK", NULL };
    gtk_alert_dialog_set_buttons(alert, buttons);
    gtk_alert_dialog_set_cancel_button(alert, 0);
    gtk_alert_dialog_set_default_button(alert, 1);
    gtk_alert_dialog_set_modal(alert, TRUE);

    alert_choose_state_t state = { .done = FALSE, .button_idx = 0 };
    gtk_alert_dialog_choose(alert, parent, NULL, on_confirm_finish, &state);

    while (!state.done) 
    {
        g_main_context_iteration(NULL, TRUE);
    }

    g_object_unref(alert);
    return state.button_idx == 1;
}

// -----------------------------------------------------------------------------
// 5. NATIVE CLIPBOARD (GTK4 / GDK4)
// -----------------------------------------------------------------------------
void platform_clipboard_write_text(const char *text) 
{
    GdkDisplay *display = gdk_display_get_default();
    if (!display) return;
    GdkClipboard *clipboard = gdk_display_get_clipboard(display);
    gdk_clipboard_set_text(clipboard, text ? text : "");
}

void platform_clipboard_clear(void) 
{
    GdkDisplay *display = gdk_display_get_default();
    if (!display) return;
    GdkClipboard *clipboard = gdk_display_get_clipboard(display);
    gdk_clipboard_set_content(clipboard, NULL);
}

bool platform_clipboard_has_text(void) 
{
    GdkDisplay *display = gdk_display_get_default();
    if (!display) return false;
    GdkClipboard *clipboard = gdk_display_get_clipboard(display);
    GdkContentFormats *formats = gdk_clipboard_get_formats(clipboard);
    return gdk_content_formats_contain_gtype(formats, G_TYPE_STRING) != FALSE;
}

static void on_clip_read_finish(GObject *source, GAsyncResult *result, gpointer user_data) 
{
    dialog_state_t *state = (dialog_state_t *)user_data;
    GError *error = NULL;
    char *text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), result, &error);
    if (text) 
    {
        state->selected_path = g_strdup(text);
        g_free(text);
    }
    if (error) 
    {
        g_error_free(error);
    }
    state->done = TRUE;
}

char* platform_clipboard_read_text(void) 
{
    GdkDisplay *display = gdk_display_get_default();
    if (!display) return NULL;
    GdkClipboard *clipboard = gdk_display_get_clipboard(display);

    dialog_state_t state = { .done = FALSE, .selected_path = NULL };
    gdk_clipboard_read_text_async(clipboard, NULL, on_clip_read_finish, &state);

    // Pump GLib events while waiting so IPC stays synchronous & responsive
    while (!state.done) 
    {
        g_main_context_iteration(NULL, TRUE);
    }

    return state.selected_path; // Caller frees with free()
}