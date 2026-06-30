#include <Elementary.h>
#include <Ecore_File.h>
#include "prevue.h"
#include "image.h"
#include "config.h"
#include "ipc.h"
#include "minimap.h"

static void
_win_del_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
    PrevueApp *app = data;

    config_save(app);
    ipc_server_shutdown(app);
    image_free(app->img);
    minimap_free(app);
    free(app->config);
    free(app);

    elm_exit();
}

static void
_win_resize_cb(void *data, Evas *e EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
    PrevueApp *app = data;
    if (!app->img) return;
    image_recalc(app);
}

static Eina_Bool
_config_changed_cb(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
    static double last_scale = 0.0;
    PrevueApp *app = data;

    double scale = elm_config_scale_get();
    if (scale < 1.0 || scale > 2.0) scale = 1.0;
    if (scale == last_scale) return ECORE_CALLBACK_PASS_ON;
    last_scale = scale;

    edje_scale_set(scale);
    if (app->img)
    {
        app->img->fit_applied = false;   /* force re-snap to new fit_zoom */
        image_window_fit(app);
        image_recalc(app);
    }

    return ECORE_CALLBACK_PASS_ON;
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
    char abs_path[PATH_MAX] = {0};

    /* if path is given, resolve it and try to forward to a running instance */
    if (argc >= 2)
    {
        if (!realpath(argv[1], abs_path))
        {
            fprintf(stderr, "prevue: cannot resolve path: %s\n", argv[1]);
            return 1;
        }

        if (ipc_client_send(abs_path))
            return 0;
    }

    /* no running instance - initiate it */
    elm_init(argc, argv);

double scale = elm_config_scale_get();
if (scale < 1.0 || scale > 2.0) scale = 1.0;
edje_scale_set(scale);

    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    PrevueApp *app = calloc(1, sizeof(PrevueApp));

    /* Config (window position) */
    app->config = config_load();

    /* Window */
    app->win = elm_win_add(NULL, "prevue", ELM_WIN_BASIC);
    elm_win_title_set(app->win, "Prevue");
    elm_win_autodel_set(app->win, EINA_TRUE);
    evas_object_size_hint_min_set(app->win, MIN_WIN_W, MIN_WIN_H);
    evas_object_smart_callback_add(app->win, "delete,request", _win_del_cb, app);
ecore_event_handler_add(ELM_EVENT_CONFIG_ALL_CHANGED, _config_changed_cb, app);

    /* set the canvas bg: there must be another way to override elm_bg than using a rect */
    Evas_Object *bg = evas_object_rectangle_add(evas_object_evas_get(app->win));
    evas_object_color_set(bg, 32, 32, 32, 255);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(app->win, bg);
    evas_object_show(bg);

    /* restore saved position from eet */
    if (app->config->win_x || app->config->win_y)
        evas_object_move(app->win, app->config->win_x, app->config->win_y);

    if (abs_path[0])
    {
        /* load the image and size the window to match */
        if (!image_load(app, abs_path))
        {
            fprintf(stderr, "prevue: failed to load: %s\n", abs_path);
            free(app->config);
            free(app);
            elm_shutdown();
            return 1;
        }

        image_window_fit(app);
        evas_object_show(app->win);
        image_recalc(app);
    }
    else
    {
        /* no image - open at minimum size, IPC ready */
        evas_object_resize(app->win, MIN_WIN_W, MIN_WIN_H);
        evas_object_show(app->win);
    }

    /* resize callback */
    evas_object_event_callback_add(app->win, EVAS_CALLBACK_RESIZE,
                                   _win_resize_cb, app);

    /* initiate the IPC server */
    ipc_server_init(app);

    elm_run();
    elm_shutdown();
    return 0;
}
ELM_MAIN()