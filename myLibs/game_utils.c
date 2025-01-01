
#define SDL_MAIN_HANDLED




#include <stdarg.h>
#include <math.h>
#include <SDL.h>
#include "vec2.c"
#include "arraylist.c" // stdio, stdlib
#include "color.c"
#include "bettersounds.c"
#include <SDL_gpu.h>
#include <sys/time.h>
#include "hashtable.c"
#include "inttypes.h"

#define RENDERER_FLAGS (SDL_RENDERER_ACCELERATED)
#define EPSILON 0.001

#define in_range(a, min, max) (a <= max && a >= min)

#ifndef min

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#endif

// self_defstruct
// kill_yourself_NOW
// overdose_on_...
// commit_sudoku

#define float_equal(a, b) in_range(a, b - EPSILON, b + EPSILON)

#define commit_sudoku() *(int *)NULL = 42

#define init_grid(type, rows, cols, default, result) do { \
    result = malloc(sizeof(type *) * rows); \
    for (int i = 0; i < rows; i++) { \
        result[i] = malloc(sizeof(type) * cols); \
        for (int j = 0; j < cols; j++) { \
            result[i][j] = default; \
        } \
    } \
} while (0)

#define Color(r, g, b, a) (SDL_Color){r, g, b, a}

#define foreach(vardecl, list, l, ...) for (int other_name_i_wont_use = 0; other_name_i_wont_use < l; other_name_i_wont_use++) { \
    vardecl = list[other_name_i_wont_use]; \
    __VA_ARGS__ \
}

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


const double DEG_TO_RAD = PI / 180;
const double RAD_TO_DEG = 180 / PI;

double lerp(double a, double b, double w) {
    return a + (b - a) * w;
}

double inverse_lerp(double a, double b, double mid) {
    return (mid - a) / (b - a);
}


void hsv_to_rgb(double h, double s, double v, int *r_out, int *g_out, int *b_out) {

    double r = 0, g = 0, b = 0;

    h = fmod(h, 360);

    if (h < 60) {
        r = 1;
        g = lerp(0, 1, h / 60);
        b = 0;
    } else if (h < 120) {
        g = 1;
        b = 0;
        r = lerp(1, 0, (h - 60) / 60);
    } else if (h < 180) {
        r = 0;
        g = 1;
        b = lerp(0, 1, (h - 120) / 60);
    } else if (h < 240) {
        r = 0;
        b = 1;
        g = lerp(1, 0, (h - 180) / 60);
    } else if (h < 300) {
        g = 0;
        b = 1;
        r = lerp(0, 1, (h - 240) / 60);
    } else {
        r = 1;
        g = 0;
        b = lerp(1, 0, (h - 300) / 60);
    }


    r = lerp(r, 1, 1 - s);
    g = lerp(g, 1, 1 - s);
    b = lerp(b, 1, 1 - s);

    r *= v;
    g *= v;
    b *= v;

    *r_out = r * 255;
    *g_out = g * 255;
    *b_out = b * 255;
}

void rgb_to_hsv(int r, int g, int b, double *h_out, double *s_out, double *v_out) {
    double h, s, v;


    double dr = (double)r / 255, dg = (double)g / 255, db = (double)b / 255;
    
    double max_val = max(dr, max(dg, db));
    double min_val = min(dr, min(dg, db));

    s = (max_val - min_val) / max_val;
    v = max_val;

    double r2 = (dr) / v;
    double g2 = (dg) / v;
    double b2 = (db) / v;

    r2 = (r2 - 1 + s) / s;
    g2 = (g2 - 1 + s) / s;
    b2 = (b2 - 1 + s) / s;

    if (float_equal(r2, 1) && float_equal(b2, 0)) {
        h = inverse_lerp(0, 1, g2) * 60;

    } else if (float_equal(g2, 1) && float_equal(b2, 0)) {
        h = inverse_lerp(1, 0, r2) * 60 + 60;

    } else if (float_equal(r2, 0) && float_equal(g2, 1)) {
        h = inverse_lerp(0, 1, b2) * 60 + 120;

    } else if (float_equal(r2, 0) && float_equal(b2, 1)) {
        h = inverse_lerp(1, 0, g2) * 60 + 180;

    } else if (float_equal(g2, 0) && float_equal(b2, 1)) {
        h = inverse_lerp(0, 1, r2) * 60 + 240;

    } else {
        h = inverse_lerp(1, 0, b2) * 60 + 300;
    }

    *h_out = h;
    *s_out = s;
    *v_out = v;

}



v2 get_screen_size() {
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(0, &mode);
    return (v2){mode.w, mode.h};
}

int sign(int x) {
    return (x > 0) - (0 > x);
}

u64 get_systime_mili() {
	struct timeval tp;

	gettimeofday(&tp, NULL);
	return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

void randomize() {
    srand(get_systime_mili());
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

TextureData *TextureData_from_png(char *file) {

    

    SDL_Surface* surface = GPU_LoadSurface(file);
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

GPU_Image *load_texture(char *file) {

    GPU_Image *image = GPU_LoadImage(file);
    if (image == NULL) {
        fprintf(stderr, "Failed to load image! File: '%s' \n", file);
    }

    GPU_SetImageFilter(image, GPU_FILTER_NEAREST);

    return image;
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

v2 get_texture_size(GPU_Image *texture) {
    return (v2){texture->w, texture->h};
}




double rad_to_deg(double radians) {
    return radians * RAD_TO_DEG;
}
double deg_to_rad(double degrees) {
    return degrees * DEG_TO_RAD;
}


#ifndef IS_POINT_IN_RECT_FUNC
#define IS_POINT_IN_RECT_FUNC
bool is_point_in_rect(v2 point, v2 rect_pos, v2 rect_size) {
    return (in_range(point.x, rect_pos.x, rect_pos.x + rect_size.x) && in_range(point.y, rect_pos.y, rect_pos.y + rect_size.y));
}
#endif


// Merge sort
// Completely destroyed by qsort()
// Will probably never touch this bc memory

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

#ifndef GET_NUM_DIGITS
#define GET_NUM_DIGITS
int get_num_digits(int num) {
    int res = 0;
    while (num > 0) {
        num /= 10;
        res++;
    }
    return res;
}
#endif

void play_spatial_sound(Sound *sound, double base_volume, v2 listener_pos, v2 sound_pos, double sound_max_radius) {
    double dist = v2_distance(listener_pos, sound_pos);
    if (dist > sound_max_radius) return;

    double volume_multiplier = (1 - inverse_lerp(0, sound_max_radius, dist)) * SDL_clamp(base_volume, 0, 1);

    volume_multiplier = SDL_clamp(volume_multiplier, 0, 1);

    volume_multiplier *= volume_multiplier; // to make it more realistic ig

    sound->volume_multiplier = volume_multiplier;

    play_sound(sound);
}

void shuffle_array(int *arr, int l) {
    for (int i = 0; i < l; i++) {
        int rand_idx = randi_range(0, i);

        int temp = arr[i];
        arr[i] = arr[rand_idx];
        arr[rand_idx] = temp;
    }
}

// #END