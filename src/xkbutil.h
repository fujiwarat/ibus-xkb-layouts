#ifndef __XKBUTIL_H__
#define __XKBUTIL_H__

#include <ibus.h>

void             ibus_xkb_init             (int *argc, gchar ***argv);
GList           *ibus_xkb_list_engines     (void);
IBusComponent   *ibus_xkb_get_component    (void);
gboolean         ibus_xkb_set_layouts      (const char *layouts,
                                            const char *variants,
                                            const char *options);

#endif
