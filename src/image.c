#include "image.h"
#include "minimap.h"

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

static double
_clamp(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Clamp pan so the image never leaves a gap at the window edge.
 * At zoom == fit_zoom the image fills the window exactly → pan must be 0.
 * As zoom increases beyond fit_zoom, the image is larger than the window
 * and pan can range up to ±(1 - fit_zoom/zoom) / 2 in each axis. */
static void
_clamp_pan(ImageState *img)
{
    double ratio = img->fit_zoom / img->zoom;   /* <1 when zoomed in */
    double half  = (1.0 - ratio) * 0.5;        /* max pan magnitude */

    if (half < 0.0) half = 0.0;

    img->pan_x = _clamp(img->pan_x, -half, half);
    img->pan_y = _clamp(img->pan_y, -half, half);
}

/* ------------------------------------------------------------------ */
/* Mouse callbacks (internal, registered by image_load)                */
/* ------------------------------------------------------------------ */

static void
_mouse_wheel_cb(void *data, Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED, void *event_info)
{
    PrevueApp *app = data;
    Evas_Event_Mouse_Wheel *ev = event_info;

    /* ev->z: +1 = scroll down (zoom out), -1 = scroll up (zoom in) */
    double delta = -ev->z * ZOOM_STEP;
    image_zoom_by(app, delta, ev->canvas.x, ev->canvas.y);
}

static void
_mouse_down_cb(void *data, Evas *e EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED, void *event_info)
{
    PrevueApp *app = data;
    ImageState *img = app->img;
    Evas_Event_Mouse_Down *ev = event_info;

    if (ev->button != 1) return;

    img->dragging    = true;
    img->drag_ox     = ev->canvas.x;
    img->drag_oy     = ev->canvas.y;
    img->drag_pan_x  = img->pan_x;
    img->drag_pan_y  = img->pan_y;
}

static void
_mouse_move_cb(void *data, Evas *e EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED, void *event_info)
{
    PrevueApp *app = data;
    ImageState *img = app->img;
    Evas_Event_Mouse_Move *ev = event_info;

    if (!img->dragging) return;

    int win_w, win_h;
    evas_object_geometry_get(app->win, NULL, NULL, &win_w, &win_h);

    double scaled_w = img->orig_w * img->zoom;
    double scaled_h = img->orig_h * img->zoom;

    int dx = ev->cur.canvas.x - img->drag_ox;
    int dy = ev->cur.canvas.y - img->drag_oy;

    img->pan_x = img->drag_pan_x + (double)dx / scaled_w;
    img->pan_y = img->drag_pan_y + (double)dy / scaled_h;

    _clamp_pan(img);
    image_recalc(app);
}

static void
_mouse_up_cb(void *data, Evas *e EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED, void *event_info)
{
    PrevueApp *app = data;
    Evas_Event_Mouse_Up *ev = event_info;

    if (ev->button != 1) return;
    app->img->dragging = false;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

bool
image_load(PrevueApp *app, const char *abs_path)
{
    Evas_Object *img_obj = elm_image_add(app->win);
    if (!img_obj) return false;

    if (!elm_image_file_set(img_obj, abs_path, NULL))
    {
        evas_object_del(img_obj);
        fprintf(stderr, "image_load: elm_image_file_set failed: %s\n", abs_path);
        return false;
    }

    elm_image_no_scale_set(img_obj, EINA_TRUE);
    elm_image_resizable_set(img_obj, EINA_TRUE, EINA_TRUE);

    ImageState *s = calloc(1, sizeof(ImageState));
    s->path   = strdup(abs_path);
    s->zoom   = 1.0;
    s->pan_x  = 0.0;
    s->pan_y  = 0.0;

    int w = 0, h = 0;
    elm_image_object_size_get(img_obj, &w, &h);
    if (w <= 0 || h <= 0)
    {
        Evas_Object *internal = elm_image_object_get(img_obj);
        if (internal)
            evas_object_image_size_get(internal, &w, &h);
    }
    if (w <= 0 || h <= 0)
    {
        fprintf(stderr, "image_load: could not determine image size: %s\n", abs_path);
        free(s->path);
        free(s);
        evas_object_del(img_obj);
        return false;
    }
    s->orig_w = w;
    s->orig_h = h;
    s->smooth_scale = true;   /* bilinear by default */

    evas_object_event_callback_add(app->win, EVAS_CALLBACK_MOUSE_WHEEL,
                               _mouse_wheel_cb, app);
    evas_object_event_callback_add(img_obj, EVAS_CALLBACK_MOUSE_DOWN,
                                   _mouse_down_cb, app);
    evas_object_event_callback_add(img_obj, EVAS_CALLBACK_MOUSE_MOVE,
                                   _mouse_move_cb, app);
    evas_object_event_callback_add(img_obj, EVAS_CALLBACK_MOUSE_UP,
                                   _mouse_up_cb, app);

    evas_object_show(img_obj);

    app->image = img_obj;
    app->img   = s;

    const char *base = strrchr(abs_path, '/');
    elm_win_title_set(app->win, base ? base + 1 : abs_path);

    return true;
}

void
image_window_fit(PrevueApp *app)
{
    double scale_w = (app->img->orig_w > PREVIEW_MAX_W)
                     ? (double)PREVIEW_MAX_W / app->img->orig_w : 1.0;
    double scale_h = (app->img->orig_h > PREVIEW_MAX_H)
                     ? (double)PREVIEW_MAX_H / app->img->orig_h : 1.0;
    double scale   = (scale_w < scale_h) ? scale_w : scale_h;

    int new_w = MAX((int)(app->img->orig_w * scale), MIN_WIN_W);
    int new_h = MAX((int)(app->img->orig_h * scale), MIN_WIN_H);
    evas_object_resize(app->win, new_w, new_h);
}

void
image_recalc(PrevueApp *app)
{
    if (!app->img || !app->image) return;

    ImageState *s = app->img;

    int win_w, win_h;
    evas_object_geometry_get(app->win, NULL, NULL, &win_w, &win_h);
    if (win_w <= 0 || win_h <= 0) return;

    double fit_w = (double)win_w / s->orig_w;
    double fit_h = (double)win_h / s->orig_h;
    s->fit_zoom = (fit_w < fit_h) ? fit_w : fit_h;
    if (s->fit_zoom > 1.0) s->fit_zoom = 1.0;

    /* Snap to fit on first recalc only */
    if (!s->fit_applied)
    {
        s->zoom = s->fit_zoom;
        s->fit_applied = true;
    }

    s->zoom = _clamp(s->zoom, ZOOM_MIN, ZOOM_MAX);

    _clamp_pan(s);

    int img_w = (int)round(s->orig_w * s->zoom);
    int img_h = (int)round(s->orig_h * s->zoom);

    int img_x = (win_w - img_w) / 2 + (int)round(s->pan_x * img_w);
    int img_y = (win_h - img_h) / 2 + (int)round(s->pan_y * img_h);

    evas_object_move(app->image, img_x, img_y);
    evas_object_resize(app->image, img_w, img_h);

    /* Pixel mode: disable smooth scaling above 3× zoom */
    Evas_Object *internal = elm_image_object_get(app->image);
    if (internal)
    {
        Eina_Bool smooth = (s->zoom < 1.0) ? EINA_TRUE : EINA_FALSE;
        evas_object_image_smooth_scale_set(internal, smooth);
    }

    if (s->zoom > s->fit_zoom + 0.001)
        minimap_update(app);
    else
        minimap_hide(app);
}

void
image_zoom_by(PrevueApp *app, double delta, int cx, int cy)
{
    if (!app->img) return;

    ImageState *s = app->img;

    int win_w, win_h;
    evas_object_geometry_get(app->win, NULL, NULL, &win_w, &win_h);

    double old_zoom = s->zoom;
    double new_zoom = _clamp(old_zoom + delta, ZOOM_MIN, ZOOM_MAX);
    if (new_zoom == old_zoom) return;

    double cur_nx = (cx - win_w * 0.5) / (s->orig_w * old_zoom) + s->pan_x;
    double cur_ny = (cy - win_h * 0.5) / (s->orig_h * old_zoom) + s->pan_y;

    s->zoom  = new_zoom;
    s->pan_x = cur_nx - (cx - win_w * 0.5) / (s->orig_w * new_zoom);
    s->pan_y = cur_ny - (cy - win_h * 0.5) / (s->orig_h * new_zoom);

    _clamp_pan(s);
    image_recalc(app);
}

void
image_free(ImageState *img)
{
    if (!img) return;
    free(img->path);
    free(img);
}

bool
image_replace(PrevueApp *app, const char *abs_path)
{
    if (app->image)
    {
        evas_object_del(app->image);
        app->image = NULL;
    }
    image_free(app->img);
    app->img = NULL;

    minimap_free(app);   /* important: destroy old minimap before loading new image */

    if (!image_load(app, abs_path)) return false;

    app->img->zoom  = 1.0;
    app->img->pan_x = 0.0;
    app->img->pan_y = 0.0;

    image_window_fit(app);
    image_recalc(app);
    return true;
}