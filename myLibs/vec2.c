#ifndef VEC2
#define VEC2


#include <math.h>
#include <stdio.h>

typedef struct v2 {
    double x;
    double y;
} v2;

#define PI 3.1415926535

#define V2_ZERO ((v2){0, 0})
#define V2_LEFT ((v2){-1, 0})
#define V2_UP ((v2){0, -1})
#define V2_DOWN ((v2){0, 1})

#define V2_RIGHT ((v2){1, 0})
#define to_vec(a) ((v2){a, a})

#define V2(a, b) (v2){a, b}

#define min(a, b) a < b ? a : b
#define max(a, b) a > b ? a : b

// double sin_table[10800];
// double cos_table[10800];
// double tan_table[10800];
// int initialized_fast_trig = 0;
// const double DEG_TO_RAD = PI / 180;
// const double RAD_TO_DEG = 180 / PI;





// double init_fast_trig() {
//     for (int i = 0; i < 10800; i++) {
//         double angle = deg_to_rad((double)i / 30);
//         sin_table[i] = sin(angle);
//         cos_table[i] = cos(angle);
//         tan_table[i] = tan(angle);
//     }
//     initialized_fast_trig = 1;
// }


// double fast_sin(double angleRads) {
//     int idx = (int)(rad_to_deg(angleRads) * 30);
//     return sin_table[(int)clamp(idx, 0, 10800 - 1)];
// }

// double fast_cos(double angleRads) {
//     int idx = (int)(rad_to_deg(angleRads) * 30);
//     return cos_table[(int)clamp(idx, 0, 10800 - 1)];    
// }

// double fast_tan(double angleRads) {
//     int idx = (int)(rad_to_deg(angleRads) * 30);
//     return tan_table[(int)clamp(idx, 0, 10800 - 1)];
// }

// double default_sin(double angleRads) {
//     if (initialized_fast_trig) {
//         return fast_sin(angleRads);
//     }
//     return sin(angleRads);
// }

// double default_cos(double angleRads) {
//     if (initialized_fast_trig) {
//         return fast_cos(angleRads);
//     }
//     return cos(angleRads);
// }

// double default_tan(double angleRads) {
//     if (initialized_fast_trig) {
//         return fast_tan(angleRads);
//     }
//     return tan(angleRads);
// }
double clamp(double num, double _min, double _max) {
    if (num < _min) return _min;
    if (num > _max) return _max;
    return num;
}

v2 v2_add(v2 a, v2 b) {
    return (v2){a.x + b.x, a.y + b.y};
}
v2 v2_sub(v2 a, v2 b) {
    return (v2){a.x - b.x, a.y - b.y};
}
v2 v2_mul(v2 a, v2 b) {
    return (v2){a.x * b.x, a.y * b.y};
}
v2 v2_div(v2 a, v2 b) {
    return (v2){a.x / b.x, a.y / b.y};
}

v2 v2_lerp(v2 a, v2 b, double w) {
    return (v2){a.x + (b.x - a.x) * w, a.y + (b.y - a.y) * w};
}

double v2_dot(v2 a, v2 b) {
    return a.x * b.x + a.y * b.y;
}

double v2_length(v2 vec) {
    return sqrt(vec.x * vec.x + vec.y * vec.y);
}

double v2_cos_angle_between(v2 a, v2 b) {
    return v2_dot(a, b) / (v2_length(a) * v2_length(b));
}


double v2_angle_between(v2 a, v2 b) {
    double lengthA = v2_length(a);
    double lengthB = v2_length(b);
    if (lengthA * lengthB == 0) return 0;
    return acos(v2_dot(a, b) / (lengthA * lengthB));
}


double v2_signed_angle_between(v2 a, v2 b) {
    return atan2(a.x * b.y - b.x * a.y, a.x * b.x + a.y * b.y);
}

double v2_length_squared(v2 vec) {
    return vec.x * vec.x + vec.y * vec.y;
}

double v2_distance_squared(v2 vec1, v2 vec2) {
    return v2_length_squared(v2_sub(vec2, vec1));
}

double v2_distance(v2 vec1, v2 vec2) {
    return v2_length(v2_sub(vec2, vec1));  
}

v2 v2_normalize(v2 vec) {
    double l = v2_length(vec);
    return (v2){vec.x / l, vec.y / l};
}

int v2_equal(v2 a, v2 b) {
    return a.x == b.x && a.y == b.y;
}

v2 v2_dir(v2 vec1, v2 vec2) {
    if (v2_equal(vec1, vec2)) return (v2){1, 0}; // what else is there to do?
    return v2_normalize(v2_sub(vec2, vec1));
}

v2 v2_floor(v2 vec) {
    return (v2){floor(vec.x), floor(vec.y)};
}

v2 v2_ceil(v2 vec) {
    return (v2){ceil(vec.x), ceil(vec.y)};
}

v2 v2_limit_length(v2 vec, double thresh) {
    double length = v2_length(vec);
    if (length < thresh) return vec;
    return v2_mul(v2_div(vec, (v2){length, length}), (v2){thresh, thresh}); // normalize and multiply by thresh
}



// Get the slope of the linear function the points create.
double v2_get_slope(v2 a, v2 b) {
    if (a.x == b.x) return INFINITY;
    return (b.y - a.y) / (b.x - a.x);
}



v2 v2_mode(v2 vecs[], int size) {
    if (size == 0) return V2_ZERO;
    v2 currentMode = vecs[0];
    int count = 1;
    for (int i = 1; i < size; i++) {
        if (!v2_equal(vecs[i], currentMode)) {
            count--;
            if (count <= 0) {
                currentMode = vecs[i];
                count = 1;
            }
        }
    }
    return currentMode;
}

// project A onto B.
v2 v2_proj(v2 a, v2 b) {
    return v2_mul(b, to_vec(v2_dot(a, b)));
}

double v2_get_angle(v2 vec) {
    if (vec.x != 0 && vec.y == 0) {
        return vec.x > 0 ? 0 : PI;
    }
    // } else if (vec.x == 0 && vec.y != 0) {
    //     return vec.y > 0 ? PI * 1.5 : PI / 2;
    // }
    return atan2(vec.y, vec.x);
}

// Returns a vector with the length of the provided vector and the angle of the provided angle.
v2 v2_rotate_to(v2 vec, double angle) {    
    double l = v2_length(vec);
    return (v2){l * cos(angle), l * sin(angle)};
}

v2 v2_rotate(v2 vec, double angle) {

    double cosAngle = cos(angle);
    double sinAngle = sin(angle);

    return (v2){vec.x * cosAngle - vec.y * sinAngle, vec.x * sinAngle + vec.y * cosAngle};
}

void v2_print(v2 vec, char *postfix) {
    printf("v2(%.2f, %.2f)", vec.x, vec.y);
    printf(postfix);
}


// slide 'a' across the normal 'b'
v2 v2_slide(v2 a, v2 b) {
    return v2_proj(a, v2_rotate(b, PI/2));
}

v2 v2_clamp(v2 vec, v2 minVec, v2 maxVec) {
    v2 result = {
        clamp(vec.x, minVec.x, maxVec.x),
        clamp(vec.y, minVec.y, maxVec.y)
    };

    return result;
}

v2 v2_get_random_dir() {
    double random = (double)rand() / RAND_MAX;
    return v2_rotate((v2){1, 0}, random * 2 * PI);
}

v2 v2_reflect(v2 vector, v2 normal) {
    return v2_sub(vector, v2_mul(to_vec(2 * v2_dot(vector, normal)), normal));
}

// #END

#endif // VEC2