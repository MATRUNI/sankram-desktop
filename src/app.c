#include "app.h"
#include "bridge.h"

#if defined(__linux__)
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#include "ipc.h"

static GtkWidget *g_active_webview = NULL;

static void activate_callback(GtkApplication *app,gpointer user_data)
{
    app_config_t *con =(app_config_t *) user_data;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window),con->window->title);
    gtk_window_set_default_size(GTK_WINDOW(window),con->window->width,con->window->height);
    
    GtkWidget *webview = webkit_web_view_new();

    g_active_webview = webview;

    gtk_widget_set_hexpand(webview, TRUE);
    gtk_widget_set_vexpand(webview, TRUE);

    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
    webkit_settings_set_enable_developer_extras(settings, con->webview->developer_extras);
    webkit_settings_set_disable_web_security(settings,con->webview->disable_web_security);
    if(con->webview->developer_extras)
    {
        WebKitWebInspector *inspector = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(webview));
        webkit_web_inspector_show(inspector);
    }

    gtk_window_set_resizable(GTK_WINDOW(window),con->window->resizable);
    gtk_widget_set_size_request(window,con->window->min_width,con->window->min_height);
    gtk_window_set_decorated(GTK_WINDOW(window),con->window->decorations);
    gtk_window_set_child(GTK_WINDOW(window),webview);

    if (con->window->maximized) {
        gtk_window_maximize(GTK_WINDOW(window));
    }
    if (con->window->fullscreen) {
        gtk_window_fullscreen(GTK_WINDOW(window));
    }

    if (con->window->icon_name) {
        gtk_window_set_icon_name(GTK_WINDOW(window), con->window->icon_name);
    }

    if (con->webview->user_agent) {
        webkit_settings_set_user_agent(settings, con->webview->user_agent);
    }

    webkit_settings_set_enable_javascript(settings, con->webview->javascript_enabled);

    webkit_settings_set_auto_load_images(settings, con->webview->auto_load_images);

    webkit_settings_set_javascript_can_open_windows_automatically(settings, con->webview->javascript_can_open_windows);
    
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview));
    ipc_init(manager);
    
    WebKitUserScript *bridge_script = webkit_user_script_new(
        bridge_js,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL, NULL
    );
    webkit_user_content_manager_add_script(manager, bridge_script);
    webkit_user_script_unref(bridge_script);

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

int app_run(const app_config_t *config, int argc, char *argv[]) 
{
    GtkApplication *app = gtk_application_new(NULL,G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app,"activate",G_CALLBACK(activate_callback),(gpointer)config);

    g_application_run(G_APPLICATION(app),argc,argv);
    g_object_unref(app);
    return 0;
}

void app_emit(const char *event, const char *json_data) 
{
    if (!g_active_webview || !event) return;

    const char *payload = (json_data && *json_data) ? json_data : "null";

    char *script = g_strdup_printf("window.myapp._emit(\"%s\", %s);", event, payload);
    webkit_web_view_evaluate_javascript( WEBKIT_WEB_VIEW(g_active_webview),script,-1,NULL, NULL, NULL, NULL, NULL);
    g_free(script);
}

#elif defined(_WIN32)
// When you're ready for Windows, you put WebView2 code right here!
int app_run(const app_config_t *config, int argc, char *argv[]) {
    // Run Windows message loop
}

#endif