//
// Created by zhangfuwen on 2022/6/1.
//

#ifndef FUNTERM_NAUTILUS_EXTENSION_H
#define FUNTERM_NAUTILUS_EXTENSION_H

#include <glib-object.h>

G_BEGIN_DECLS
#define NAUTILUS_FUNTERM_TYPE_MENU_PROVIDER (nautilus_funterm_menu_provider_get_type())
G_DECLARE_FINAL_TYPE(NautilusFuntermMenuProvider,
                     nautilus_funterm_menu_provider,
                     NAUTILUS_FUNTERM,
                     MENU_PROVIDER,
                     GObject)
void
nautilus_funterm_menu_provider_load(GTypeModule* module);
G_END_DECLS


#endif // FUNTERM_NAUTILUS_EXTENSION_H
