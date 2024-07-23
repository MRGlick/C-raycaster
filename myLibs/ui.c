
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_gpu.h>
#include "vec2.c"
#include <stdbool.h>
#include "array.c"
#include "arraylist.c"
#include "mystring.c"


#define MAX_COMPONENT_STACK_SIZE 1000
#define DEFAULT_FONT_SIZE 30

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

typedef enum StyleType {
    STYLE_DEFAULT,
    STYLE_HOVER,
    STYLE_PRESSED
} StyleType;

typedef struct UIComponent {
    bool is_root;
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

    UIStyle default_style;

    StyleType current_style_type;

} UIComponent;

typedef struct UILabel {
    UIComponent component;
    String text;
    TTF_Font *font;
    int font_size;
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

#define UI_get_comp(comp) (UIComponent *)comp

#define UI_set(type, comp, property, value) ((type *)comp)->property = value 




TTF_Font *default_font;
bool _ui_initialized = false;
bool _is_mouse_down = false;
UIComponent *root;

// #FUNC

void UI_set_global_pos(UIComponent *comp, v2 pos);

void UI_center_around_pos(UIComponent *comp, v2 pos);

void UILabel_set_alignment(UILabel *label, TextAlignment x, TextAlignment y);

void UI_set_visible(UIComponent *comp, bool visibility);

v2 UI_get_size(UIComponent *comp);
v2 UI_get_pos(UIComponent *comp);

void UI_set_pos(UIComponent *comp, v2 pos);
void UI_set_size(UIComponent *comp, v2 size);

v2 UI_get_global_pos(UIComponent *comp);

SDL_Surface *_UI_create_color_surface(int width, int height, SDL_Color color);

void UI_init();
UIStyle UIStyle_new();
UIComponent UIComponent_new();
UILabel UILabel_new();
UIButton UIButton_new();

void UIComponent_render(GPU_Target *target, UIComponent *component);
void UI_render(GPU_Target *target, UIComponent *component);

UIStyle UIComponent_get_current_style(UIComponent *component);
v2 _get_mouse_pos();

#ifndef IS_POINT_IN_RECT_FUNC
#define IS_POINT_IN_RECT_FUNC
bool is_point_in_rect(v2 point, v2 rect_pos, v2 rect_size);
#endif

void UIComponent_update(UIComponent *component);
void UILabel_update(UIComponent *component);
void UIButton_update(UIComponent *comp);

void _UI_mouse_motion(SDL_MouseMotionEvent event);
void _UI_mouse_click(SDL_MouseButtonEvent event, bool pressed);

void UI_handle_event(SDL_Event event);
void UILabel_set_text(UILabel *label, String text);

UIComponent *UI_get_root();
void UI_update(UIComponent *comp);



// #IMPL

UIStyle UIComponent_get_current_style(UIComponent *component) {
    if (component->isbutton && component->contains_mouse) {
        UIButton *button = component;
        if (_is_mouse_down) {
            return button->pressed_style;
        } else {
            return button->hover_style;
        }
    }


    return component->default_style;
}

v2 _get_mouse_pos() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return (v2){x, y};
}
#ifndef IS_POINT_IN_RECT_FUNC
#define IS_POINT_IN_RECT_FUNC
bool is_point_in_rect(v2 point, v2 rect_pos, v2 rect_size) {
    return point.x >= rect_pos.x && point.x <= rect_pos.x + rect_size.x && point.y >= rect_pos.y && point.y <= rect_pos.y + rect_size.y;
}
#endif

void UIButton_update(UIComponent *comp) {
    UIButton *button = comp;

    if (comp->contains_mouse) {

        if (_is_mouse_down) {
            comp->current_style_type = STYLE_PRESSED;
        } else {
            comp->current_style_type = STYLE_HOVER;
        }
    } else {
        comp->current_style_type = STYLE_DEFAULT;
    }

    UILabel_update(comp);
}

void _UI_mouse_motion(SDL_MouseMotionEvent event) {
    
    v2 mouse_pos = _get_mouse_pos();
    
    UIComponent *component_stack[MAX_COMPONENT_STACK_SIZE];
    int stack_ptr = 0;

    component_stack[stack_ptr++] = root;

    while (stack_ptr > 0) {

        if (stack_ptr > MAX_COMPONENT_STACK_SIZE) {
            fprintf(stderr, "ERROR! UI is too powerful for the stack! \n");
        }

        UIComponent *comp = component_stack[--stack_ptr];
        
        if (!comp->visible) {
            continue; // skip the children too
        }

        bool contains_mouse = is_point_in_rect(mouse_pos, UI_get_global_pos(comp), comp->size);
        
        if (comp->contains_mouse != contains_mouse) {
            comp->contains_mouse = contains_mouse;
            if (comp->update != NULL) {
                comp->update(comp);
            }
        }

        children: for (int i = 0; i < array_length(comp->children); i++) {
            component_stack[stack_ptr++] = comp->children[i];
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

    default_font = TTF_OpenFont("FFFFORWA.TTF", DEFAULT_FONT_SIZE);

    root = UI_alloc(UIComponent);
    root->is_root = true;
}   

UIStyle UIStyle_new() {
    UIStyle style;
    style.bg_color = (SDL_Color){30, 30, 30, 255};
    style.fg_color = (SDL_Color){210, 210, 210, 255};


    return style;
}

UIComponent UIComponent_new() {
    UIComponent component;
    component.is_root = false;
    component.children = array(UIComponent *, 10);
    component.parent = NULL;
    component.size = to_vec(100);
    component.pos = V2_ZERO;
    component.update = UIComponent_update;
    component.render = UIComponent_render;
    component.z_index = 0;
    component.visible = true;
    component.texture = NULL;
    component.isbutton = false;
    component.default_style = UIStyle_new();
    component.current_style_type = STYLE_DEFAULT;

    component.contains_mouse = false;

    return component;
}

bool UIComponent_is_visible(UIComponent *component) {
    
    if (!_ui_initialized) {
        printf("Error! UI not initialized! \n");
        exit(1);
        return false;
    }

    if (component->parent == NULL)
        return component->visible;

    return component->visible && UIComponent_is_visible(component->parent);

}

void UILabel_update(UIComponent *component) {
    
    printf("Updated! \n");

    UILabel *label = component;
    
    UIStyle current_style = UIComponent_get_current_style(label);

    TTF_SetFontSize(label->font, label->font_size);

    SDL_Surface *text_surface = TTF_RenderText_Blended(label->font, label->text.data, current_style.fg_color);

    // TTF_SetFontSize(label->font, DEFAULT_FONT_SIZE); // other stuff might use this font so ill be a good label and reset it
    // nvm.

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

    SDL_Color col = current_style.bg_color;

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

}

void UI_render(GPU_Target *target, UIComponent *component) {
    if (!component->is_root) {
        component->render(target, component);
    }

    for (int i = 0; i < array_length(component->children); i++) {
        UI_render(target, component->children[i]);
    }
}

void UIComponent_render(GPU_Target *target, UIComponent *component) {
    if (!UIComponent_is_visible(component)) return;

    v2 global_pos = UI_get_global_pos(component);

    GPU_Rect rect = {
        global_pos.x,
        global_pos.y,
        component->size.x,
        component->size.y
    };

    GPU_BlitRect(component->texture, NULL, target, &rect);
}

UILabel UILabel_new() {
    UILabel label;
    label.component = UIComponent_new();
    label.component.default_style.bg_color = (SDL_Color){0, 0, 0, 0};
    label.component.default_style.fg_color = (SDL_Color){255, 255, 255, 255};
    label.component.update = UILabel_update;
    label.text = String("Text here");
    label.font = default_font;
    label.font_size = DEFAULT_FONT_SIZE;
    label.alignment_x = ALIGNMENT_LEFT;
    label.alignment_y = ALIGNMENT_TOP;

    return label;
}

UIComponent UI_add_child(UIComponent *parent, UIComponent *child) {
    array_append(parent->children, child);
    child->parent = parent;

    if (parent == root) { // QoL
        UI_update(child);
    }
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


void _UI_mouse_click(SDL_MouseButtonEvent event, bool pressed) {

    _is_mouse_down = pressed;

    v2 mouse_pos = (v2){event.x, event.y};

    UIComponent *component_stack[MAX_COMPONENT_STACK_SIZE];
    int stack_ptr = 0;

    component_stack[stack_ptr++] = root;

    while (stack_ptr > 0) {

        if (stack_ptr > MAX_COMPONENT_STACK_SIZE) {
            fprintf(stderr, "ERROR! UI is too powerful for the stack! \n");
        }


        UIComponent *component = component_stack[--stack_ptr];

        if (!component->visible) continue; //skip the children too, dont need is_visible() bc it starts from the root

        if (!component->isbutton) goto children;
        UIButton *button = component;
        bool should_activate = button->activate_on_release == !pressed;
        if (is_point_in_rect(mouse_pos, UI_get_global_pos(button), UI_get_size(button))) {
            
            if (button->on_click != NULL && should_activate) {
                button->on_click(button, pressed);
            }
            
            // button might move inside on_click, but im not dealing with that right now bc its unlikely
            button->label.component.contains_mouse = is_point_in_rect(mouse_pos, UI_get_global_pos(button), UI_get_size(button));
            button->label.component.update(button);
        }
        
        

        children: for (int i = 0; i < array_length(component->children); i++) {
            component_stack[stack_ptr++] = component->children[i];
        }
    };
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

void UILabel_set_text(UILabel *label, String text) {
    if (label->text.data != NULL) String_delete(&label->text);

    label->text = text;
}

UIComponent *UI_get_root() {
    return root;
}

void UI_update(UIComponent *comp) {
    comp->update(comp);

    for (int i = 0; i < array_length(comp->children); i++) {
        UI_update(comp->children[i]);
    }
}

void UIComponent_update(UIComponent *component) {

    UIStyle current_style = UIComponent_get_current_style(component);

    SDL_Surface *surface = _UI_create_color_surface(component->size.x, component->size.y, current_style.bg_color);

    GPU_Image *texture = GPU_CopyImageFromSurface(surface);

    SDL_FreeSurface(surface);

    component->texture = texture;

    if (component->texture == NULL) {
        printf("Error! texture is null! \n");
    }
}

SDL_Surface *_UI_create_color_surface(int width, int height, SDL_Color color) {
    SDL_Surface *surf = SDL_CreateRGBSurface(
        0, 
        width,
        height,
        32, 
        0xFF000000, 
        0xFF0000, 
        0xFF00, 
        0xFF
    );

    SDL_FillRect(surf, NULL, SDL_MapRGBA(surf->format, color.r, color.g, color.b, color.a)); 

    return surf;
}

v2 UI_get_global_pos(UIComponent *comp) {
    v2 sum = V2_ZERO;
    UIComponent *current = comp;
    while (current != NULL) {
        sum = v2_add(sum, current->pos);
        current = current->parent;
    }

    return sum;
}

void UI_set_pos(UIComponent *comp, v2 pos) {
    comp->pos = pos;
}


void UI_set_size(UIComponent *comp, v2 size) {
    comp->size = size; 
}


v2 UI_get_size(UIComponent *comp) {
    return comp->size;
}


v2 UI_get_pos(UIComponent *comp) {
    return comp->size;
}


void UI_set_visible(UIComponent *comp, bool visibility) {
    comp->visible = visibility;
    if (visibility) {
        UI_update(comp);
    }
}


void UILabel_set_alignment(UILabel *label, TextAlignment x, TextAlignment y) {
    label->alignment_x = x;
    label->alignment_y = y;
}

void UI_center_around_pos(UIComponent *comp, v2 pos) {
    UI_set_global_pos(comp, v2_sub(pos, v2_div(comp->size, to_vec(2))));
}


void UI_set_global_pos(UIComponent *comp, v2 pos) {

    if (comp->parent == NULL) {
        comp->pos = pos;
        return;
    }

    v2 parent_pos = UI_get_global_pos(comp->parent);

    UI_set_pos(comp, v2_sub(pos, parent_pos));
}


// #END

#pragma GCC diagnostic pop