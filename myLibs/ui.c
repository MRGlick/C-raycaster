#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_gpu.h>
#include "vec2.c"
#include <stdbool.h>
#include "array.c"
#include "arraylist.c"
#include "mystring.c"


#define MAX_COMPONENT_STACK_SIZE 1000

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
UIComponent *root;

// #FUNC

void UI_init();
UIStyle UIStyle_new();
UIComponent UIComponent_new();
UILabel UILabel_new();
UIButton UIButton_new();

void UIComponent_render(GPU_Target *target, UIComponent *component);
void UI_render(GPU_Target *target, UIComponent *component);

UIStyle UIComponent_get_current_style(UIComponent *component);
v2 _get_mouse_pos();
bool is_point_in_rect(v2 point, v2 rect_pos, v2 rect_size);

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

bool is_point_in_rect(v2 point, v2 rect_pos, v2 rect_size) {
    return point.x >= rect_pos.x && point.x <= rect_pos.x + rect_size.x && point.y >= rect_pos.y && point.y <= rect_pos.y + rect_size.y;
}

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

        bool contains_mouse = is_point_in_rect(mouse_pos, comp->pos, comp->size);
        
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

    default_font = TTF_OpenFont("FFFFORWA.TTF", default_font_ptsize);

    root = UI_alloc(UIComponent);
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

    if (label->text.data == NULL) {
        printf("data: NULL \n");
    } else {
        printf("data: %s \n", label->text.data);
    }

    SDL_Surface *text_surface = TTF_RenderText_Blended(label->font, label->text.data, current_style.fg_color);

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
    component->render(target, component);

    for (int i = 0; i < array_length(component->children); i++) {
        component->children[i]->render(target, component->children[i]);
    }
}

void UIComponent_render(GPU_Target *target, UIComponent *component) {
    if (!UIComponent_is_visible(component)) return;

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
    label.text = String("Text here");
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
        if (is_point_in_rect(mouse_pos, button->label.component.pos, button->label.component.size)) {
            
            if (button->on_click != NULL && should_activate) {
                button->on_click(button, pressed);
            }
            
            // button might move inside on_click, but im not dealing with that right now bc its unlikely
            button->label.component.contains_mouse = is_point_in_rect(mouse_pos, button->label.component.pos, button->label.component.size);
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
        comp->children[i]->update(comp->children[i]);
    }
}

// #END