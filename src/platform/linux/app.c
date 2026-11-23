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
    const char *mime = get_memi_type(full_path);

    webkit_uri_scheme_request_finish(request, G_INPUT_STREAM(stream), st.st_size, mime);

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
    
    GtkWidget *webview = webkit_web_view_new();
    g_active_webview = webview;

    gtk_widget_set_hexpand(webview, TRUE);
    gtk_widget_set_vexpand(webview, TRUE);

    // -------------------------------------------------------------------------
    // 2. WEBKIT BROWSER SETTINGS (Developer Tools & Security)
    // -------------------------------------------------------------------------
    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
    webkit_settings_set_disable_web_security(settings, con->webview->disable_web_security);
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
    WebKitWebContext *context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(webview));

    webkit_web_context_register_uri_scheme(context,"app",on_uri_scheme_request,"./dist",NULL);

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