#ifndef IMAGE_H
#define IMAGE_H

#include "prevue.h"

/**
 * Load an image file into app->image (elm_image).
 * Populates app->img (ImageState) with path and orig dimensions.
 * Does NOT resize or position — call image_window_fit() then image_recalc().
 *
 * @return true on success
 */
bool image_load(PrevueApp *app, const char *abs_path);

/**
 * Resize the window to match the loaded image, respecting PREVIEW_MAX_W/H
 * and MIN_WIN_W/H. Images within the preview ceiling open at 1:1.
 * Larger images are scaled down proportionally to fit within the ceiling.
 * Called from elm_main() on first load and image_replace() on IPC load.
 */
void image_window_fit(PrevueApp *app);

/**
 * Recalculate fit_zoom from current window size, clamp zoom,
 * and reposition app->image on the canvas.
 * Safe to call from a resize callback.
 */
void image_recalc(PrevueApp *app);

/**
 * Apply a zoom delta (from mouse wheel) centred on canvas point (cx, cy).
 * Adjusts app->img->zoom and pan, then calls image_recalc.
 */
void image_zoom_by(PrevueApp *app, double delta, int cx, int cy);

/**
 * Free ImageState. The elm_image object is deleted separately
 * (by EFL on window close, or explicitly in image_replace).
 */
void image_free(ImageState *img);

/**
 * Replace the current image with a new file path.
 * Frees the old ImageState, loads the new one, fits the window,
 * and recalcs. Called by the IPC handler when a new file arrives.
 */
bool image_replace(PrevueApp *app, const char *abs_path);

#endif /* IMAGE_H */