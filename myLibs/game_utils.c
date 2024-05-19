
#define SDL_MAIN_HANDLED



#include "vec2.c"
#include "arraylist.c" // stdio, stdlib
#include "color.c"
#include <stdarg.h>
#include <math.h>
#include <SDL.h>


#define RENDERER_FLAGS (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
#define EPSILON 0.001
#define u64 uint64_t
#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t

#define in_range(a, min, max) (a <= max && a >= min) 
#define min(a, b) a < b ? a : b
#define max(a, b) a > b ? a : b

#define init_grid(type, rows, cols, default, result) do { \
    *result = malloc(sizeof(type *) * rows); \
    for (int i = 0; i < rows; i++) { \
        (*result)[i] = malloc(sizeof(type) * cols); \
        for (int j = 0; j < cols; j++) { \
            (*result)[i][j] = default; \
        } \
    } \
} while (0)

typedef struct SortObject {
    void *val;
    double num;
} SortObject;


bool canPrint = false;
const double DEG_TO_RAD = PI / 180;
const double RAD_TO_DEG = 180 / PI;

// // testing 
// int main(int argc, char **argv) {
//     if (TTF_Init() == -1) {
//         printf("Couldn't init TTF! \n");
//     }
//     printf("Test \n");
//     main2(argc, argv);
// }

double mili_to_sec(u64 mili) {
    return (double)mili / 1000;
}

char *concat(char *s1, char *s2) {
    char *result = malloc(sizeof(char) * (strlen(s1) + strlen(s2)));

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

    SDL_Texture *texture = SDL_CreateTextureFromSurface(
        renderer,
        surface
    );

    SDL_FreeSurface(surface);

    return texture;
}

void decimalToText(double decimal, char *buf) {
    gcvt(decimal, 4, buf);
}

double loop_clamp(double num, double min, double max) {
    double result = num;
    while (result > max) result -= max - min;
    while (result < min) result += max - min;

    return result;
}

// SDL_Texture *make_text_texture(SDL_Renderer *renderer, char *text) {
//     SDL_Surface surface = TTF_
// }

v2 getTextureSize(SDL_Texture *texture) {
    int x, y;
    SDL_QueryTexture(texture, NULL, NULL, &x, &y);

    return (v2){x, y};
}

void cdPrint(bool activateCooldown, const char *text, ...) {
    if (!canPrint) return;
    if (activateCooldown) canPrint = false;
    
    va_list args;
    int done;

    va_start(args, text);
    done = vprintf(text, args);
    va_end(args);
}

double clamp(double num, double min, double max) {
    if (num < min) return min;
    if (num > max) return max;
    return num;
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

// check how far 'mid' is along a and b
double inverse_lerp(double a, double b, double mid) {
    // mid = a + (b - a) * w
    // solve for w
    // mid - a = (b - a) * w
    // w = (mid - a)/(b - a)
    return (mid - a) / (b - a);
}

bool isPointInRect(v2 point, v2 rectPos, v2 rectSize) {
    return (in_range(point.x, rectPos.x, rectPos.x + rectSize.x) && in_range(point.y, rectPos.y, rectPos.y + rectSize.y));
}


// Merge sort

// Merges the arrays in a sorted fashion
SortObject *merge(SortObject *arr1, SortObject *arr2, int l1, int l2) {
    SortObject *res = malloc(sizeof(SortObject) * (l1 + l2));
    int arr1Idx = 0;
    int arr2Idx = 0;
    int i = 0;
    while (arr1Idx < l1 && arr2Idx < l2) {
        if (arr1[arr1Idx].num < arr2[arr2Idx].num) {
            res[i] = arr1[arr1Idx];
            arr1Idx++;
        } else {
            res[i] = arr2[arr2Idx];
            arr2Idx++;
        }
        i++;
    }
    while (arr1Idx < l1) {
        res[i] = arr1[arr1Idx];
        arr1Idx++;
        i++;
    }
    while (arr2Idx < l2) {
        res[i] = arr2[arr2Idx];
        arr2Idx++;
        i++;
    }

    return res;
}

SortObject *mergeSort(SortObject arr[], int arrlen) {
    if (arrlen == 1) {
        SortObject *res = malloc(sizeof(SortObject));
        res[0] = arr[0];
        return res;
    }
    int l1 = arrlen / 2;
    int l2 = arrlen % 2 == 0 ? arrlen / 2 : arrlen / 2 + 1;
    SortObject a1[l1];
    SortObject a2[l2];
    for (int i = 0; i < arrlen; i++) {
        if (i < l1) {
            a1[i] = arr[i];
        } else {
            a2[i - l1] = arr[i];
        }
    }

    SortObject *a = mergeSort(a1, l1);
    SortObject *b = mergeSort(a2, l2);
    
    SortObject *sorted = merge(a, b, l1, l2);

    free(a);
    free(b);

    return sorted;

}

