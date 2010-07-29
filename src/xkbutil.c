#include <string.h>
#include <glib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <stdio.h> /* for XKBrules.h */
#include <X11/extensions/XKBrules.h>
#include <X11/extensions/XKBstr.h>
#include "xkbutil.h"

#ifndef XKB_RULES_XML_FILE
#define XKB_RULES_XML_FILE "/usr/share/X11/xkb/rules/evdev.xml"
#endif

static Display         *xdisplay;
static gchar          **default_layouts;
static int              default_layout_group;

static IBusEngineDesc *
ibus_xkb_engine_new (gchar    *layout,
                     gchar    *layout_desc,
                     gchar    *variant,
                     gchar    *variant_desc,
                     gchar    *language)
{
    IBusEngineDesc *engine;
    gchar *engine_name = NULL;
    gchar *engine_longname = NULL;
    gchar *engine_layout = NULL;
    gchar **ranked_layouts = NULL;
    int i;

    engine_name = g_strdup_printf ("xkb:%s:%s:%s", layout, variant, language);
    if (variant_desc && *variant_desc) {
        engine_longname = g_strdup_printf ("%s - %s", layout_desc, variant_desc);
    }

    if (variant && *variant) {
        engine_layout = g_strdup_printf ("%s(%s)", layout, variant);
    }

    engine = ibus_engine_desc_new (engine_name,
                                   engine_longname ? engine_longname : layout_desc,
                                   "",
                                   language,
                                   "",
                                   "",
                                   "",
                                   engine_layout ? engine_layout : layout);

    /* set default rank to 0 */
    engine->rank = 0;

#ifdef RANKED_LAYOUTS
    if (RANKED_LAYOUTS != NULL) {
        ranked_layouts = g_strsplit (RANKED_LAYOUTS, ",", 0);
        for (i = 0; ranked_layouts && ranked_layouts[i]; i++) {
            if (g_strcmp0(layout, ranked_layouts[i]) == 0 &&
                (!variant || !*variant)) {
                engine->rank = 100;
            }
        }
        g_strfreev (ranked_layouts);
    }
#endif

    g_free (engine_name);
    g_free (engine_longname);
    g_free (engine_layout);

    return engine;
}

static GList *
ibus_xkb_create_engines (GList *engines,
                         gchar *layout,
                         gchar *layout_desc,
                         gchar *variant,
                         gchar *variant_desc,
                         GList *langs)
{
    GList *l;
    for (l = langs; l != NULL; l = l->next) {
        engines = g_list_prepend (engines, ibus_xkb_engine_new (layout, layout_desc, variant, variant_desc, (gchar *) l->data));
    }

    return engines;
}

static void
ibus_xkb_parse_config_item (XMLNode  *node,
                            gchar   **name,
                            gchar   **desc,
                            GList   **langs)
{
    GList *p = NULL;
    for (p = node->sub_nodes; p != NULL; p = p->next) {
        XMLNode *sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "name") == 0) {
            *name = sub_node->text;
        }
        else if (g_strcmp0 (sub_node->name, "description") == 0) {
            *desc = sub_node->text;
        }
        else if (g_strcmp0 (sub_node->name, "languageList") == 0) {
            GList *l = NULL;
            for (l = sub_node->sub_nodes; l != NULL; l = l->next) {
                XMLNode *lang_node = (XMLNode *) l->data;
                if (g_strcmp0 (lang_node->name, "iso639Id") == 0) {
                    *langs = g_list_prepend (*langs, lang_node->text);
                }
            }
        }
    }
    if (*langs) {
        *langs = g_list_reverse (*langs);
    }
}

static GList *
ibus_xkb_parse_variant_list (GList *engines,
                             XMLNode *node,
                             gchar *layout_name,
                             gchar *layout_desc,
                             GList *layout_langs)
{
    GList *p = NULL;
    for (p = node->sub_nodes; p != NULL; p = p->next) {
        XMLNode *variant = (XMLNode *) p->data;
        if (g_strcmp0 (variant->name, "variant") == 0 && variant->sub_nodes) {
            XMLNode *config = (XMLNode *) variant->sub_nodes->data;
            if (g_strcmp0 (config->name, "configItem") == 0) {
                gchar *name = NULL;
                gchar *desc = NULL;
                GList *langs = NULL;

                ibus_xkb_parse_config_item (config, &name, &desc, &langs);

                if (name && desc && (langs || layout_langs)) {
                    engines = ibus_xkb_create_engines (engines, layout_name, layout_desc, name, desc,
                                                       langs ? langs : layout_langs);
                }

                g_list_free (langs);
            }
        }
    }

    return engines;
}

static GList *
ibus_xkb_parse_layout (GList   *engines,
                       XMLNode *layout)
{
    gchar *name = NULL;
    gchar *desc = NULL;
    GList *langs = NULL;
    GList *p = NULL;

    for (p = layout->sub_nodes; p != NULL; p = p->next) {
        XMLNode *sub_node = (XMLNode *) p->data;
        if (g_strcmp0 (sub_node->name, "configItem") == 0 && !name) {
            ibus_xkb_parse_config_item (sub_node, &name, &desc, &langs);

            if (name && desc && langs) {
                engines = ibus_xkb_create_engines (engines, name, desc, "", "", langs);
            }
        }
        else if (g_strcmp0 (sub_node->name, "variantList") == 0 && name) {
            engines = ibus_xkb_parse_variant_list (engines, sub_node, name, desc, langs);
        }
    }

    g_list_free (langs);

    return engines;
}

static GList *
ibus_xkb_parse_layout_list (GList   *engines,
                            XMLNode *layout_list)
{
    GList *p = NULL;
    for (p = layout_list->sub_nodes; p != NULL; p = p->next) {
        XMLNode *layout = (XMLNode *) p->data;

        if (g_strcmp0 (layout->name, "layout") == 0) {
           engines = ibus_xkb_parse_layout(engines, layout);
        }
    }

    return engines;
}

GList *
ibus_xkb_list_engines (void)
{
    GList *engines = NULL;
    GList *p = NULL;
    XMLNode *xkb_rules_xml = ibus_xml_parse_file (XKB_RULES_XML_FILE);

    if (!xkb_rules_xml) {
        return NULL;
    }

    if (g_strcmp0 (xkb_rules_xml->name, "xkbConfigRegistry") != 0) {
        g_warning ("File %s is not a valid XKB rules xml file.", XKB_RULES_XML_FILE);
        ibus_xml_free (xkb_rules_xml);
        return NULL;
    }

    for (p = xkb_rules_xml->sub_nodes; p != NULL; p = p->next) {
        XMLNode *sub_node = (XMLNode *) p->data;

        if (g_strcmp0 (sub_node->name, "layoutList") == 0) {
           engines = ibus_xkb_parse_layout_list (engines, sub_node);
        }
    }

    ibus_xml_free (xkb_rules_xml);
    return g_list_reverse (engines);
}

static void
init_xkb_default_layouts (void)
{
    XkbStateRec state;
    Atom xkb_rules_name, type;
    int format;
    unsigned long l, nitems, bytes_after;
    unsigned char *prop = NULL;

    xdisplay = XOpenDisplay (NULL);

    if (XkbGetState (xdisplay, XkbUseCoreKbd, &state) != Success) {
        g_warning ("Could not get state");
        return;
    }
    default_layout_group = state.group;

    xkb_rules_name = XInternAtom (xdisplay, "_XKB_RULES_NAMES", TRUE);
    if (xkb_rules_name == None) {
        g_warning ("Could not get XKB rules atom");
        return;
    }
    if (XGetWindowProperty (xdisplay,
                            XDefaultRootWindow (xdisplay),
                            xkb_rules_name,
                            0, 1024, FALSE, XA_STRING,
                            &type, &format, &nitems, &bytes_after, &prop) != Success) {
        g_warning ("Could not get X property");
        return;
    }
    if (nitems < 3) {
        g_warning ("Could not get group layout from X property");
        return;
    }
    for (l = 0; l < 2; l++) {
        prop += strlen ((const char *) prop) + 1;
    }
    if (prop == NULL || *prop == '\0') {
        g_warning ("No layouts form X property");
        return;
    }
    default_layouts = g_strsplit ((gchar *) prop, ",", -1);
}

static Bool
set_xkb_rules (Display *xdisplay,
               const char *rules_file, const char *model,
               const char *all_layouts, const char *all_variants,
               const char *all_options)
{
    gchar *rules_path;
    XkbRF_RulesPtr rules;
    XkbRF_VarDefsRec rdefs;
    XkbComponentNamesRec rnames;
    XkbDescPtr xkb;

    rules_path = g_strdup ("./rules/evdev");
    rules = XkbRF_Load (rules_path, "C", TRUE, TRUE);
    if (rules == NULL) {
        g_return_val_if_fail (XKB_RULES_XML_FILE != NULL, FALSE);

        g_free (rules_path);
        if (g_str_has_suffix (XKB_RULES_XML_FILE, ".xml")) {
            rules_path = g_strndup (XKB_RULES_XML_FILE,
                                    strlen (XKB_RULES_XML_FILE) - 4);
        } else {
            rules_path = g_strdup (XKB_RULES_XML_FILE);
        }
        rules = XkbRF_Load (rules_path, "C", TRUE, TRUE);
    }
    g_return_val_if_fail (rules != NULL, FALSE);

    memset (&rdefs, 0, sizeof (XkbRF_VarDefsRec));
    memset (&rnames, 0, sizeof (XkbComponentNamesRec));
    rdefs.model = model ? g_strdup (model) : NULL;
    rdefs.layout = all_layouts ? g_strdup (all_layouts) : NULL;
    rdefs.variant = all_variants ? g_strdup (all_variants) : NULL;
    rdefs.options = all_options ? g_strdup (all_options) : NULL;
    XkbRF_GetComponents (rules, &rdefs, &rnames);
    xkb = XkbGetKeyboardByName (xdisplay, XkbUseCoreKbd, &rnames,
                                XkbGBN_AllComponentsMask,
                                XkbGBN_AllComponentsMask &
                                (~XkbGBN_GeometryMask), True);
    if (!xkb) {
        g_warning ("Cannot load new keyboard description.");
        return FALSE;
    }
    XkbRF_SetNamesProp (xdisplay, rules_path, &rdefs);
    g_free (rules_path);
    g_free (rdefs.model);
    g_free (rdefs.layout);
    g_free (rdefs.variant);
    g_free (rdefs.options);

    return TRUE;
}

static Bool
update_xkb_properties (Display *xdisplay,
                       const char *rules_file, const char *model,
                       const char *all_layouts, const char *all_variants,
                       const char *all_options)
{
    int len;
    char *pval;
    char *next;
    Atom rules_atom;
    Window root_window;

    len = (rules_file ? strlen (rules_file) : 0);
    len += (model ? strlen (model) : 0);
    len += (all_layouts ? strlen (all_layouts) : 0);
    len += (all_variants ? strlen (all_variants) : 0);
    len += (all_options ? strlen (all_options) : 0);

    if (len < 1) {
        return TRUE;
    }
    len += 5; /* trailing NULs */

    rules_atom = XInternAtom (xdisplay, _XKB_RF_NAMES_PROP_ATOM, False);
    root_window = XDefaultRootWindow (xdisplay);
    pval = next = g_new0 (char, len + 1);
    if (!pval) {
        return TRUE;
    }

    if (rules_file) {
        strcpy (next, rules_file);
        next += strlen (rules_file);
    }
    *next++ = '\0';
    if (model) {
        strcpy (next, model);
        next += strlen (model);
    }
    *next++ = '\0';
    if (all_layouts) {
        strcpy (next, all_layouts);
        next += strlen (all_layouts);
    }
    *next++ = '\0';
    if (all_variants) {
        strcpy (next, all_variants);
        next += strlen (all_variants);
    }
    *next++ = '\0';
    if (all_options) {
        strcpy (next, all_options);
        next += strlen (all_options);
    }
    *next++ = '\0';
    if ((next - pval) != len) {
        g_free (pval);
        return TRUE;
    }

    XChangeProperty (xdisplay, root_window,
                    rules_atom, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) pval, len);
    XSync(xdisplay, False);

    return TRUE;
}

void
ibus_xkb_init (int *argcp, gchar ***argvp)
{
    init_xkb_default_layouts ();
}

IBusComponent *
ibus_xkb_get_component (void)
{
    GList *engines, *p;
    IBusComponent *component;

    component = ibus_component_new ("org.freedesktop.IBus.XKBLayouts",
                                    "XKB Layouts Component",
                                    "0.0.0",
                                    "",
                                    "James Su <james.su@gmail.com>",
                                    "http://github.com/suzhe/ibus-xkb-layouts",
                                    "",
                                    "ibus-xkb-layouts");

    engines = ibus_xkb_list_engines ();

    for (p = engines; p != NULL; p = p->next) {
        ibus_component_add_engine (component, (IBusEngineDesc *) p->data);
    }

    g_list_free (engines);
    return component;
}

gboolean
ibus_xkb_set_layouts (const char *layouts,
                      const char *variants,
                      const char *options)
{
    gboolean retval;
    gchar *layouts_line;

    if (default_layouts == NULL) {
        init_xkb_default_layouts ();
    }
    if (layouts == NULL) {
        layouts_line = g_strjoinv (",", (gchar **) default_layouts);
    } else {
        layouts_line = g_strdup (layouts);
    }

    retval = set_xkb_rules (xdisplay,
                            "evdev", "evdev",
                            layouts_line, variants, options);
    update_xkb_properties (xdisplay,
                           "evdev", "evdev",
                           layouts_line, variants, options);
    g_free (layouts_line);

    return retval;
}
