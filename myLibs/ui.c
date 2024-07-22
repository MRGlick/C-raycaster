#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_gpu.h>
#include "vec2.c"
#include <stdbool.h>
#include "array.c"
#include "arraylist.c"


typedef enum TextAlignment {
    ALIGNMENT_CENTER,
    ALIGNMENT_LEFT,
    ALIGNMENT_RIGHT,
    ALIGNMENT_TOP,
    ALIGNMENT_BOTTOM
} TextAlignment;


typedef struct UIStyle {
    SDL_Color bg_color, fg_color;
} UIStyle;

typedef struct UIComponent {
    v2 pos, size;
    int z_index;
    struct UIComponent **children;
    struct UIComponent *parent;
    bool visible;
    void (*render)(GPU_Target *, struct UIComponent *);
    void (*update)(struct UIComponent *);

    bool isbutton;

    bool contains_mouse;

    GPU_Image *texture;

    UIStyle current_style, default_style;

} UIComponent;

typedef struct UILabel {
    UIComponent component;
    char *text;
    TTF_Font *font;
    TextAlignment alignment_x;
    TextAlignment alignment_y;
} UILabel;

typedef struct UIButton {
    UILabel label;
    void (*on_click)(struct UIComponent *, bool);
    bool activate_on_release;

    UIStyle hover_style, pressed_style;
} UIButton;


#define UI_alloc(type) ({type *ptr; ptr = malloc(sizeof(type)); (*ptr) = type##_new(); ptr;})





TTF_Font *default_font;
int default_font_ptsize = 30;
bool _ui_initialized = false;
bool _is_mouse_down = false;
arraylist *_components;

void UIComponent_render(GPU_Target *target, UIComponent *component);



// #START

v2 _get_mouse_pos() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return (v2){x, y};
}

bool is_point_in_rect(v2 point, v2 rect_pos, v2 rect_size) {
    return point.x >= rect_pos.x && point.x <= rect_pos.x + rect_size.x && point.y >= rect_pos.y && point.y <= rect_pos.y + rect_size.y;
}

void UIButton_update(UIComponent *comp) {
    UIButton *button = comp;

    UIStyle style;

    if (comp->contains_mouse) {

        if (_is_mouse_down) {
            style = button->pressed_style;
        } else {
            style = button->hover_style;
        }
    } else {
        style = comp->default_style;
    }

    comp->current_style = style;

    UILabel_update(comp);
}

void _UI_mouse_motion(SDL_MouseMotionEvent event) {
    
    v2 mouse_pos = _get_mouse_pos();
    

    for (int i = 0; i < _components->length; i++) {
        UIComponent *comp = arraylist_get_val(_components, i);
        
        bool contains_mouse = is_point_in_rect(mouse_pos, comp->pos, comp->size);
        
        if (comp->contains_mouse != contains_mouse) {
            comp->contains_mouse = contains_mouse;
            comp->update(comp);
        }
    }
}

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

UIStyle UIStyle_new() {
    UIStyle style;
    style.bg_color = (SDL_Color){30, 30, 30, 255};
    style.fg_color = (SDL_Color){210, 210, 210, 255};


    return style;
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
    component.default_style = UIStyle_new();
    component.current_style = component.default_style;

    component.contains_mouse = false;

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
    
    printf("Updated! \n");

    UILabel *label = component;
    

    SDL_Surface *text_surface = TTF_RenderText_Blended(label->font, label->text, component->current_style.fg_color);

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

    SDL_Color col = component->current_style.bg_color;

    SDL_FillRect(main_surface, NULL, SDL_MapRGBA(main_surface->format, col.r, col.g, col.b, col.a));

    SDL_Rect text_dst = {
        0,
        0,
        text_surface->w,
        text_surface->h
    };

    switch (label->alignment_x) {
        case ALIGNMENT_LEFT:
            text_dst.x = 0;
            break;
        case ALIGNMENT_CENTER:
            text_dst.x = component->size.x / 2 - text_surface->w / 2;
            break;
        case ALIGNMENT_RIGHT:
            text_dst.x = component->size.x - text_surface->w;
            break;
    }

    switch (label->alignment_y) {
        case ALIGNMENT_TOP:
            text_dst.y = 0;
            break;
        case ALIGNMENT_CENTER:
            text_dst.y = component->size.y / 2 - text_surface->h / 2;
            break;
        case ALIGNMENT_BOTTOM:
            text_dst.y = component->size.y - text_surface->h;
            break;
    }


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
    label.alignment_x = ALIGNMENT_LEFT;
    label.alignment_y = ALIGNMENT_TOP;

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
    button.label.component.update = UIButton_update;
    button.on_click = default_on_click;
    button.activate_on_release = true;

    button.hover_style = UIStyle_new();

    button.hover_style.bg_color = (SDL_Color){60, 60, 60, 255};

    button.pressed_style = UIStyle_new();

    button.pressed_style.bg_color = (SDL_Color){100, 100, 100, 255};
    button.pressed_style.fg_color = (SDL_Color){255, 255, 255, 255};

    return button;
}

void UI_add_component(UIComponent *component) {
    arraylist_add(_components, component, -1);
}



void _UI_mouse_click(SDL_MouseButtonEvent event, bool pressed) {

    _is_mouse_down = pressed;

    v2 mouse_pos = (v2){event.x, event.y};

    for (int i = 0; i < _components->length; i++) {
        UIComponent *component = arraylist_get_val(_components, i);
        if (!component->isbutton) continue;
        UIButton *button = component;
        bool should_activate = button->activate_on_release == !pressed;
        if (is_point_in_rect(mouse_pos, button->label.component.pos, button->label.component.size)) {
            button->label.component.update(button);
            
            if (button->on_click != NULL && should_activate) {
                button->on_click(button, pressed);
            }
            
            // button might move inside on_click, but im not dealing with that right now bc its unlikely
            // button->label.component.contains_mouse = is_point_in_rect(mouse_pos, button->label.component.pos, button->label.component.size);
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
        case SDL_MOUSEMOTION:
            _UI_mouse_motion(event.motion);
    }
}




arraylist *UI_get_components() {
    return _components;
}


// #END