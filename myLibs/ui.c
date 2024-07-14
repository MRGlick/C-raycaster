#include <SDL.h>
#include <SDL_ttf.h>
#include "vec2.c"
#include <stdbool.h>
#include "array.c"

typedef struct UIComponent {
    v2 pos, size;
    int z_index;
    struct UIComponent **children;
    struct UIComponent *parent;
    bool visible;
    void (*on_click)(struct UIComponent *, bool);
    void (*render)(SDL_Renderer *, struct UIComponent *);
    void (*update)(SDL_Renderer *, struct UIComponent *);

    SDL_Texture *texture;

} UIComponent;

typedef struct UILabel {
    UIComponent component;
    char *text;
    TTF_Font *font;
} UILabel;


void UIComponent_render(SDL_Renderer *renderer, UIComponent *component);

void init_ui() {
    TTF_Init();
}

UIComponent UIComponent_new(v2 pos, v2 size) {
    UIComponent component;
    component.children = array(UIComponent *, 10);
    component.parent = NULL;
    component.size = size;
    component.pos = pos;
    component.on_click = NULL;
    component.update = NULL;
    component.render = UIComponent_render;
    component.z_index = 0;
    component.visible = true;
    component.texture = NULL;

    return component;
}

bool UIComponent_should_render(UIComponent *component) {
    
    if (component->parent == NULL)
        return component->visible;

    return component->visible && UIComponent_should_render(component->parent);

}

void UILabel_update(SDL_Renderer *renderer, UIComponent *component) {
    
    UILabel *label = component;
    
    SDL_Surface *surface = TTF_RenderText(label->font, label->text, ((SDL_Color){255, 255, 255}), ((SDL_Color){0, 0, 0, 0}));

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);

    label->component.texture = texture;

    if (component->children != NULL) {
        for (int i = 0; i < array_length(component->children); i++) {
            component->children[i]->update(renderer, component->children[i]);
        }
    }
}

void UIComponent_render(SDL_Renderer *renderer, UIComponent *component) {
    if (!UIComponent_should_render(component)) return;

    SDL_Rect rect = {
        component->pos.x,
        component->pos.y,
        component->size.x,
        component->size.y
    };

    SDL_RenderCopy(renderer, component->texture, NULL, &rect);
}

UILabel UILabel_new(v2 pos, v2 size, char *text, TTF_Font *font) {
    UILabel label;
    label.component = UIComponent_new(pos, size);
    label.component.update = UILabel_update;
    label.text = text;
    label.font = font;

    return label;
}

UIComponent add_child(UIComponent *parent, UIComponent *child) {
    array_append(parent->children, child);
    child->parent = parent;
}
