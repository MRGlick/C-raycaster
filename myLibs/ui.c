#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_gpu.h>
#include "vec2.c"
#include <stdbool.h>
#include "array.c"
#include "arraylist.c"

typedef struct UIComponent {
    v2 pos, size;
    int z_index;
    struct UIComponent **children;
    struct UIComponent *parent;
    bool visible;
    void (*render)(GPU_Target *, struct UIComponent *);
    void (*update)(struct UIComponent *);

    bool isbutton;

    GPU_Image *texture;

} UIComponent;

typedef struct UILabel {
    UIComponent component;
    char *text;
    TTF_Font *font;
} UILabel;

typedef struct UIButton {
    UILabel label;
    void (*on_click)(struct UIComponent *, bool);
    bool activate_on_release;
} UIButton;


#define UI_alloc(type) ({type *ptr; ptr = malloc(sizeof(type)); (*ptr) = type##_new(); ptr;})





TTF_Font *default_font;
int default_font_ptsize = 30;
bool _ui_initialized = false;
bool _is_mouse_down = false;
arraylist *_components;





void UIComponent_render(GPU_Target *target, UIComponent *component);

void UI_init() {
    if (_ui_initialized) {
        printf("UI already initialized! \n");
        return;
    }
    _ui_initialized = true;
    TTF_Init();

    default_font = TTF_OpenFont("FFFFORWA.TTF", default_font_ptsize);

    _components = create_arraylist(10);
}



UIComponent UIComponent_new() {
    UIComponent component;
    component.children = array(UIComponent *, 10);
    component.parent = NULL;
    component.size = to_vec(100);
    component.pos = V2_ZERO;
    component.update = NULL;
    component.render = UIComponent_render;
    component.z_index = 0;
    component.visible = true;
    component.texture = NULL;
    component.isbutton = false;

    return component;
}

bool UIComponent_should_render(UIComponent *component) {
    
    if (!_ui_initialized) {
        printf("Error! UI not initialized! \n");
        exit(1);
        return false;
    }

    if (component->parent == NULL)
        return component->visible;

    return component->visible && UIComponent_should_render(component->parent);

}

void UILabel_update(UIComponent *component) {
    
    UILabel *label = component;
    
    SDL_Surface *text_surface = TTF_RenderText_Blended(label->font, label->text, ((SDL_Color){255, 255, 255, 255}));

    SDL_Surface *main_surface = SDL_CreateRGBSurface(
        0, 
        max(component->size.x, text_surface->w), 
        max(component->size.y, text_surface->h), 
        32, 
        0xFF000000, 
        0xFF0000, 
        0xFF00, 
        0xFF
    );

    SDL_Rect text_dst = {
        0,
        0,
        text_surface->w,
        text_surface->h
    };

    SDL_BlitSurface(text_surface, NULL, main_surface, &text_dst);

    GPU_Image *texture = GPU_CopyImageFromSurface(main_surface);

    SDL_FreeSurface(text_surface);
    SDL_FreeSurface(main_surface);

    label->component.texture = texture;

    if (label->component.texture == NULL) {
        printf("Error! texture is null! \n");
    }

    if (component->children != NULL) {
        for (int i = 0; i < array_length(component->children); i++) {
            component->children[i]->update(component->children[i]);
        }
    }
}

void UIComponent_render(GPU_Target *target, UIComponent *component) {
    if (!UIComponent_should_render(component)) return;

    GPU_Rect rect = {
        component->pos.x,
        component->pos.y,
        component->size.x,
        component->size.y
    };

    GPU_BlitRect(component->texture, NULL, target, &rect);
}

UILabel UILabel_new() {
    UILabel label;
    label.component = UIComponent_new();
    label.component.update = UILabel_update;
    label.text = "Text here";
    label.font = default_font;

    return label;
}

UIComponent UIComponent_add_child(UIComponent *parent, UIComponent *child) {
    array_append(parent->children, child);
    child->parent = parent;
}

void default_on_click(UIComponent *component, bool pressed) {
    printf("Default click event! Pressed: %d \n", pressed);
}

UIButton UIButton_new() {

    UIButton button;

    button.label = UILabel_new();
    button.label.component.isbutton = true;
    button.on_click = default_on_click;
    button.activate_on_release = false;

    return button;
}

void UI_add_component(UIComponent *component) {
    arraylist_add(_components, component, -1);
}

bool is_point_in_rect(v2 point, v2 rect_pos, v2 rect_size) {
    return point.x >= rect_pos.x && point.x <= rect_pos.x + rect_size.x && point.y >= rect_pos.y && point.y <= rect_pos.y + rect_size.y;
}

void _UI_mouse_click(SDL_MouseButtonEvent event, bool pressed) {

    printf("Click! \n");
    v2 mouse_pos = (v2){event.x, event.y};

    for (int i = 0; i < _components->length; i++) {
        UIComponent *component = arraylist_get_val(_components, i);
        if (!component->isbutton) continue;
        UIButton *button = component;
        bool should_activate = button->activate_on_release == !pressed;
        if (button->on_click != NULL && should_activate && is_point_in_rect(mouse_pos, button->label.component.pos, button->label.component.size)) {
            button->on_click(button, pressed);
        }
    }
}

void UI_handle_event(SDL_Event event) {
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            _UI_mouse_click(event.button, true);
            break;
        case SDL_MOUSEBUTTONUP:
            _UI_mouse_click(event.button, false);
            break;
    }
}

v2 _get_mouse_pos() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return (v2){x, y};
}


arraylist *UI_get_components() {
    return _components;
}


// #END