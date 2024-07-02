
#define SDL_MAIN_HANDLED

#include "sounds.c"

double max(double a, double b) {
    return a > b? a : b;
}

int main() {

    SDL_Init(SDL_INIT_AUDIO);
    Sound *test_sound = create_sound("Sounds/test_sine2.wav");
    Sound *test_sound2 = create_sound("Sounds/test_triangle_16bit.wav");
    for (int i = 0; i < 10; i++) {
        test_sound->volume_multiplier = 1;
        if (i % 2 == 0) {
            play_sound(test_sound, 1);
        } else {
            play_sound(test_sound2, 1);
        }
        SDL_Delay(800);
    }
    SDL_Delay(1000);
}