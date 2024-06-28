#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef struct Sound {
    u8 *data;
    u32 data_len;
    SDL_AudioDeviceID device;
    SDL_AudioSpec audio_spec;
    u64 time_at_start;
    double volume_multiplier;
} Sound;

Sound *last_played_sound = NULL;

Sound *create_sound(const char *filename, double volume_multiplier) {
    SDL_AudioSpec audio_spec;
    u8 *data;
    u32 data_len; // Use Uint32 as required by SDL_LoadWAV
    
    if (SDL_LoadWAV(filename, &audio_spec, &data, &data_len) == NULL) {
        fprintf(stderr, "Could not open %s: %s\n", filename, SDL_GetError());
        return NULL;
    }

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);
    if (device == 0) {
        fprintf(stderr, "Could not open audio device: %s\n", SDL_GetError());
        SDL_FreeWAV(data);
        return NULL;
    }

    Sound *sound = malloc(sizeof(Sound));
    if (sound == NULL) {
        fprintf(stderr, "Could not allocate memory for Sound\n");
        SDL_FreeWAV(data);
        SDL_CloseAudioDevice(device);
        return NULL;
    }

    sound->data = data;
    sound->data_len = data_len;
    sound->audio_spec = audio_spec;
    sound->device = device;
    sound->volume_multiplier = volume_multiplier;
    
    return sound;
}

void list_audio_devices() {
    int count = SDL_GetNumAudioDevices(0);
    printf("Audio devices available:\n");
    for (int i = 0; i < count; ++i) {
        printf(" - %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
    }
}

int get_sample_count(Sound *sound) {
    return sound->data_len;
}

void _queue_sound(Sound *sound) {

    u64 now = SDL_GetTicks64();

    sound->time_at_start = now;

    if (SDL_QueueAudio(sound->device, sound->data, sound->data_len) != 0) {
        fprintf(stderr, "Could not queue audio: %s\n", SDL_GetError());
    }
    SDL_PauseAudioDevice(sound->device, 0); // Use SDL_PauseAudioDevice to control specific device

    last_played_sound = sound;
}

void play_sound(Sound *sound) {

    if (last_played_sound == NULL) {
        _queue_sound(sound);
        return;
    }

    u64 now = SDL_GetTicks64();


    double time_passed_since_lps = (double)(now - last_played_sound->time_at_start) / 1000;

    int lps_current_sample = last_played_sound->audio_spec.freq * time_passed_since_lps;

    int lps_sample_count = get_sample_count(last_played_sound);
    
    printf("lps's current sample: %d out of %d \n", lps_current_sample, lps_sample_count);
    if (lps_current_sample >= lps_sample_count) {
        last_played_sound = NULL;
        _queue_sound(sound);
        return;
    }

    // ugh guess we gotta do the thing

    int current_sound_data_len = get_sample_count(sound);

    int new_data_len = max(lps_sample_count - lps_current_sample, current_sound_data_len);

    int loudest = -999999;

    for (int i = lps_current_sample; i < lps_current_sample + new_data_len; i++) {
        
        int current_sound_current_sample = i - lps_current_sample;

        int current_s;
        if (i < lps_sample_count) {
            current_s = sound->data[current_sound_current_sample] + last_played_sound->data[i];
        } else {
            current_s = sound->data[current_sound_current_sample];
        }
        
        if (abs(current_s) > loudest) loudest = current_s;
    }

    u8 *new_data = malloc(new_data_len);
    for (int i = lps_current_sample; i < lps_current_sample + new_data_len; i++) {
        
        int current_sound_current_sample = i - lps_current_sample;

        if (i < lps_sample_count) {
            new_data[i - lps_current_sample] = (int)(sound->data[current_sound_current_sample] + last_played_sound->data[i]) * 128 / abs(loudest);
        } else {
            new_data[i - lps_current_sample] = (int)sound->data[current_sound_current_sample] * 128 / abs(loudest);
        }
        
    }

    Sound *final = malloc(sizeof(Sound)); // DONT FORGET TO FREE THE OLD DATA
    final->data = new_data;
    final->data_len = new_data_len;
    final->device = sound->device;
    final->time_at_start = now;
    final->audio_spec = sound->audio_spec; // ..ehhh...

    SDL_PauseAudio(1);
    SDL_ClearQueuedAudio(final->device);
    _queue_sound(final);

}

void free_sound(Sound *sound) {
    if (sound) {
        SDL_FreeWAV(sound->data);
        SDL_CloseAudioDevice(sound->device); // Close the audio device
        free(sound);

    }
}

void set_audio_paused(Sound *sound, bool p) {
    SDL_PauseAudioDevice(sound->device, p); // Use SDL_PauseAudioDevice to control specific device
}


// #END