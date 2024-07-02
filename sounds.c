#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define FADE_SAMPLE_COUNT 2000

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;


typedef struct Sound {
    u8 *data;
    u32 data_len;
    SDL_AudioDeviceID device;
    SDL_AudioSpec audio_spec;
    u64 time_at_start;

    // BETWEEN 0 AND 1
    double volume_multiplier;
} Sound;

const SDL_AudioSpec DESIRED_SPEC = {
    .format = AUDIO_S16,
    .channels = 1,
    .freq = 44100
};

Sound *last_played_sound = NULL;

double _max(double a, double b) {
    return a > b? a : b;
}

Sound *create_sound(const char *filename) {
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

    SDL_AudioStream *stream = SDL_NewAudioStream(
        audio_spec.format,
        audio_spec.channels,
        audio_spec.freq,
        DESIRED_SPEC.format,
        DESIRED_SPEC.channels,
        DESIRED_SPEC.freq
    );

    SDL_AudioStreamPut(stream, data, data_len);

    u8 *converted = malloc(data_len * sizeof(u8));

    SDL_AudioStreamGet(stream, converted, data_len);

    SDL_AudioStreamFlush(stream);

    SDL_FreeAudioStream(stream);

    stream = NULL;

    sound->data = converted;
    sound->data_len = data_len;
    sound->audio_spec = DESIRED_SPEC;
    sound->device = device;

    
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

int sign(int x) {
    return x > 0 - 0 > x;
}

void apply_volume_u8(const u8 *base, u8 *dst, int len, double volume_multiplier) {
    printf("Applying volume for u8 \n");
    for (int i = 0; i < len; i++) {
        dst[i] = base[i] * volume_multiplier;
    }
}

void apply_volume_s8(const u8 *base, u8 *dst, int len, double volume_multiplier) {
    printf("Applying volume for s8 \n");
    for (int i = 0; i < len; i++) {
        int8_t sample = (int8_t)base[i] * volume_multiplier;
        dst[i] = *(u8 *)&sample;
    }
}

void apply_volume_u16(const u8 *base, u8 *dst, int len, double volume_multiplier) {
    printf("Applying volume for u16 \n");
    for (int i = 0; i < len; i += 2) {
        u16 sample = *((u16 *)&base[i]);

        sample *= volume_multiplier;
        
        *(u16 *)&dst[i] = sample;
    }
}

void apply_volume_s16(const u8 *base, u8 *dst, int len, double volume_multiplier) {
    printf("Applying volume for s16 \n");
    for (int i = 0; i < len; i += 2) {
        int16_t sample = *((int16_t *)&base[i]);

        sample *= volume_multiplier;

        *(int16_t *)&dst[i] = sample;
    }
}

void apply_volume_f32(const u8 *base, u8 *dst, int len, double volume_multiplier) {
    printf("Applying volume for 32 bit floatager \n");
    for (int i = 0; i < len; i += 4) {
        u32 f32_bit_rep = *(u32 *)&base[i];
        float f = *(float *)&f32_bit_rep;
        f *= volume_multiplier;

        u32 updated_bit_rep = *(u32 *)&f;
        *(u32 *)&base[i] = updated_bit_rep;
    }
}

void apply_volume_32_bit(const u8 *base, u8 *dst, int len, double volume_multiplier) {
    printf("Applying volume for 32 bitager \n");
    for (int i = 0; i < len; i += 4) {
        u32 sample = *(u32 *)&base[i];
        sample *= volume_multiplier;
        *(u32 *)&base[i] = sample;
    }
}

void _queue_sound(Sound *sound) {

    u64 now = SDL_GetTicks64();

    sound->time_at_start = now;

    u8 volume_scaled_data[sound->data_len];
    
    // determine format so i can scale it properly
    SDL_AudioFormat format = sound->audio_spec.format;
        
    apply_volume_s16(sound->data, volume_scaled_data, sound->data_len, sound->volume_multiplier);

    if (SDL_QueueAudio(sound->device, volume_scaled_data, sound->data_len) != 0) {
        fprintf(stderr, "Could not queue audio: %s\n", SDL_GetError());
    }

    SDL_PauseAudioDevice(sound->device, 0); // Use SDL_PauseAudioDevice to control specific device

    last_played_sound = sound;
}

void play_sound(Sound *sound, double volume_multiplier) {

    volume_multiplier = SDL_clamp(volume_multiplier, 0, 1);

    sound->volume_multiplier = volume_multiplier;

    if (last_played_sound == NULL) {
        _queue_sound(sound);
        return;
    }

    u64 now = SDL_GetTicks64();


    double time_passed_since_lps = (double)(now - last_played_sound->time_at_start) / 1000;

    int lps_current_sample = last_played_sound->audio_spec.freq * time_passed_since_lps;

    int lps_sample_count = get_sample_count(last_played_sound);

    if (lps_current_sample >= lps_sample_count) {
        last_played_sound = NULL;
        _queue_sound(sound);
        return;
    }

    // ugh guess we gotta do the thing

    int current_sound_data_len = get_sample_count(sound);

    int new_data_len = _max(lps_sample_count - lps_current_sample, current_sound_data_len);

    int abs_loudest = -1;

    for (int i = lps_current_sample; i < lps_current_sample + new_data_len; i += 2) {
        
        int current_sound_sample = *(s16 *)&sound->data[i - lps_current_sample];
        
        int sample = current_sound_sample * sound->volume_multiplier;
        if (i < lps_sample_count) {
            int lps_sample = *(s16 *)&last_played_sound->data[i];
            sample += lps_sample * last_played_sound->volume_multiplier;
        }

        if (abs(sample) > abs_loudest) {
            abs_loudest = abs(sample);
        }
    }

    u8 *new_data = malloc(new_data_len);
    for (int i = lps_current_sample; i < lps_current_sample + new_data_len; i += 2) {
        int current_sound_sample = *(s16 *)&sound->data[i - lps_current_sample];
        
        int sample = current_sound_sample * sound->volume_multiplier;
        if (i < lps_sample_count) {
            int lps_sample = *(s16 *)&last_played_sound->data[i];
            sample += lps_sample * last_played_sound->volume_multiplier;
        }

        sample = (s64)sample * INT16_MAX / abs_loudest;

        *(s16 *)&new_data[i - lps_current_sample] = sample;
    }

    Sound *final = malloc(sizeof(Sound)); // DONT FORGET TO FREE THE OLD DATA
    final->data = new_data;
    final->data_len = new_data_len;
    final->device = sound->device;
    final->time_at_start = now;
    final->audio_spec = sound->audio_spec; // ..ehhh...
    final->volume_multiplier = _max(sound->volume_multiplier, last_played_sound->volume_multiplier); // ...ehhhhhhh..

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