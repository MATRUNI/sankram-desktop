#include "internal.h"
#include <stdio.h>
#include <string.h>

void platform_notification_send(const char *title, const char *body, const char *icon)
{
    GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!bus) return;

    GVariantBuilder actions, hints;
    g_variant_builder_init(&actions, G_VARIANT_TYPE("as"));
    g_variant_builder_init(&hints, G_VARIANT_TYPE("a{sv}"));

    g_dbus_connection_call(
        bus,
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "Notify",
        g_variant_new("(susssasa{sv}i)",
                      "Vyom",
                      (guint32)0,
                      (icon && *icon) ? icon : "dialog-information",
                      (title && *title) ? title : "Vyom",
                      body ? body : "",
                      &actions,
                      &hints,
                      (gint32)-1),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);

    g_object_unref(bus);
}

static GDBusConnection *g_tray_bus = NULL;
static guint g_sni_reg_id = 0;
static guint g_menu_reg_id = 0;
static char g_tray_tooltip[256] = "Vyom";
static char g_tray_icon[256] = "applications-system";
static tray_menu_item_t g_tray_items[32];
static int g_tray_item_count = 0;
static guint32 g_menu_revision = 1;

static const char SNI_XML[] =
    "<node>"
    "  <interface name='org.kde.StatusNotifierItem'>"
    "    <property name='Category' type='s' access='read'/>"
    "    <property name='Id' type='s' access='read'/>"
    "    <property name='Title' type='s' access='read'/>"
    "    <property name='Status' type='s' access='read'/>"
    "    <property name='IconName' type='s' access='read'/>"
    "    <property name='ToolTip' type='(sa(iiay)ss)' access='read'/>"
    "    <property name='Menu' type='o' access='read'/>"
    "    <property name='ItemIsMenu' type='b' access='read'/>"
    "    <method name='Activate'><arg type='i' direction='in'/><arg type='i' direction='in'/></method>"
    "    <signal name='NewToolTip'/>"
    "    <signal name='NewIcon'/>"
    "  </interface>"
    "</node>";

static const char MENU_XML[] =
    "<node>"
    "  <interface name='com.canonical.dbusmenu'>"
    "    <property name='Version' type='u' access='read'/>"
    "    <property name='Status' type='s' access='read'/>"
    "    <method name='GetLayout'>"
    "      <arg type='i' name='parentId' direction='in'/>"
    "      <arg type='i' name='recursionDepth' direction='in'/>"
    "      <arg type='as' name='propertyNames' direction='in'/>"
    "      <arg type='u' name='revision' direction='out'/>"
    "      <arg type='(ia{sv}av)' name='layout' direction='out'/>"
    "    </method>"
    "    <method name='Event'>"
    "      <arg type='i' name='id' direction='in'/>"
    "      <arg type='s' name='eventId' direction='in'/>"
    "      <arg type='v' name='data' direction='in'/>"
    "      <arg type='u' name='timestamp' direction='in'/>"
    "    </method>"
    "    <method name='AboutToShow'>"
    "      <arg type='i' name='id' direction='in'/>"
    "      <arg type='b' name='needUpdate' direction='out'/>"
    "    </method>"
    "    <signal name='LayoutUpdated'>"
    "      <arg type='u' name='revision'/>"
    "      <arg type='i' name='parent'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

static void sni_method_call(GDBusConnection *conn, const gchar *sender, const gchar *path,
                            const gchar *iface, const gchar *method, GVariant *params,
                            GDBusMethodInvocation *inv, gpointer user_data)
{
    (void)conn; (void)sender; (void)path; (void)iface; (void)params; (void)user_data;
    if (strcmp(method, "Activate") == 0)
    {
        if (g_active_window) gtk_window_present(GTK_WINDOW(g_active_window));
        platform_app_emit("tray:click", "{}");
    }
    g_dbus_method_invocation_return_value(inv, NULL);
}

static GVariant* sni_get_prop(GDBusConnection *conn, const gchar *sender, const gchar *path,
                              const gchar *iface, const gchar *prop, GError **error, gpointer user_data)
{
    (void)conn; (void)sender; (void)path; (void)iface; (void)error; (void)user_data;
    if (strcmp(prop, "Category") == 0)   return g_variant_new_string("ApplicationStatus");
    if (strcmp(prop, "Id") == 0)         return g_variant_new_string("vyom-app");
    if (strcmp(prop, "Title") == 0)      return g_variant_new_string(g_tray_tooltip);
    if (strcmp(prop, "Status") == 0)     return g_variant_new_string("Active");
    if (strcmp(prop, "IconName") == 0)   return g_variant_new_string(g_tray_icon);
    if (strcmp(prop, "Menu") == 0)       return g_variant_new_object_path("/MenuBar");
    if (strcmp(prop, "ItemIsMenu") == 0) return g_variant_new_boolean(FALSE);
    if (strcmp(prop, "ToolTip") == 0)
    {
        GVariantBuilder pix;
        g_variant_builder_init(&pix, G_VARIANT_TYPE("a(iiay)"));
        return g_variant_new("(sa(iiay)ss)", g_tray_icon, &pix, g_tray_tooltip, "");
    }
    return NULL;
}

static void menu_method_call(GDBusConnection *conn, const gchar *sender, const gchar *path,
                             const gchar *iface, const gchar *method, GVariant *params,
                             GDBusMethodInvocation *inv, gpointer user_data)
{
    (void)conn; (void)sender; (void)path; (void)iface; (void)user_data;
    if (strcmp(method, "GetLayout") == 0)
    {
        GVariantBuilder root_props, children;
        g_variant_builder_init(&root_props, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&root_props, "{sv}", "children-display", g_variant_new_string("submenu"));

        g_variant_builder_init(&children, G_VARIANT_TYPE("av"));
        for (int i = 0; i < g_tray_item_count; i++)
        {
            GVariantBuilder item_props, sub;
            g_variant_builder_init(&item_props, G_VARIANT_TYPE("a{sv}"));
            g_variant_builder_init(&sub, G_VARIANT_TYPE("av"));

            if (g_tray_items[i].separator)
            {
                g_variant_builder_add(&item_props, "{sv}", "type", g_variant_new_string("separator"));
            }
            else
            {
                g_variant_builder_add(&item_props, "{sv}", "label", g_variant_new_string(g_tray_items[i].label));
                g_variant_builder_add(&item_props, "{sv}", "enabled", g_variant_new_boolean(TRUE));
                g_variant_builder_add(&item_props, "{sv}", "visible", g_variant_new_boolean(TRUE));
            }
            GVariant *child = g_variant_new("(ia{sv}av)", i + 1, &item_props, &sub);
            g_variant_builder_add(&children, "v", child);
        }

        GVariant *layout = g_variant_new("(ia{sv}av)", 0, &root_props, &children);
        g_dbus_method_invocation_return_value(inv, g_variant_new("(u@(ia{sv}av))", g_menu_revision, layout));
        return;
    }
    if (strcmp(method, "Event") == 0)
    {
        gint32 id = 0;
        const gchar *ev_id = NULL;
        g_variant_get(params, "(isvu)", &id, &ev_id, NULL, NULL);
        if (ev_id && strcmp(ev_id, "clicked") == 0 && id >= 1 && id <= g_tray_item_count)
        {
            char payload[128];
            snprintf(payload, sizeof(payload), "{\"id\":\"%s\"}", g_tray_items[id - 1].id);
            platform_app_emit("tray:menu_click", payload);
        }
        g_dbus_method_invocation_return_value(inv, NULL);
        return;
    }
    if (strcmp(method, "AboutToShow") == 0)
    {
        g_dbus_method_invocation_return_value(inv, g_variant_new("(b)", FALSE));
        return;
    }
    g_dbus_method_invocation_return_value(inv, NULL);
}

static GVariant* menu_get_prop(GDBusConnection *conn, const gchar *sender, const gchar *path,
                               const gchar *iface, const gchar *prop, GError **error, gpointer user_data)
{
    (void)conn; (void)sender; (void)path; (void)iface; (void)error; (void)user_data;
    if (strcmp(prop, "Version") == 0) return g_variant_new_uint32(3);
    if (strcmp(prop, "Status") == 0)  return g_variant_new_string("normal");
    return NULL;
}

void platform_tray_create(const char *tooltip, const char *icon)
{
    if (tooltip && *tooltip) snprintf(g_tray_tooltip, sizeof(g_tray_tooltip), "%s", tooltip);
    if (icon && *icon)       snprintf(g_tray_icon, sizeof(g_tray_icon), "%s", icon);

    if (g_tray_bus) return;
    gchar *addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!addr) return;
    g_tray_bus = g_dbus_connection_new_for_address_sync(
        addr,
        G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
        NULL, NULL, NULL);
    g_free(addr);
    if (!g_tray_bus) return;

    static const GDBusInterfaceVTable sni_vtable = { sni_method_call, sni_get_prop, NULL, {0} };
    static const GDBusInterfaceVTable menu_vtable = { menu_method_call, menu_get_prop, NULL, {0} };

    GDBusNodeInfo *sni_info = g_dbus_node_info_new_for_xml(SNI_XML, NULL);
    GDBusNodeInfo *menu_info = g_dbus_node_info_new_for_xml(MENU_XML, NULL);

    g_sni_reg_id = g_dbus_connection_register_object(
        g_tray_bus, "/StatusNotifierItem", sni_info->interfaces[0], &sni_vtable, NULL, NULL, NULL);
    g_menu_reg_id = g_dbus_connection_register_object(
        g_tray_bus, "/MenuBar", menu_info->interfaces[0], &menu_vtable, NULL, NULL, NULL);

    g_dbus_node_info_unref(sni_info);
    g_dbus_node_info_unref(menu_info);

    const gchar *unique_name = g_dbus_connection_get_unique_name(g_tray_bus);
    g_dbus_connection_call(
        g_tray_bus,
        "org.kde.StatusNotifierWatcher",
        "/StatusNotifierWatcher",
        "org.kde.StatusNotifierWatcher",
        "RegisterStatusNotifierItem",
        g_variant_new("(s)", unique_name),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

void platform_tray_set_tooltip(const char *tooltip)
{
    if (!tooltip) return;
    snprintf(g_tray_tooltip, sizeof(g_tray_tooltip), "%s", tooltip);
    if (g_tray_bus)
    {
        g_dbus_connection_emit_signal(
            g_tray_bus, NULL, "/StatusNotifierItem",
            "org.kde.StatusNotifierItem", "NewToolTip", NULL, NULL);
    }
}

void platform_tray_set_menu(const tray_menu_item_t *items, int count)
{
    if (count > 32) count = 32;
    g_tray_item_count = count;
    if (count > 0 && items)
        memcpy(g_tray_items, items, (size_t)count * sizeof(tray_menu_item_t));

    g_menu_revision++;
    if (g_tray_bus)
    {
        g_dbus_connection_emit_signal(
            g_tray_bus, NULL, "/MenuBar",
            "com.canonical.dbusmenu", "LayoutUpdated",
            g_variant_new("(ui)", g_menu_revision, 0), NULL);
    }
}

void platform_tray_destroy(void)
{
    if (!g_tray_bus) return;
    if (g_sni_reg_id)  { g_dbus_connection_unregister_object(g_tray_bus, g_sni_reg_id);  g_sni_reg_id = 0; }
    if (g_menu_reg_id) { g_dbus_connection_unregister_object(g_tray_bus, g_menu_reg_id); g_menu_reg_id = 0; }
    g_dbus_connection_close_sync(g_tray_bus, NULL, NULL);
    g_object_unref(g_tray_bus);
    g_tray_bus = NULL;
}

