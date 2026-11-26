#include "internal.h"

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
    if (error) g_error_free(error);
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
        g_main_context_iteration(NULL, TRUE);

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
    if (error) g_error_free(error);
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

    while (!state.done)
        g_main_context_iteration(NULL, TRUE);

    g_object_unref(dialog);
    return state.selected_path;
}

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
        g_main_context_iteration(NULL, TRUE);

    g_object_unref(dialog);
    return state.selected_path;
}

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
    if (error) g_error_free(error);
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
        g_main_context_iteration(NULL, TRUE);

    g_object_unref(alert);
    return state.button_idx == 1;
}

