#include <SDL.h>
#include <SDL_ttf.h>
#include "vec2.c"
#include <stdbool.h>
#include "array.c"

typedef struct UIComponent {

    v2 pos, size;
    int z_index;
    struct UIComponent **children;
    bool visible;
    void (*on_click)(bool);
    void (*render)();

} UIComponent;

typedef struct UILabel {
    UIComponent component;
    char *text;
} UILabel;

void init_ui() {
    TTF_Init();
}

UIComponent *UIComponent_new(v2 pos, v2 size) {
    UIComponent *component = malloc(sizeof(UIComponent));
    component->children = array(UIComponent *, 10);
    component->size = size;
    component->pos = pos;
    component->on_click = NULL;
    component->render = NULL;
    component->z_index = 0;
    component->visible = true;

    return component;
}

