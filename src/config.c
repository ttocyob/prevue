#include "config.h"
#include <Eet.h>
#include <Ecore_File.h>

/* ------------------------------------------------------------------ */
/* EET descriptor                                                       */
/* ------------------------------------------------------------------ */

static Eet_Data_Descriptor *_edd = NULL;

static void
_edd_init(void)
{
    if (_edd) return;

    Eet_Data_Descriptor_Class eddc;
    EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, ConfigData);
    _edd = eet_data_descriptor_stream_new(&eddc);

    EET_DATA_DESCRIPTOR_ADD_BASIC(_edd, ConfigData, "win_x", win_x, EET_T_INT);
    EET_DATA_DESCRIPTOR_ADD_BASIC(_edd, ConfigData, "win_y", win_y, EET_T_INT);
}

/* ------------------------------------------------------------------ */
/* Path helper                                                          */
/* ------------------------------------------------------------------ */

static const char *
_config_path(void)
{
    static char path[PATH_MAX] = {0};
    if (path[0]) return path;

    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(path, sizeof(path), "%s/.config/prevue/prevue.eet", home);
    return path;
}

static void
_ensure_config_dir(void)
{
    char dir[PATH_MAX];
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    snprintf(dir, sizeof(dir), "%s/.config/prevue", home);
    if (!ecore_file_exists(dir))
        ecore_file_mkpath(dir);
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

ConfigData *
config_load(void)
{
    eet_init();
    _edd_init();

    ConfigData *cfg = NULL;
    const char *path = _config_path();

    if (ecore_file_exists(path))
    {
        Eet_File *ef = eet_open(path, EET_FILE_MODE_READ);
        if (ef)
        {
            cfg = eet_data_read(ef, _edd, "config");
            eet_close(ef);
        }
    }

    if (!cfg)
        cfg = calloc(1, sizeof(ConfigData));  /* zeroed: win_x=0, win_y=0 */

    return cfg;
}

void
config_save(PrevueApp *app)
{
    if (!app->config || !app->win) return;

    int x = 0, y = 0;
    evas_object_geometry_get(app->win, &x, &y, NULL, NULL);

    /* Don't persist (0,0) — likely an unrealised window */
    if (x == 0 && y == 0) return;

    app->config->win_x = x;
    app->config->win_y = y;

    eet_init();
    _edd_init();
    _ensure_config_dir();

    const char *path = _config_path();
    Eet_File *ef = eet_open(path, EET_FILE_MODE_WRITE);
    if (!ef)
    {
        fprintf(stderr, "config_save: could not open %s for writing\n", path);
        return;
    }

    if (!eet_data_write(ef, _edd, "config", app->config, EINA_TRUE))
        fprintf(stderr, "config_save: eet_data_write failed\n");

    eet_close(ef);
}