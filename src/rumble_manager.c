#include "rumble_manager.h"
#include "input_actions.h"

static float applyMasterStrength(const AppContext *app, UCHAR rumble)
{
    if (!app || rumble == 0) return 0.0f;
    return (float)rumble * app->rumble_config.master_strength;
}

void rumbleSetMove(PSMove *move, UCHAR rumble)
{
    if (!move) return;
    psmove_set_rumble(move, rumble);
    psmove_update_leds(move);
}

UCHAR rumbleAdaptMotor(const AppContext *app, UCHAR motor_value, const RumbleMotorConfig *cfg)
{
    int rumble;
    float scaled;
    if (!app || !app->rumble_config.enabled || !cfg) return 0;
    rumble = (int)motor_value;
    if (rumble <= 0) return 0;
    if (rumble < cfg->min_active) return 0;
    if (rumble < cfg->low_threshold) scaled = rumble * cfg->boost_low;
    else if (rumble < cfg->mid_threshold) scaled = rumble * cfg->boost_mid;
    else scaled = rumble * cfg->boost_high;
    if (scaled < (float)cfg->min_output) scaled = (float)cfg->min_output;
    scaled = applyMasterStrength(app, (UCHAR)scaled);
    return clampToByte((int)(scaled + 0.5f));
}

UCHAR rumbleCombineValues(UCHAR large_rumble, UCHAR small_rumble)
{
    int combined = (int)large_rumble;
    if ((int)small_rumble > combined) combined = (int)small_rumble;
    return (UCHAR)combined;
}

VOID CALLBACK rumbleHandleXboxCallback(PVIGEM_CLIENT client, PVIGEM_TARGET target, UCHAR large_motor, UCHAR small_motor, UCHAR led_number, LPVOID user_data)
{
    AppContext *app = (AppContext *)user_data;
    UCHAR rumble_large;
    UCHAR rumble_small;
    UCHAR combined;
    (void)client;
    (void)target;
    (void)led_number;
    if (!app || !app->emulation_enabled) return;
    rumble_large = rumbleAdaptMotor(app, large_motor, &app->rumble_config.large);
    rumble_small = rumbleAdaptMotor(app, small_motor, &app->rumble_config.small);
    if (app->moves[0] && app->moves[1]) {
        rumbleSetMove(app->moves[0], rumble_large);
        rumbleSetMove(app->moves[1], rumble_small);
        return;
    }
    combined = rumbleCombineValues(rumble_large, rumble_small);
    if (app->moves[0]) {
        rumbleSetMove(app->moves[0], app->rumble_config.combine_when_single_move ? combined : rumble_large);
    }
    if (app->moves[1]) {
        rumbleSetMove(app->moves[1], app->rumble_config.combine_when_single_move ? combined : rumble_small);
    }
}
