
#define SDL_MAIN_HANDLED




#include <stdarg.h>
#include <math.h>
#include <SDL.h>
#include "vec2.c"
#include "arraylist.c" // stdio, stdlib
#include "color.c"
#include "sounds.c"

#define RENDERER_FLAGS (SDL_RENDERER_ACCELERATED)
#define EPSILON 0.001
#define u64 uint64_t
#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t

#define in_range(a, min, max) (a <= max && a >= min)

#ifndef min

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#endif

#define new(type) malloc(sizeof(type))

#define debug_crash() *(int *)NULL = 42

#define init_grid(type, rows, cols, default, result) do { \
    result = malloc(sizeof(type *) * rows); \
    for (int i = 0; i < rows; i++) { \
        result[i] = malloc(sizeof(type) * cols); \
        for (int j = 0; j < cols; j++) { \
            result[i][j] = default; \
        } \
    } \
} while (0)

typedef struct SortObject {
    void *val;
    double num;
} SortObject;

typedef struct TextureData { // for anything that involves writing to one big texture
    int *pixels;
    int w, h;
} TextureData;

typedef struct Pixel {
    Uint8 r, g, b, a;
} Pixel;

double last_print_time;
const double DEG_TO_RAD = PI / 180;
const double RAD_TO_DEG = 180 / PI;


void init_cd_print() {
    last_print_time = SDL_GetTicks64();
}

Pixel TextureData_get_pixel(TextureData *data, int x, int y) {
    int p = data->pixels[y * data->w + x];

    // 0xRRGGBBAA

    Uint8 r = p >> 24;
    Uint8 g = p >> 16;
    Uint8 b = p >> 8;
    Uint8 a = p;

    return (Pixel){r, g, b, a};
}

TextureData *TextureData_from_bmp(char *bmp_file) {
    SDL_Surface* surface = SDL_LoadBMP(bmp_file);
    if (!surface) {
        fprintf(stderr, "SDL_LoadBMP Error: %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    // Allocate memory for the pixel data
    int* pixels = (int*)malloc(surface->w * surface->h * sizeof(int));
    if (!pixels) {
        fprintf(stderr, "Memory allocation failed\n");
        SDL_FreeSurface(surface);
        return NULL;
    }

    // Access pixel data from the surface
    int pitch = surface->pitch;
    Uint32* pixel_data = (Uint32*)surface->pixels;
    SDL_PixelFormat* format = surface->format;

    for (int y = 0; y < surface->h; ++y) {
        for (int x = 0; x < surface->w; ++x) {
            Uint32 pixel = pixel_data[y * (pitch / 4) + x]; // pitch is in bytes, divide by 4 for Uint32
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
            pixels[y * surface->w + x] = (r << 24) | (g << 16) | (b << 8) | a; // 0xRRGGBBAA
        }
    }

    TextureData *data = malloc(sizeof(TextureData));
    data->pixels = pixels;
    data->w = surface->w;
    data->h = surface->h;

    SDL_FreeSurface(surface);

    return data;
}

double mili_to_sec(u64 mili) {
    return (double)mili / 1000;
}


char *concat(char *s1, char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    if (result == NULL) {
        printf("Malloc failed for concat. \n");
        return NULL;
    }

    strcpy(result, s1);
    strcat(result, s2);

    return result;

}

// inclusive
int randi_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

double randf() {
    return ((double)rand()) / RAND_MAX;
}

double randf_range(double min, double max) {
    return min + randf() * (max - min);
}

SDL_Texture *make_texture(SDL_Renderer *renderer, char *bmp_file) {
    SDL_Surface *surface = SDL_LoadBMP(bmp_file);
    if (surface == NULL) {
        printf("%s path: '%s'\n", SDL_GetError(), bmp_file);
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(
        renderer,
        surface
    );

    SDL_FreeSurface(surface);

    return texture;
}

void decimal_to_text(double decimal, char *buf) {
    gcvt(decimal, 4, buf);
}

double loop_clamp(double num, double min, double max) {
    double result = num;
    while (result > max) result -= max - min;
    while (result < min) result += max - min;

    return result;
}

v2 get_texture_size(SDL_Texture *texture) {
    int x, y;
    SDL_QueryTexture(texture, NULL, NULL, &x, &y);

    return (v2){x, y};
}

void cd_print(bool activate_cooldown, const char *text, ...) {
    u64 now  = SDL_GetTicks64();
    if (mili_to_sec(now - last_print_time) < 0.01) return;
    if (activate_cooldown) last_print_time = now;

    va_list args;
    int done;

    va_start(args, text);
    done = vprintf(text, args);
    va_end(args);
}



double rad_to_deg(double radians) {
    return radians * RAD_TO_DEG;
}
double deg_to_rad(double degrees) {
    return degrees * DEG_TO_RAD;
}

double lerp(double a, double b, double w) {
    return a + (b - a) * w;
}

double inverse_lerp(double a, double b, double mid) {
    return (mid - a) / (b - a);
}

bool is_point_in_rect(v2 point, v2 rect_pos, v2 rect_size) {
    return (in_range(point.x, rect_pos.x, rect_pos.x + rect_size.x) && in_range(point.y, rect_pos.y, rect_pos.y + rect_size.y));
}


// Merge sort

// Merges the arrays in a sorted fashion
SortObject *merge(SortObject *arr1, SortObject *arr2, int l1, int l2) {
    SortObject *res = malloc(sizeof(SortObject) * (l1 + l2));
    int i1 = 0;
    int i2 = 0;
    int i = 0;
    while (i1 < l1 && i2 < l2) {
        if (arr1[i1].num < arr2[i2].num) {
            res[i] = arr1[i1];
            i1++;
        } else {
            res[i] = arr2[i2];
            i2++;
        }
        i++;
    }
    while (i1 < l1) {
        res[i] = arr1[i1];
        i1++;
        i++;
    }
    while (i2 < l2) {
        res[i] = arr2[i2];
        i2++;
        i++;
    }

    return res;
}

SortObject *merge_sort(SortObject arr[], int len) {
    if (len == 1) {
        SortObject *res = malloc(sizeof(SortObject));
        res[0] = arr[0];
        return res;
    }
    int l1 = len / 2;
    int l2 = len % 2 == 0 ? len / 2 : len / 2 + 1;
    SortObject a1[l1];
    SortObject a2[l2];
    for (int i = 0; i < len; i++) {
        if (i < l1) {
            a1[i] = arr[i];
        } else {
            a2[i - l1] = arr[i];
        }
    }

    SortObject *a = merge_sort(a1, l1);
    SortObject *b = merge_sort(a2, l2);
    
    SortObject *sorted = merge(a, b, l1, l2);

    free(a);
    free(b);

    return sorted;

}

int get_num_digits(int num) {
    int res = 0;
    while (num > 0) {
        num /= 10;
        res++;
    }
    return res;
}

void play_spatial_sound(Sound *sound, double base_volume, v2 listener_pos, v2 sound_pos, double sound_max_radius) {
    double dist = v2_distance(listener_pos, sound_pos);
    if (dist > sound_max_radius) return;

    double volume_multiplier = (1 - inverse_lerp(0, sound_max_radius, dist)) * SDL_clamp(base_volume, 0, 1);

    volume_multiplier = SDL_clamp(volume_multiplier, 0, 1);

    volume_multiplier *= volume_multiplier; // to make it more realistic ig

    play_sound(sound, volume_multiplier);
}

// #END