#ifdef CAPI_SDL2

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL.h>

// Analog camera movement by Pathétique (github.com/vrmiguel), y0shin and Mors
// Contribute or communicate bugs at github.com/vrmiguel/sm64-analog-camera

#include <ultra64.h>

#include "controller_api.h"
#include "controller_sdl.h"
#include "../configfile.h"
#include "../platform.h"
#include "../fs/fs.h"

#include "game/level_update.h"

// mouse buttons are also in the controller namespace (why), just offset 0x100
#define VK_OFS_SDL_MOUSE 0x0100
#define VK_BASE_SDL_MOUSE (VK_BASE_SDL_GAMEPAD + VK_OFS_SDL_MOUSE)
#define MAX_JOYBINDS 32
#define MAX_MOUSEBUTTONS 8 // arbitrary

extern int16_t rightx;
extern int16_t righty;

#ifdef BETTERCAMERA
int mouse_x;
int mouse_y;

extern u8 newcam_mouse;
#endif

static bool init_ok;
static bool haptics_enabled;
static SDL_GameController *sdl_cntrl;
static SDL_Haptic *sdl_haptic;
static SDL_Joystick *sdl_joystick;

static u32 num_joy_binds = 0;
static u32 num_mouse_binds = 0;
static u32 joy_binds[MAX_JOYBINDS][2];
static u32 mouse_binds[MAX_JOYBINDS][2];

static bool joy_buttons[SDL_CONTROLLER_BUTTON_MAX ] = { false };
static u32 mouse_buttons = 0;
static u32 last_mouse = VK_INVALID;
static u32 last_joybutton = VK_INVALID;

static inline void controller_add_binds(const u32 mask, const u32 *btns) {
    for (u32 i = 0; i < MAX_BINDS; ++i) {
        if (btns[i] >= VK_BASE_SDL_GAMEPAD && btns[i] <= VK_BASE_SDL_GAMEPAD + VK_SIZE) {
            if (btns[i] >= VK_BASE_SDL_MOUSE && num_joy_binds < MAX_JOYBINDS) {
                mouse_binds[num_mouse_binds][0] = btns[i] - VK_BASE_SDL_MOUSE;
                mouse_binds[num_mouse_binds][1] = mask;
                ++num_mouse_binds;
            } else if (num_mouse_binds < MAX_JOYBINDS) {
                joy_binds[num_joy_binds][0] = btns[i] - VK_BASE_SDL_GAMEPAD;
                joy_binds[num_joy_binds][1] = mask;
                ++num_joy_binds;
            }
        }
    }
}

static void controller_sdl_bind(void) {
    bzero(joy_binds, sizeof(joy_binds));
    bzero(mouse_binds, sizeof(mouse_binds));
    num_joy_binds = 0;
    num_mouse_binds = 0;

    controller_add_binds(A_BUTTON,     configKeyA);
    controller_add_binds(B_BUTTON,     configKeyB);
    controller_add_binds(Z_TRIG,       configKeyZ);
    controller_add_binds(STICK_UP,     configKeyStickUp);
    controller_add_binds(STICK_LEFT,   configKeyStickLeft);
    controller_add_binds(STICK_DOWN,   configKeyStickDown);
    controller_add_binds(STICK_RIGHT,  configKeyStickRight);
    controller_add_binds(U_CBUTTONS,   configKeyCUp);
    controller_add_binds(L_CBUTTONS,   configKeyCLeft);
    controller_add_binds(D_CBUTTONS,   configKeyCDown);
    controller_add_binds(R_CBUTTONS,   configKeyCRight);
    controller_add_binds(L_TRIG,       configKeyL);
    controller_add_binds(R_TRIG,       configKeyR);
    controller_add_binds(START_BUTTON, configKeyStart);
}

static void controller_sdl_init(void) {
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return;
    }

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    haptics_enabled = (SDL_InitSubSystem(SDL_INIT_HAPTIC) == 0);

    // try loading an external gamecontroller mapping file
    uint64_t gcsize = 0;
    void *gcdata = fs_load_file("gamecontrollerdb.txt", &gcsize);
    if (gcdata && gcsize) {
        SDL_RWops *rw = SDL_RWFromConstMem(gcdata, gcsize);
        if (rw) {
            int nummaps = SDL_GameControllerAddMappingsFromRW(rw, SDL_TRUE);
            if (nummaps >= 0)
                printf("loaded %d controller mappings from 'gamecontrollerdb.txt'\n", nummaps);
        }
        free(gcdata);
    }

#ifdef BETTERCAMERA
    if (newcam_mouse == 1)
        SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
#endif

    controller_sdl_bind();

    init_ok = true;
}

static SDL_Haptic *controller_sdl_init_haptics(const int joy) {
    if (!haptics_enabled) return NULL;

    SDL_Haptic *hap = SDL_HapticOpen(joy);
    if (!hap) return NULL;

    if (SDL_HapticRumbleSupported(hap) != SDL_TRUE) {
        SDL_HapticClose(hap);
        return NULL;
    }

    if (SDL_HapticRumbleInit(hap) != 0) {
        SDL_HapticClose(hap);
        return NULL;
    }

    printf("controller %s has haptics support, rumble enabled\n", SDL_JoystickNameForIndex(joy));
    return hap;
}

static void controller_sdl_read(OSContPad *pad) {
    if (!init_ok) {
        return;
    }

#ifdef BETTERCAMERA
    if (newcam_mouse == 1 && sCurrPlayMode != 2)
        SDL_SetRelativeMouseMode(SDL_TRUE);
    else
        SDL_SetRelativeMouseMode(SDL_FALSE);
    
    u32 mouse = SDL_GetRelativeMouseState(&mouse_x, &mouse_y);

    for (u32 i = 0; i < num_mouse_binds; ++i)
        if (mouse & SDL_BUTTON(mouse_binds[i][0]))
            pad->button |= mouse_binds[i][1];

    // remember buttons that changed from 0 to 1
    last_mouse = (mouse_buttons ^ mouse) & mouse;
    mouse_buttons = mouse;
#endif

    if (sdl_joystick == NULL) {
        sdl_joystick = SDL_JoystickOpen(0);
    }
    if (sdl_joystick == NULL) {
        return;
    }

    SDL_JoystickUpdate();

    int16_t leftx = SDL_JoystickGetAxis(sdl_joystick, 1);
    int16_t lefty = SDL_JoystickGetAxis(sdl_joystick, 0);

    uint32_t magnitude_sq = (uint32_t)(leftx * leftx) + (uint32_t)(lefty * lefty);
    uint32_t stickDeadzoneActual = configStickDeadzone * DEADZONE_STEP;
    if (magnitude_sq > (uint32_t)(stickDeadzoneActual * stickDeadzoneActual)) {
        pad->stick_x = leftx / 0x100;
        int stick_y = -lefty / 0x100;
        pad->stick_y = stick_y == 128 ? 127 : stick_y;
    }
}

static void controller_sdl_rumble_play(f32 strength, f32 length) {
    if (sdl_haptic)
        SDL_HapticRumblePlay(sdl_haptic, strength, (u32)(length * 1000.0f));
}

static void controller_sdl_rumble_stop(void) {
    if (sdl_haptic)
        SDL_HapticRumbleStop(sdl_haptic);
}

static u32 controller_sdl_rawkey(void) {
    if (last_joybutton != VK_INVALID) {
        const u32 ret = last_joybutton;
        last_joybutton = VK_INVALID;
        return ret;
    }

    for (int i = 0; i < MAX_MOUSEBUTTONS; ++i) {
        if (last_mouse & SDL_BUTTON(i)) {
            const u32 ret = VK_OFS_SDL_MOUSE + i;
            last_mouse = 0;
            return ret;
        }
    }
    return VK_INVALID;
}

static void controller_sdl_shutdown(void) {
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        if (sdl_cntrl) {
            SDL_GameControllerClose(sdl_cntrl);
            sdl_cntrl = NULL;
        }
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }

    if (SDL_WasInit(SDL_INIT_HAPTIC)) {
        if (sdl_haptic) {
            SDL_HapticClose(sdl_haptic);
            sdl_haptic = NULL;
        }
        SDL_QuitSubSystem(SDL_INIT_HAPTIC);
    }

    haptics_enabled = false;
    init_ok = false;
}

struct ControllerAPI controller_sdl = {
    VK_BASE_SDL_GAMEPAD,
    controller_sdl_init,
    controller_sdl_read,
    controller_sdl_rawkey,
    controller_sdl_rumble_play,
    controller_sdl_rumble_stop,
    controller_sdl_bind,
    controller_sdl_shutdown
};

#endif // CAPI_SDL2
