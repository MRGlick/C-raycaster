#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define FADE_SAMPLE_COUNT 2000
#define SOUND_REDUCTION_FACTOR 0.25
#define VOLUME_REDUCTION_THRESHOLD 28000

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

// debug only
Sound *prev_sound1 = NULL;
Sound *prev_sound2 = NULL;

double _max(double a, double b) {
    return a > b? a : b;
}

Sound *create_sound(const char *filename) {
    SDL_AudioSpec audio_spec;
    u8 *data;
    u32 data_len;
    
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

    if (audio_spec.freq != DESIRED_SPEC.freq || audio_spec.format != DESIRED_SPEC.format || audio_spec.channels != DESIRED_SPEC.channels) {

        printf("Audio has unsupported format! Attempting to convert to 16 bit signed 44100 Hz \n");

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
    } else {
        sound->data = data;
    }

    
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
    return sound->data_len / 2; // assuming 16 bit
}

int sign(int x) {
    return (x > 0) - (0 > x);
}

void apply_volume_s16(const u8 *base, u8 *dst, int len, double volume_multiplier) { // works
    for (int i = 0; i < len; i += 2) {
        int16_t sample = *((int16_t *)&base[i]);

        sample *= volume_multiplier;

        *(int16_t *)&dst[i] = sample;
    }
}

void _queue_sound(Sound *sound) { // works
    u8 volume_scaled_data[sound->data_len];
    apply_volume_s16(sound->data, volume_scaled_data, sound->data_len, sound->volume_multiplier);

    if (SDL_QueueAudio(sound->device, volume_scaled_data, sound->data_len) != 0) {
        fprintf(stderr, "Could not queue audio: %s\n", SDL_GetError());
    }
    SDL_PauseAudioDevice(sound->device, 0); // Use SDL_PauseAudioDevice to control specific device

    u64 now = SDL_GetTicks64();
    sound->time_at_start = now;

    last_played_sound = sound;
}




void play_sound(Sound *sound, double volume_multiplier) {

    volume_multiplier = SDL_clamp(volume_multiplier, 0, 1);

    sound->volume_multiplier = volume_multiplier;

    if (last_played_sound == NULL) {
        prev_sound1 = NULL;
        prev_sound2 = sound;

        _queue_sound(sound);
        return;
    }

    u64 ticks_passed_since_lps = (double)(SDL_GetTicks64() - last_played_sound->time_at_start);

    int lps_current_sample = last_played_sound->audio_spec.freq * ticks_passed_since_lps / 1000;

    if (lps_current_sample % 2 == 1) lps_current_sample--; // WHAT THE ACTUAL FUCK


    int lps_data_len = last_played_sound->data_len;

    if (lps_current_sample >= lps_data_len) {

        prev_sound1 = NULL;
        prev_sound2 = sound;
        _queue_sound(sound);
        return;
    }

    // ugh guess we gotta do the thing

    int current_sound_data_len = sound->data_len;

    int new_data_len = _max(lps_data_len - lps_current_sample, current_sound_data_len);

    double volume_factor = 1;

    // for (int i = lps_current_sample; i < lps_current_sample + new_data_len; i++) {
    //     if (i - lps_current_sample < sound->data_len && i < lps_sample_count) {
            
    //         s16 current_sound_sample = *(s16 *)&sound->data[i - lps_current_sample];
    //         s16 lps_sample = *(s16 *)&last_played_sound->data[i];

    //         int sample = current_sound_sample * sound->volume_multiplier + lps_sample * last_played_sound->volume_multiplier;
    //         if (abs(sample) >= VOLUME_REDUCTION_THRESHOLD) {
    //             volume_factor = SOUND_REDUCTION_FACTOR;
    //             break;
    //         }
    //     }
    // }

    u8 *new_data = malloc(new_data_len);
    for (int i = lps_current_sample; i < lps_current_sample + new_data_len; i += 2) {
        s64 sample = 0;
        if (i - lps_current_sample < sound->data_len) {
            s16 current_sound_sample = *(s16 *)&sound->data[i - lps_current_sample];
        
            sample += (s64)current_sound_sample * sound->volume_multiplier;
        }
        if (i < lps_data_len) {
            s16 lps_sample = *(s16 *)&last_played_sound->data[i];
            sample += (s64)lps_sample * last_played_sound->volume_multiplier;
        }

        sample = SDL_clamp(sample * volume_factor, INT16_MIN + 1, INT16_MAX - 1);

        *(s16 *)&new_data[i - lps_current_sample] = sample;
    }

    prev_sound1 = last_played_sound;
    prev_sound2 = sound;

    Sound *final = malloc(sizeof(Sound)); // DONT FREE THE OLD DATA
    final->data = new_data;
    final->data_len = new_data_len;
    final->device = sound->device;
    final->time_at_start = (double)SDL_GetTicks64() / 1000;
    final->audio_spec = sound->audio_spec;
    final->volume_multiplier = 1;

    SDL_PauseAudio(1);
    SDL_ClearQueuedAudio(final->device);
    _queue_sound(final);

}

void free_sound(Sound *sound) {
    if (sound) {
        SDL_FreeWAV(sound->data);
        // SDL_CloseAudioDevice(sound->device); // Close the audio device
        free(sound);

    }
}

void set_audio_paused(Sound *sound, bool p) {
    SDL_PauseAudioDevice(sound->device, p); // Use SDL_PauseAudioDevice to control specific device
}

// #END