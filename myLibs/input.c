#include <SDL.h>
#include <stdbool.h>



#define INPUT(name) SDL_SCANCODE_##name
#define MOUSE(name) SDL_BUTTON_##name
#define explode() *(int *)0 = 9

typedef void (*KeyCallback)(SDL_Scancode, bool);
typedef void (*MouseButtonCallback)(int, bool);
typedef void (*MouseMotionCallback)(SDL_MouseMotionEvent);
typedef void (*QuitCallback)();

KeyCallback _IN_key_pressed = NULL;
MouseButtonCallback _IN_mouse_pressed = NULL;
MouseMotionCallback _IN_mouse_motion = NULL;
QuitCallback _IN_quit = NULL;

bool _IN_inputs[SDL_NUM_SCANCODES] = {0};
bool _IN_mouse_buttons[5] = {0};

bool is_mouse_button_pressed(int b) {
    if (b < 0 || b > 4) {
        explode();
    }

    return _IN_mouse_buttons[b];
}

bool is_key_pressed(SDL_Scancode scancode) {
    if (scancode < 0 || scancode > 511) {
        explode();
    }

    return _IN_inputs[scancode];
}



void _IN_default_quit() {
    exit(0);
}



void IN_init(KeyCallback pressed_callback, MouseButtonCallback mouse_callback, MouseMotionCallback mouse_motion_callback, QuitCallback quit_callback) {
    _IN_key_pressed = pressed_callback;
    _IN_mouse_pressed = mouse_callback;
    _IN_mouse_motion = mouse_motion_callback;
    _IN_quit = quit_callback == NULL? _IN_default_quit : quit_callback;
    
}

void IN_handle_input(SDL_Event event) {
    if (event.type == SDL_KEYDOWN) {
        
        if (!_IN_inputs[event.key.keysym.scancode]) {
            printf("bruih \n");
            _IN_key_pressed(event.key.keysym.scancode, true);
            _IN_inputs[event.key.keysym.scancode] = true;
        }
    } else if (event.type == SDL_KEYUP) {

        if (_IN_inputs[event.key.keysym.scancode]) {
            _IN_key_pressed(event.key.keysym.scancode, false);
            _IN_inputs[event.key.keysym.scancode] = false;
        }

    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (!_IN_mouse_buttons[event.button.button]) {
            _IN_mouse_buttons[event.button.button] = true;
            _IN_mouse_pressed(event.button.button, true);
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (_IN_mouse_buttons[event.button.button]) {
            _IN_mouse_buttons[event.button.button] = false;
            _IN_mouse_pressed(event.button.button, false);
        }
    } else if (event.type == SDL_MOUSEMOTION) {

        _IN_mouse_motion(event.motion);
    }
    
    else if (event.type == SDL_QUIT) {

        _IN_quit();
    }
}