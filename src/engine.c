#include "engine.h"
#include "xkbutil.h"

typedef struct _IBusXkbLayoutEngine IBusXkbLayoutEngine;
typedef struct _IBusXkbLayoutEngineClass IBusXkbLayoutEngineClass;

struct _IBusXkbLayoutEngine {
    IBusEngine parent;
};

struct _IBusXkbLayoutEngineClass {
    IBusEngineClass parent;
};

/* functions prototype */
static void     ibus_xkb_layout_engine_class_init       (IBusXkbLayoutEngineClass   *klass);
static void     ibus_xkb_layout_engine_init             (IBusXkbLayoutEngine        *engine);
static void     ibus_xkb_layout_engine_destroy          (IBusXkbLayoutEngine        *engine);
static gboolean ibus_xkb_layout_engine_process_key_event(IBusEngine                 *engine,
                                                         guint                       keyval,
                                                         guint                       keycode,
                                                         guint                       modifiers);
static void     ibus_xkb_layout_engine_enable           (IBusEngine *engine);
static void     ibus_xkb_layout_engine_disable          (IBusEngine *engine);

G_DEFINE_TYPE (IBusXkbLayoutEngine, ibus_xkb_layout_engine, IBUS_TYPE_ENGINE)

static void
ibus_xkb_layout_engine_class_init (IBusXkbLayoutEngineClass *klass)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_xkb_layout_engine_destroy;

    engine_class->process_key_event = ibus_xkb_layout_engine_process_key_event;
    engine_class->enable = ibus_xkb_layout_engine_enable;
    engine_class->disable = ibus_xkb_layout_engine_disable;
}

static void
ibus_xkb_layout_engine_init (IBusXkbLayoutEngine *xkb_layout)
{
}

static void
ibus_xkb_layout_engine_destroy (IBusXkbLayoutEngine *xkb_layout)
{
    ibus_xkb_set_layouts (NULL, NULL, NULL);
    ((IBusObjectClass *) ibus_xkb_layout_engine_parent_class)->destroy ((IBusObject *)xkb_layout);
}

static gboolean
ibus_xkb_layout_engine_process_key_event (IBusEngine *engine,
                                          guint       keyval,
                                          guint       keycode,
                                          guint       modifiers)
{
    /* TODO: Support Compose/Dead keys */
    return FALSE;
}

static gchar *
get_engine_layout (IBusEngine *engine)
{
    GList *engines, *p;
    IBusComponent *component;
    IBusEngineDesc *desc = NULL;
    gchar *layout = NULL;

    component = ibus_xkb_get_component ();
    engines = ibus_component_get_engines (component);
    for (p = engines; p != NULL; p = p->next) {
        desc = (IBusEngineDesc *)p->data;
        if (!g_strcmp0 (ibus_engine_get_name (engine), desc->name)) {
            break;
        }
    }
    if (p != NULL) {
        layout = desc->layout ? g_strdup (desc->layout) : NULL;
    }
    g_object_unref (component);

    return layout;
}

static void
ibus_xkb_layout_engine_enable (IBusEngine *engine)
{
    gchar *layout = get_engine_layout (engine);

    if (layout == NULL) {
        return;
    }
    ibus_xkb_set_layouts (layout, NULL, NULL);
    g_free (layout);
}

static void
ibus_xkb_layout_engine_disable (IBusEngine *engine)
{
    /* TODO: If get_engine_layout() returns "ru", you may like to set
     * "us" layout in the disable IM.
     * I think setup dialog is better in such a case and the default
     * is better not to change layouts. */
    ibus_xkb_set_layouts (NULL, NULL, NULL);
}
