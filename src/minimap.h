#ifndef PREVUE_MINIMAP_H
#define PREVUE_MINIMAP_H

#include "prevue.h"

/**
 * Show the minimap and update the thumbnail image + viewport indicator
 * to reflect the current zoom and pan state in app->img.
 * Creates the minimap on first call (lazy init).
 */
void minimap_update(PrevueApp *app);

/**
 * Hide the minimap overlay. Does not destroy it.
 */
void minimap_hide(PrevueApp *app);

/**
 * Destroy the minimap and free MinimapData.
 * Called from _win_del_cb.
 */
void minimap_free(PrevueApp *app);

#endif /* PREVUE_MINIMAP_H */