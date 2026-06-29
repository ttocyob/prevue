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

EAPI_MAIN int
elm_main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: prevue <image>\n");
        return 1;
    }

    const char *path = argv[1];

    /* Resolve to absolute path before any IPC attempt */
    char abs_path[PATH_MAX];
    if (!realpath(path, abs_path))
    {
        fprintf(stderr, "prevue: cannot resolve path: %s\n", path);
        return 1;
    }

    /* Single-instance check: forward to running instance and exit */
    if (ipc_client_send(abs_path))
        return 0;

    /* ---- No running instance — become it ---- */

    elm_init(argc, argv);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    PrevueApp *app = calloc(1, sizeof(PrevueApp));

    /* Config (window position) */
    app->config = config_load();

    /* Window */
    app->win = elm_win_add(NULL, "prevue", ELM_WIN_BASIC);
    elm_win_title_set(app->win, "Prevue");

    /* Let's override the bg color set by 'elm_bg' with a rectangle for now */
    Evas_Object *bg = evas_object_rectangle_add(evas_object_evas_get(app->win));
    evas_object_color_set(bg, 32, 32, 32, 255);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(app->win, bg);
    evas_object_show(bg);

    elm_win_autodel_set(app->win, EINA_TRUE);
    evas_object_size_hint_min_set(app->win, MIN_WIN_W, MIN_WIN_H);
    evas_object_smart_callback_add(app->win, "delete,request", _win_del_cb, app);

    /* Restore saved position (size is driven by image) */
    if (app->config->win_x || app->config->win_y)
        evas_object_move(app->win, app->config->win_x, app->config->win_y);

    /* Load image */
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

    /* Now that the window is realised, calculate fit and apply */
    image_recalc(app);

    /* Wire resize callback */
    evas_object_event_callback_add(app->win, EVAS_CALLBACK_RESIZE,
                                   _win_resize_cb, app);

    /* Become the IPC server */
    ipc_server_init(app);

    elm_run();
    elm_shutdown();
    return 0;
}
ELM_MAIN()