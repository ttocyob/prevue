#include "minimap.h"
#include "image.h"

/* ------------------------------------------------------------------ */
/* Viewport drag signal callback                                        */
/* ------------------------------------------------------------------ */

static void
_viewport_drag_cb(void *data, Evas_Object *obj,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
    PrevueApp  *app = data;
    ImageState *s   = app->img;
    if (!s) return;

    double dx = 0.5, dy = 0.5;
    edje_object_part_drag_value_get(obj, "viewport", &dx, &dy);

    int win_w, win_h;
    evas_object_geometry_get(app->win, NULL, NULL, &win_w, &win_h);

    double ratio_w = (double)win_w  / (s->orig_w * s->zoom);
    double ratio_h = (double)win_h  / (s->orig_h * s->zoom);
    if (ratio_w > 1.0) ratio_w = 1.0;
    if (ratio_h > 1.0) ratio_h = 1.0;

    double half_x = (1.0 - ratio_w) * 0.5;
    double half_y = (1.0 - ratio_h) * 0.5;

    s->pan_x = -(dx - 0.5) * 2.0 * half_x;
    s->pan_y = -(dy - 0.5) * 2.0 * half_y;

    image_recalc(app);
}

/* ------------------------------------------------------------------ */
/* Internal init                                                        */
/* ------------------------------------------------------------------ */

static bool
_minimap_create(PrevueApp *app)
{
    Evas *canvas = evas_object_evas_get(app->win);

    Evas_Object *edje = edje_object_add(canvas);
    if (!edje_object_file_set(edje, EDJ_PATH, "prevue/minimap"))
    {
        fprintf(stderr, "minimap: could not load prevue/minimap from %s\n", EDJ_PATH);
        evas_object_del(edje);
        return false;
    }

    /* Thumbnail image swallowed into the EDJ */
    Evas_Object *thumb = evas_object_image_add(canvas);
    evas_object_image_file_set(thumb, app->img->path, NULL);
    evas_object_image_filled_set(thumb, EINA_TRUE);
    evas_object_show(thumb);
    edje_object_part_swallow(edje, "thumbnail.image", thumb);

    /* "drag" signal fires whenever the user moves the viewport dragable */
    edje_object_signal_callback_add(edje, "drag", "viewport",
                                    _viewport_drag_cb, app);

    app->minimap = calloc(1, sizeof(MinimapData));
    app->minimap->edje    = edje;
    app->minimap->thumb   = thumb; /* store the thumb pointer */
    app->minimap->visible = false;

    evas_object_stack_above(edje, app->image);

    return true;
}

/* ------------------------------------------------------------------ */
/* Viewport position + size update                                     */
/* ------------------------------------------------------------------ */

static void
_viewport_update(PrevueApp *app)
{
    ImageState *s = app->img;

    int win_w, win_h;
    evas_object_geometry_get(app->win, NULL, NULL, &win_w, &win_h);

    /* Fraction of image visible = viewport handle size as fraction of confine */
    double vp_w = (double)win_w  / (s->orig_w * s->zoom);
    double vp_h = (double)win_h  / (s->orig_h * s->zoom);
    if (vp_w > 1.0) vp_w = 1.0;
    if (vp_h > 1.0) vp_h = 1.0;

    /* Must set size before value — Edje ignores value if size not set */
    edje_object_part_drag_size_set(app->minimap->edje, "viewport", vp_w, vp_h);

    /* Fine step for continuous drag tracking */
    edje_object_part_drag_step_set(app->minimap->edje, "viewport", 0.001, 0.001);

    /* Map pan [-half, +half] to drag value [0, 1] */
    double half_x = (1.0 - vp_w) * 0.5;
    double half_y = (1.0 - vp_h) * 0.5;

    double dx = 0.5, dy = 0.5;
    if (half_x > 0.0) dx = (-s->pan_x / (2.0 * half_x)) + 0.5;
    if (half_y > 0.0) dy = (-s->pan_y / (2.0 * half_y)) + 0.5;
    if (dx < 0.0) dx = 0.0;
    if (dx > 1.0) dx = 1.0;
    if (dy < 0.0) dy = 0.0;
    if (dy > 1.0) dy = 1.0;

    edje_object_part_drag_value_set(app->minimap->edje, "viewport", dx, dy);
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void
minimap_update(PrevueApp *app)
{
    if (!app->img) return;

    if (!app->minimap)
        if (!_minimap_create(app)) return;

    /* Size and place the EDJ — aspect-correct within MINIMAP_W x MINIMAP_H */
    int win_w, win_h;
    evas_object_geometry_get(app->win, NULL, NULL, &win_w, &win_h);

    double aspect = (double)app->img->orig_w / (double)app->img->orig_h;
    int mw, mh;
    if (aspect >= (double)MINIMAP_W / (double)MINIMAP_H)
    {
        mw = MINIMAP_W;
        mh = (int)round(MINIMAP_W / aspect);
    }
    else
    {
        mh = MINIMAP_H;
        mw = (int)round(MINIMAP_H * aspect);
    }

    evas_object_move(app->minimap->edje,
                     win_w - mw - MINIMAP_MARGIN, MINIMAP_MARGIN);
    evas_object_resize(app->minimap->edje, mw, mh);

    _viewport_update(app);

    if (!app->minimap->visible)
    {
        evas_object_show(app->minimap->edje);
        app->minimap->visible = true;
    }
}

void
minimap_hide(PrevueApp *app)
{
    if (!app->minimap || !app->minimap->visible) return;
    evas_object_hide(app->minimap->edje);
    app->minimap->visible = false;
}

void
minimap_free(PrevueApp *app)
{
    if (!app->minimap) return;
    edje_object_part_unswallow(app->minimap->edje, app->minimap->thumb);
    evas_object_del(app->minimap->thumb);
    evas_object_del(app->minimap->edje);
    free(app->minimap);
    app->minimap = NULL;
}