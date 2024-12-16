
#pragma once

#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>

#include "inttypes.h"

double last_print_time;
bool cd_print_initialized = false;

void init_cd_print() {
    last_print_time = SDL_GetTicks64();
    cd_print_initialized = true;
}

void cd_print(bool activate_cooldown, const char *text, ...) {

    if (!cd_print_initialized) {
        init_cd_print();
    }

    u64 now  = SDL_GetTicks64();
    if (now - last_print_time < 10) return;
    if (activate_cooldown) last_print_time = now;

    va_list args;
    int done;

    va_start(args, text);
    done = vprintf(text, args);
    va_end(args);
}