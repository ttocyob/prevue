#ifndef PREVUE_H
#define PREVUE_H

#include <Elementary.h>
#include <Ecore.h>
#include <Evas.h>
#include <Edje.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MIN_WIN_W   200
#define MIN_WIN_H   120
#define PREVIEW_MAX_W  1280
#define PREVIEW_MAX_H   720

/* Zoom limits */
#define ZOOM_MIN    0.1
#define ZOOM_MAX    8.0
#define ZOOM_STEP   0.1

/* Minimap geometry */
#define MINIMAP_W   90
#define MINIMAP_H   70
#define MINIMAP_MARGIN  12

/* IPC */
#define IPC_SERVER_NAME  "prevue"
#define IPC_PORT         13731

#define EDJ_PATH    PREVUE_DATADIR "/minimap.edj"

/* ------------------------------------------------------------------ */

typedef struct _ImageState  ImageState;
typedef struct _MinimapData MinimapData;
typedef struct _ConfigData  ConfigData;
typedef struct _PrevueApp   PrevueApp;

/* Current image and view state — all geometry lives here */
struct _ImageState {
    char        *path;      /* strdup'd file path                  */
    int          orig_w;    /* original pixel dimensions           */
    int          orig_h;
    double       zoom;      /* current zoom factor (1.0 = fit)     */
    double       fit_zoom;  /* zoom at which image fills window    */
    double       pan_x;     /* normalised pan offset [-0.5, 0.5]   */
    double       pan_y;
    /* drag tracking — zeroed on mouse-up */
    bool         dragging;
    int          drag_ox;   /* canvas coords at drag start         */
    int          drag_oy;
    double       drag_pan_x;
    double       drag_pan_y;
    bool         fit_applied;
    bool         smooth_scale; /* true = bilinear, false = nearest-neighbour */
};

/* Minimap overlay */
struct _MinimapData {
    Evas_Object *edje;      /* the EDJ object (prevue/minimap group)  */
    Evas_Object *thumb;     /* store the thumb pointer for deletion   */
    bool         visible;
};

/* EET-persisted config — window position only */
struct _ConfigData {
    int  win_x;
    int  win_y;
};

/* Central application context */
struct _PrevueApp {
    /* UI */
    Evas_Object  *win;
    Evas_Object  *image;    /* elm_image                            */

    /* State */
    ImageState   *img;      /* NULL when no image is loaded         */
    MinimapData  *minimap;
    ConfigData   *config;
};

#endif /* PREVUE_H */