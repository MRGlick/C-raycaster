
#define SDL_MAIN_HANDLED

#include "sounds.c"
#include <time.h>

double max(double a, double b) {
    return a > b? a : b;
}

int main() {

    SDL_Init(SDL_INIT_AUDIO);
    Sound *sounds[] = {
        create_sound("Sounds/enemy_default_kill.wav"),
        create_sound("Sounds/player_default_shoot.wav"),
        create_sound("Sounds/enemy_default_hit.wav"),
        create_sound("Sounds/player_default_hurt.wav")
    };

    time_t rawtime;

    time ( &rawtime );
    srand(rawtime);

    for (int i = 0; i < 10; i++) {
        int choice = (int)((double)rand() / RAND_MAX * 4);
        play_sound(sounds[choice], 1);
        printf("Choice: %d \n", choice);
        SDL_Delay(100);    
    }
    

    
    SDL_Delay(5000);
}