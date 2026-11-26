#include "internal.h"

typedef struct {
    gboolean done;
    char *text;
} clip_read_state_t;

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
    clip_read_state_t *state = (clip_read_state_t *)user_data;
    GError *error = NULL;
    char *text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), result, &error);
    if (text)
    {
        state->text = g_strdup(text);
        g_free(text);
    }
    if (error) g_error_free(error);
    state->done = TRUE;
}

char* platform_clipboard_read_text(void)
{
    GdkDisplay *display = gdk_display_get_default();
    if (!display) return NULL;
    GdkClipboard *clipboard = gdk_display_get_clipboard(display);

    clip_read_state_t state = { .done = FALSE, .text = NULL };
    gdk_clipboard_read_text_async(clipboard, NULL, on_clip_read_finish, &state);

    // Pump GLib events while waiting so IPC stays synchronous & responsive
    while (!state.done)
        g_main_context_iteration(NULL, TRUE);

    return state.text;
}

