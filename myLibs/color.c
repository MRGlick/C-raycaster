
typedef struct Color {
    int r, g, b, a;
} Color;

Color Color_lerp(Color a, Color b, double w) {
    return (Color){a.r + (b.r - a.r) * w,
                    a.g + (b.g - a.g) * w,
                    a.b + (b.b - a.b) * w,
                    a.a + (b.a - a.a) * w
    };
}

const Color WHITE = (Color){255, 255, 255, 255},
            BLACK = (Color){0, 0, 0, 255},
            RED = (Color){255, 0, 0, 255},
            BLUE = (Color){0, 0, 255, 255},
            YELLOW = (Color){255, 255, 0, 255},
            GREEN = (Color){0, 255, 0, 255},
            GRAY = (Color){150, 150, 150, 255},
            PURPLE = (Color){255, 0, 255, 255};

