#ifndef IPC_H
#define IPC_H

#include "prevue.h"

/* IPC major command codes */
#define IPC_CMD_OPEN  1   /* payload: absolute file path (no NUL) */
#define IPC_CMD_ACK 2

/**
 * Try to connect to a running Prevue instance and send it abs_path.
 * Returns true if the message was delivered — caller should exit(0).
 * Returns false if no instance is running — caller should become the server.
 */
bool ipc_client_send(const char *abs_path);

/**
 * Start the IPC server for this instance.
 * Incoming OPEN commands call image_replace() on app.
 */
void ipc_server_init(PrevueApp *app);

/**
 * Shut down the IPC server. Called from _win_del_cb.
 */
void ipc_server_shutdown(PrevueApp *app);

#endif /* IPC_H */