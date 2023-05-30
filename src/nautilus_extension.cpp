//
// Created by zhangfuwen on 2022/6/1.
//
#include "nautilus_extension.h"
#include "funterm_config.h"
#include <gtkmm.h>
#include <gtkmm/filechooser.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/window.h>

// #include <libnautilus-extension/nautilus-extension-types.h>

extern "C" {

#include <libnautilus-extension/nautilus-menu-provider.h>

struct _NautilusFuntermMenuProvider {
    GObject parent_instance;
};

static void menu_provider_iface_init(NautilusMenuProviderIface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED(NautilusFuntermMenuProvider,
                               nautilus_funterm_menu_provider,
                               G_TYPE_OBJECT,
                               0,
                               G_IMPLEMENT_INTERFACE_DYNAMIC(NAUTILUS_TYPE_MENU_PROVIDER, menu_provider_iface_init))

#define DATA_KEY "funterm_extension_files"

static void do_file_item_cb(NautilusMenuItem *item, gpointer user_data) {
    GList *files;
    GList *l;

    files = (GList *) g_object_get_data((GObject *) item, DATA_KEY);

    NautilusFileInfo *file = NAUTILUS_FILE_INFO(l->data);
    auto name = nautilus_file_info_get_name(file);
    auto uri = nautilus_file_info_get_uri(file);
    g_print("doing stuff with %s\n", name);
    using namespace std::string_literals;
    system((BIN_DIR "/funterm "s + uri).c_str());

    g_free(uri);
    g_free(name);
}

static void do_background_item_cb(NautilusMenuItem *item, gpointer user_data) {
    char *uri = (char *) g_object_get_data(reinterpret_cast<GObject *>(item), DATA_KEY);
    using namespace std::string_literals;
    g_print("-doing stuff with %s\n", uri);
    // system(("/usr/bin/funterm "s + uri + " &").c_str());
    system((BIN_DIR "/funterm "s + " --tab " + uri + " &").c_str());
    g_free(uri);
}

GList *get_background_items(NautilusMenuProvider *menuProvider, GtkWidget *widget, NautilusFileInfo *info) {
    NautilusMenuItem *item;
    GList *ret = g_list_alloc();
    item = nautilus_menu_item_new(
        "FuntermExtension::open_dir", "Open with Funterm", "Open directory with funterm", NULL /* icon name */);
    g_signal_connect(item, "activate", G_CALLBACK(do_background_item_cb), menuProvider);
    auto uri = nautilus_file_info_get_uri(info);
    g_print("doing stuff with %s\n", uri);
    g_object_set_data_full((GObject *) item, DATA_KEY, uri, nullptr);
    ret = g_list_append(NULL, item);

    return ret;
}

GList *get_file_items(NautilusMenuProvider *menuProvider, GtkWidget *widget, GList *files) {

    // only work when just one file is selected
    int count = 0;
    for (auto l = files; l != NULL; l = l->next) {
        NautilusFileInfo *file = NAUTILUS_FILE_INFO(l->data);
        count++;
        if (!nautilus_file_info_is_directory(file)) {
            return nullptr;
        }
    }
    if (count != 1) {
        return nullptr;
    }

    // add menu items
    NautilusMenuItem *item;
    GList *ret = g_list_alloc();
    item = nautilus_menu_item_new(
        "FuntermExtension::open_dir", "Open with Funterm", "Open directory with funterm", NULL /* icon name */);
    g_signal_connect(item, "activate", G_CALLBACK(do_file_item_cb), menuProvider);
    g_object_set_data_full(
        (GObject *) item, DATA_KEY, nautilus_file_info_list_copy(files), (GDestroyNotify) nautilus_file_info_list_free);
    ret = g_list_append(NULL, item);

    return ret;
}

static void menu_provider_iface_init(NautilusMenuProviderIface *iface) {
    iface->get_background_items = get_background_items;
    iface->get_file_items = get_file_items;
    //    iface->get_file_items = nullptr;
}

static void nautilus_funterm_menu_provider_init(NautilusFuntermMenuProvider *nl) {}
static void nautilus_funterm_menu_provider_finalize(GObject *gobject) {
    NautilusFuntermMenuProvider *nl = NAUTILUS_FUNTERM_MENU_PROVIDER(gobject);
    G_OBJECT_CLASS(nautilus_funterm_menu_provider_parent_class)->finalize(gobject);
}
static void nautilus_funterm_menu_provider_class_finalize(NautilusFuntermMenuProviderClass *klass) {}

static void nautilus_funterm_menu_provider_class_init(NautilusFuntermMenuProviderClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = nautilus_funterm_menu_provider_finalize;
}

static GType type_list[1];

void nautilus_funterm_menu_provider_load(GTypeModule *module) { nautilus_funterm_menu_provider_register_type(module); }

void nautilus_module_initialize(GTypeModule *module) {
    nautilus_funterm_menu_provider_load(module);
    type_list[0] = NAUTILUS_FUNTERM_TYPE_MENU_PROVIDER;
    /* bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR); */
    /* bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8"); */
}
void nautilus_module_shutdown(void) {}
void nautilus_module_list_types(const GType **types, int *num_types) {
    *types = type_list;
    *num_types = G_N_ELEMENTS(type_list);
}
}
