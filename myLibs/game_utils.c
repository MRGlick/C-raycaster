
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
#include "reusable_threads.c"

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

const double DEG_TO_RAD = PI / 180;
const double RAD_TO_DEG = 180 / PI;

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

double lerp(double a, double b, double w) {
    return a + (b - a) * w;
}

double inverse_lerp(double a, double b, double mid) {
    return (mid - a) / (b - a);
}

#ifndef IS_POINT_IN_RECT_FUNC
#define IS_POINT_IN_RECT_FUNC
bool is_point_in_rect(v2 point, v2 rect_pos, v2 rect_size) {
    return (in_range(point.x, rect_pos.x, rect_pos.x + rect_size.x) && in_range(point.y, rect_pos.y, rect_pos.y + rect_size.y));
}
#endif

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