#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <stdio.h>
#include "array.c"
#include <stdbool.h>
#include <math.h>
#include "inttypes.h"
#include "cd_print.c"
#include <math.h>

#define BS_AUDIO_FREQ 44100

typedef struct Sound {
    u8 *data;
    u32 data_len;
    bool active;
    u64 current_sample;
    bool loop;

    // BETWEEN 0 AND 1
    double volume_multiplier;
} Sound;

SDL_AudioSpec BS_audio_spec = {0}, BS_obtained = {0};
SDL_AudioDeviceID BS_device;

Sound **BS_active_sounds = NULL;



s16 get_next_sample(Sound *sound) {

    s16 *s16_data = (s16 *)sound->data;

    sound->current_sample++; // if it aint broke dont fix it :)

    if (sound->current_sample >= sound->data_len / sizeof(s16)) {
        if (sound->loop) {
            sound->current_sample = 0;
        } else {
            sound->active = false;
            return BS_audio_spec.silence;
        }
        
    }

    return s16_data[sound->current_sample] * sound->volume_multiplier;
}

void audio_callback(void *userdata, Uint8 *stream, int len) {
    
    int samples = len / sizeof(s16);
    s16 *buf = (s16 *)stream;

    for (int i = 0; i < samples; i++) {

        s16 *sample = ((s16 *)stream) + i;
        s16 sum = 0;

        for (int j = 0; j < array_length(BS_active_sounds); j++) {

            Sound *current_sound = BS_active_sounds[j];

            if (!current_sound->active) continue;

            // cd_print(false, "Time: %d \n", SDL_GetTicks64() - current_sound->time_at_start);

            s16 current_sample = get_next_sample(current_sound);
            sum += current_sample;
        }

        ((s16 *)stream)[i] = sum;
    }
}

void BS_init() {

    SDL_Init(SDL_INIT_AUDIO);

    BS_active_sounds = array(Sound *, 10);

    BS_obtained = (SDL_AudioSpec){0};
    BS_audio_spec = (SDL_AudioSpec){
        .callback = audio_callback,
        .freq = 44100,
        .format = AUDIO_S16,
        .channels = 1,
        .samples = 512
    };

    BS_device = SDL_OpenAudioDevice(NULL, 0, &BS_audio_spec, &BS_obtained, 0);

    if (BS_device == 0) {
        printf("Failed to open audio device. %s \n", SDL_GetError());
        
        exit(1);
    }
    printf("Opened audio device. \n");
    printf("Better sounds initialized! \n");

    SDL_PauseAudioDevice(BS_device, 0);


}

Sound *create_sound(const char *filename) {
    SDL_AudioSpec audio_spec = {0};
    u8 *data = {0};
    u32 data_len = 0;
    
    if (SDL_LoadWAV(filename, &BS_audio_spec, &data, &data_len) == NULL) {
        fprintf(stderr, "Could not open %s: %s\n", filename, SDL_GetError());
        return NULL;
    }

    Sound *sound = malloc(sizeof(Sound));
    if (sound == NULL) {
        fprintf(stderr, "Could not allocate memory for Sound\n");
        SDL_FreeWAV(data);
        return NULL;
    }

    sound->data = data;
    sound->data_len = data_len;
    sound->active = false;
    sound->volume_multiplier = 1;
    sound->current_sample = 0;
    sound->loop = false;

    array_append(BS_active_sounds, sound);
    
    return sound;
}

Sound *play_sound(Sound *sound) {
    sound->active = true;
    sound->current_sample = 0;
}

Sound *pause_sound(Sound *sound) {
    sound->active = false;
}

Sound *unpause_sound(Sound *sound) {
    if (sound->current_sample < sound->data_len / sizeof(s16)) {
        sound->active = true;
    }
}