#include "vigem_manager.h"
#include "rumble_manager.h"
#include "logger.h"

bool vigemEnsureClient(AppContext *app)
{
    VIGEM_ERROR vigem_error;
    if (!app) return false;
    if (app->vigem_client_ready) return true;
    app->vigem_client = vigem_alloc();
    if (!app->vigem_client) {
        logWrite("vigem", "vigem_alloc failed");
        return false;
    }
    vigem_error = vigem_connect(app->vigem_client);
    if (!VIGEM_SUCCESS(vigem_error)) {
        logWrite("vigem", "vigem_connect failed: 0x%08X", (unsigned int)vigem_error);
        vigem_free(app->vigem_client);
        app->vigem_client = NULL;
        return false;
    }
    app->vigem_client_ready = true;
    logWrite("vigem", "client initialized");
    return true;
}

bool vigemEnsurePad(AppContext *app)
{
    VIGEM_ERROR vigem_error;
    if (!app) return false;
    if (app->vigem_pad) return true;
    if (!vigemEnsureClient(app)) return false;
    app->vigem_pad = vigem_target_x360_alloc();
    if (!app->vigem_pad) {
        logWrite("vigem", "vigem_target_x360_alloc failed");
        return false;
    }
    vigem_error = vigem_target_add(app->vigem_client, app->vigem_pad);
    if (!VIGEM_SUCCESS(vigem_error)) {
        logWrite("vigem", "vigem_target_add failed: 0x%08X", (unsigned int)vigem_error);
        vigem_target_free(app->vigem_pad);
        app->vigem_pad = NULL;
        return false;
    }
    vigem_error = vigem_target_x360_register_notification(app->vigem_client, app->vigem_pad, rumbleHandleXboxCallback, app);
    if (!VIGEM_SUCCESS(vigem_error)) {
        logWrite("vigem", "register rumble callback failed: 0x%08X", (unsigned int)vigem_error);
        app->vigem_notification_registered = false;
    } else {
        app->vigem_notification_registered = true;
    }
    logWrite("vigem", "virtual Xbox 360 controller created");
    return true;
}

bool vigemSubmitReport(AppContext *app, const XUSB_REPORT *report)
{
    VIGEM_ERROR vigem_error;

    if (!app || !report || !app->vigem_client || !app->vigem_pad) {
        return false;
    }

    vigem_error = vigem_target_x360_update(app->vigem_client, app->vigem_pad, *report);
    if (!VIGEM_SUCCESS(vigem_error)) {
        logWrite("vigem", "vigem_target_x360_update failed: 0x%08X", (unsigned int)vigem_error);
        vigemDestroyPad(app);
        return false;
    }

    return true;
}

void vigemDestroyPad(AppContext *app)
{
    if (!app || !app->vigem_pad) return;
    if (app->vigem_client) {
        if (app->vigem_notification_registered) {
            vigem_target_x360_unregister_notification(app->vigem_pad);
            app->vigem_notification_registered = false;
        }
        vigem_target_remove(app->vigem_client, app->vigem_pad);
    }
    vigem_target_free(app->vigem_pad);
    app->vigem_pad = NULL;
    logWrite("vigem", "virtual Xbox 360 controller removed");
}

void vigemShutdown(AppContext *app)
{
    if (!app) return;
    vigemDestroyPad(app);
    if (app->vigem_client) {
        vigem_disconnect(app->vigem_client);
        vigem_free(app->vigem_client);
        app->vigem_client = NULL;
    }
    app->vigem_client_ready = false;
    app->vigem_notification_registered = false;
}
