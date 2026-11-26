#include "internal.h"

void platform_window_minimize(void)
{
    if (g_active_window) gtk_window_minimize(GTK_WINDOW(g_active_window));
}

void platform_window_toggle_maximize(void)
{
    if (!g_active_window) return;
    GtkWindow *win = GTK_WINDOW(g_active_window);
    if (gtk_window_is_maximized(win))
        gtk_window_unmaximize(win);
    else
        gtk_window_maximize(win);
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
    GtkWindow *win = GTK_WINDOW(g_active_window);
    if (gtk_window_is_maximized(win))
        gtk_window_unmaximize(win);
    gtk_window_set_default_size(win, width, height);
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

