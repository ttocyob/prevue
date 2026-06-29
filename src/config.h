#ifndef CONFIG_H
#define CONFIG_H

#include "prevue.h"

/**
 * Load config from EET file.
 * Always returns a valid ConfigData* (zeroed defaults on first run
 * or if the file is missing/corrupt).
 */
ConfigData *config_load(void);

/**
 * Persist current window position to EET file.
 * Silent no-op if app->win is not realised.
 */
void config_save(PrevueApp *app);

#endif /* CONFIG_H */