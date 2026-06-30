#include "ipc.h"
#include "image.h"
#include <Ecore_Ipc.h>

typedef struct {
    Eina_Bool    *ack;
    Ecore_Timer **timer;
} ClientWaitCtx;

static Ecore_Ipc_Server        *_server  = NULL;
static Ecore_Event_Handler     *_hdl_add = NULL;
static Ecore_Event_Handler     *_hdl_del = NULL;
static Ecore_Event_Handler     *_hdl_dat = NULL;

/* ------------------------------------------------------------------ */
/* Server-side callbacks                                                */
/* ------------------------------------------------------------------ */

static Eina_Bool
_client_add_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
    return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_client_del_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
    return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_client_data_cb(void *data, int type EINA_UNUSED, void *event)
{
    PrevueApp *app = data;
    Ecore_Ipc_Event_Client_Data *e = event;

    if (e->major != IPC_CMD_OPEN) return ECORE_CALLBACK_PASS_ON;
    if (!e->data || e->size <= 0)  return ECORE_CALLBACK_PASS_ON;

    /* Ensure NUL-terminated path */
    char *path = malloc(e->size + 1);
    if (!path) return ECORE_CALLBACK_PASS_ON;
    memcpy(path, e->data, e->size);
    path[e->size] = '\0';

    fprintf(stdout, "prevue: IPC received: %s\n", path);

    image_replace(app, path);

    /* Raise the window to the top — the sender is waiting for us to appear */
    elm_win_raise(app->win);

    /* ACK — unblocks the client's main loop */
    ecore_ipc_client_send(e->client, IPC_CMD_ACK, 0, 0, 0, 0, NULL, 0);

    free(path);
    return ECORE_CALLBACK_PASS_ON;
}

/* ------------------------------------------------------------------ */
/* Client-side callbacks                                                */
/* ------------------------------------------------------------------ */

static Eina_Bool
_server_data_cb(void *data, int type EINA_UNUSED, void *event)
{
    ClientWaitCtx *ctx = data;
    Ecore_Ipc_Event_Server_Data *e = event;

    if (e->major == IPC_CMD_ACK)
    {
        *(ctx->ack) = EINA_TRUE;
        ecore_main_loop_quit();
    }
    return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_timeout_cb(void *data)
{
    Ecore_Timer **timer = data;
    *timer = NULL;   /* Ecore deletes this timer itself once we return
                       * ECORE_CALLBACK_CANCEL — null the caller's handle
                       * so ipc_client_send() doesn't try to delete it again. */
    ecore_main_loop_quit();
    return ECORE_CALLBACK_CANCEL;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

bool
ipc_client_send(const char *abs_path)
{
    Eina_Bool ack_received = EINA_FALSE;

    ecore_ipc_init();

    Ecore_Ipc_Server *srv =
        ecore_ipc_server_connect(ECORE_IPC_LOCAL_SYSTEM,
                                 IPC_SERVER_NAME, IPC_PORT, NULL);
    if (!srv)
    {
        ecore_ipc_shutdown();
        return false;   /* no instance running */
    }

    /* Flush the connection before sending */
    ecore_main_loop_iterate();

    int len = (int)strlen(abs_path);
    ecore_ipc_server_send(srv, IPC_CMD_OPEN, 0, 0, 0, 0, abs_path, len);

    /* Block until ACK or 2-second timeout */
    Ecore_Timer *timer = NULL;
    timer = ecore_timer_add(2.0, _timeout_cb, &timer);

    ClientWaitCtx ctx = { .ack = &ack_received, .timer = &timer };
    Ecore_Event_Handler *hdl =
        ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA,
                                _server_data_cb, &ctx);

    ecore_main_loop_begin();

    ecore_event_handler_del(hdl);
    if (timer) ecore_timer_del(timer);   /* NULL if _timeout_cb already
                                           * self-cancelled it via CANCEL */
    ecore_ipc_server_del(srv);
    ecore_ipc_shutdown();

    return (bool)ack_received;
}

void
ipc_server_init(PrevueApp *app)
{
    ecore_ipc_init();

    _server = ecore_ipc_server_add(ECORE_IPC_LOCAL_SYSTEM,
                                   IPC_SERVER_NAME, IPC_PORT, app);
    if (!_server)
    {
        fprintf(stderr, "prevue: warning: could not start IPC server\n");
        ecore_ipc_shutdown();
        return;
    }

    _hdl_add = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_ADD,
                                       _client_add_cb, app);
    _hdl_del = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DEL,
                                       _client_del_cb, app);
    _hdl_dat = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DATA,
                                       _client_data_cb, app);
}

void
ipc_server_shutdown(PrevueApp *app EINA_UNUSED)
{
    if (_hdl_add) { ecore_event_handler_del(_hdl_add); _hdl_add = NULL; }
    if (_hdl_del) { ecore_event_handler_del(_hdl_del); _hdl_del = NULL; }
    if (_hdl_dat) { ecore_event_handler_del(_hdl_dat); _hdl_dat = NULL; }

    if (_server)
    {
        ecore_ipc_server_del(_server);
        _server = NULL;
    }

    ecore_ipc_shutdown();
}