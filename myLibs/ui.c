
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
#include <windows.h>
#include <stdarg.h>

#define MAX_COMPONENT_STACK_SIZE 1024
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
    void (*handle_input)(struct UIComponent *, SDL_Event);
    void (*on_click)(struct UIComponent *, bool);
    void (*on_gain_focus)(struct UIComponent *);
    void (*on_lose_focus)(struct UIComponent *);

    bool isbutton;

    bool contains_mouse;

    bool move_on_new_parent;

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

typedef struct UIRect {
    UIComponent comp;
    SDL_Color color;
} UIRect;

typedef struct UIButton {
    UILabel label;
    void (*custom_on_click)(struct UIComponent *, bool);
    bool activate_on_release;

    UIStyle hover_style, pressed_style;
} UIButton;

typedef struct UITextLine {
    UILabel label;
    char *text;
    int cursor_pos;
    int char_limit;
    bool numbers_only;
} UITextLine;


#define first_arg(arg1, ...) arg1

#define rest_args(arg1, ...) __VA_ARGS__

// Assumes all args are of the same type
#define count_args(type, ...) (sizeof((type[]){__VA_ARGS__}) / sizeof(type))


UIComponent UI_add_child(UIComponent *parent, UIComponent *child);

// Only used for the macro. DO NOT USE. Just do a for loop or something.
void _UI_add_children(UIComponent *comp, int num_children, ...) {
    va_list args;

    va_start(args, num_children);

    for (int i = 0; i < num_children; i++) {

        UIComponent *current = va_arg(args, UIComponent *);

        if (current == NULL) continue;

        UI_add_child(comp, current);
    }
}



#define UI_alloc(type, name, ...) ({ \
    type *name; \
    name = malloc(sizeof(type));  \
    (*name) = type##_new(); \
    first_arg(__VA_ARGS__);  \
    int args_count__ = count_args(UIComponent *, rest_args(__VA_ARGS__)); \
    _UI_add_children(name, args_count__, rest_args(__VA_ARGS__) + 0); \
    name; \
})

//                                           its big brain time ^

#define UI_get_comp(comp) ((UIComponent *)comp)

#define UI_set(type, comp, property, value) ((type *)comp)->property = value 




TTF_Font *default_font;
bool _ui_initialized = false;
bool _is_mouse_down = false;
bool _ctrl_down = false;
bool _shift_down = false;
bool UI_debug_show_focused = false;
UIComponent *root;
bool _fullscreen = false; // for functionality rather than visibility
SDL_Window *_window;
v2 _window_size;

UIComponent *_current_focused_comp = NULL;

// #FUNC

void UI_render_text(GPU_Target *target, String text, v2 pos, v2 size, TTF_Font *f);

void UITextLine_remove_char(UITextLine *text_line);

void UIRect_update(UIComponent *comp);

UIRect UIRect_new();

void UITextLine_type_char(UIComponent *comp, char c);

char *_UI_get_clipboard();

void UITextLine_update(UIComponent *comp);

void UITextLine_gain_focus(UIComponent *comp);

void UITextLine_lose_focus(UIComponent *comp);

void reset_focus();

void UIComponent_gain_focus(UIComponent *comp);

void UIButton_on_click(UIButton *button, bool pressed);

void UITextLine_handle_input(UIComponent *comp, SDL_Event input);

void UIComponent_handle_input(UIComponent *comp, SDL_Event input);

UITextLine UITextLine_new();

void UI_set_fullscreen(bool f);

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


    SDL_DisplayMode mode;
    int gw, gh;
    SDL_GetDesktopDisplayMode(0, &mode);
    gw = mode.w;
    gh = mode.h;


    if (GPU_GetFullscreen()) {
        int x, y;
    
        SDL_GetGlobalMouseState(&x, &y);

        return v2_mul(v2_div((v2){x, y}, (v2){gw, gh}), _window_size);

    } else {
        int x, y;
    
        SDL_GetMouseState(&x, &y);

        return (v2){x, y};
    }
    
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

void UI_init(SDL_Window *w, v2 window_size, String font) {
    if (_ui_initialized) {
        printf("UI already initialized! \n");
        return;
    }
    _ui_initialized = true;
    TTF_Init();

    if (String_isnull(font)) {
        default_font = TTF_OpenFont("FFFFORWA.TTF", DEFAULT_FONT_SIZE);
    } else {
        default_font = TTF_OpenFont(font.data, DEFAULT_FONT_SIZE);
    }


    root = UI_alloc(UIComponent, r);
    root->is_root = true;

    _window = w;
    _window_size = window_size;

    

}

UIStyle UIStyle_new() {
    UIStyle style;
    style.bg_color = (SDL_Color){30, 30, 30, 255};
    style.fg_color = (SDL_Color){210, 210, 210, 255};


    return style;
}

UIComponent UIComponent_new() {
    UIComponent component = {0};
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
    component.move_on_new_parent = true;
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

SDL_Surface *create_empty_surface(int w, int h) {

    SDL_Surface *surf = SDL_CreateRGBSurface(
            0, 
            w,
            h,
            32, 
            0xFF000000, 
            0xFF0000, 
            0xFF00, 
            0xFF
    );

    return surf;
}

void UILabel_update(UIComponent *component) {

    UILabel *label = component;
    
    UIStyle current_style = UIComponent_get_current_style(label);

    TTF_SetFontSize(label->font, label->font_size);

    SDL_Surface *text_surface = NULL;

    if (label->text.len == 0) {
        text_surface = create_empty_surface(1, 1);
    } else {
        text_surface = TTF_RenderText_Blended(label->font, label->text.data, current_style.fg_color);
    }

    // TTF_SetFontSize(label->font, DEFAULT_FONT_SIZE); // other stuff might use this font so ill be a good label and reset it
    // nvm.

    SDL_Surface *main_surface = create_empty_surface(max(component->size.x, text_surface->w), max(component->size.y, text_surface->h));

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

}

void UI_render(GPU_Target *target, UIComponent *component) {
    if (!component->is_root) {

        bool is_visible = UIComponent_is_visible(component);

        if (is_visible) {
            component->render(target, component);
        }
        if (component == _current_focused_comp && is_visible && UI_debug_show_focused) {

            int padding = 10;

            v2 pos = v2_sub(UI_get_global_pos(component), to_vec(padding));
            v2 size = v2_add(component->size, to_vec(padding * 2));

            GPU_RectangleFilled2(target, GPU_MakeRect(pos.x, pos.y, size.x, size.y), GPU_MakeColor(255, 50, 50, 30));
        }
    }

    for (int i = 0; i < array_length(component->children); i++) {
        UI_render(target, component->children[i]);
    }
}

void UIComponent_render(GPU_Target *target, UIComponent *component) {
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

    if (parent == NULL) {
        commit_sudoku();
    } else if (child == NULL) {
        commit_sudoku();
    }


    array_append(parent->children, child);
    child->parent = parent;

    if (!child->move_on_new_parent) {
        UI_set_global_pos(child, child->pos);
    }
    

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
    button.label.alignment_x = ALIGNMENT_CENTER;
    button.label.alignment_y = ALIGNMENT_CENTER;
    UI_get_comp(&button)->on_click = UIButton_on_click;
    button.custom_on_click = default_on_click;
    button.activate_on_release = true;

    UI_get_comp(&button)->default_style = (UIStyle){.bg_color = (SDL_Color){0, 0, 0, 50}, .fg_color = (SDL_Color){255, 255, 255, 255}};

    button.hover_style = UIStyle_new();

    button.hover_style.bg_color = (SDL_Color){0, 0, 0, 100};
    button.hover_style.fg_color = (SDL_Color){255, 255, 255, 255};

    button.pressed_style = UIStyle_new();

    button.pressed_style.bg_color = (SDL_Color){0, 0, 0, 150};
    button.pressed_style.fg_color = (SDL_Color){255, 255, 255, 255};

    return button;
}


void _UI_mouse_click(SDL_MouseButtonEvent event, bool pressed) {

    _is_mouse_down = pressed;

    v2 mouse_pos = _get_mouse_pos();

    bool clicked_something = false;

    UIComponent *component_stack[MAX_COMPONENT_STACK_SIZE];
    int stack_ptr = 0;

    component_stack[stack_ptr++] = root;

    while (stack_ptr > 0) {

        if (stack_ptr > MAX_COMPONENT_STACK_SIZE) {
            fprintf(stderr, "ERROR! UI is too powerful for the stack! \n");
        }


        UIComponent *component = component_stack[--stack_ptr];

        if (!component->visible) continue; //skip the children too, dont need is_visible() bc it starts from the root

        if (is_point_in_rect(mouse_pos, UI_get_global_pos(component), UI_get_size(component))) {
            if (component->on_click != NULL) component->on_click(component, pressed);

            UIComponent_gain_focus(component);
            
            clicked_something = true;
            // remember to handle contains_mouse
        }

        // if (!component->isbutton) goto children;
        // UIButton *button = component;
        // bool should_activate = button->activate_on_release == !pressed;
        // if (is_point_in_rect(mouse_pos, UI_get_global_pos(button), UI_get_size(button))) {
            
        //     if (button->on_click != NULL && should_activate) {
        //         button->on_click(button, pressed);
        //     }
            
        //     // button might move inside on_click, but im not dealing with that right now bc its unlikely
        //     button->label.component.contains_mouse = is_point_in_rect(mouse_pos, UI_get_global_pos(button), UI_get_size(button));
        //     button->label.component.update(button);
        // }
        
        if (!clicked_something) {
            reset_focus();
        }
        

        for (int i = array_length(component->children) - 1; i >= 0; i--) { // what's after is on top
            component_stack[stack_ptr++] = component->children[i];
        }
    };
}

void UI_handle_event(SDL_Event event) {

    UIComponent_handle_input(root, event);

    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            _UI_mouse_click(event.button, true);
            break;
        case SDL_MOUSEBUTTONUP:
            _UI_mouse_click(event.button, false);
            break;
        case SDL_MOUSEMOTION:
            _UI_mouse_motion(event.motion);
            break;
        case SDL_QUIT:
            // exit(0);
            break;
    }
}

void UILabel_set_text(UILabel *label, String text) {
    if (label->text.data != NULL && !label->text.ref) String_delete(&label->text);
    
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

    comp->move_on_new_parent = false;

    if (comp->parent == NULL) {
        comp->pos = pos;
        return;
    }

    v2 parent_pos = UI_get_global_pos(comp->parent);

    UI_set_pos(comp, v2_sub(pos, parent_pos));
}

void UI_set_fullscreen(bool f) {
    _fullscreen = f;
}

UITextLine UITextLine_new() {
    UITextLine text_line = {0};

    text_line.char_limit = 20;

    text_line.label = UILabel_new();

    UI_get_comp(&text_line)->default_style.bg_color = GPU_MakeColor(0, 0, 0, 70);
    UI_get_comp(&text_line)->default_style.fg_color = GPU_MakeColor(255, 255, 255, 255);

    text_line.cursor_pos = 0;
    text_line.text = array(char, 20);
    UI_get_comp(&text_line)->handle_input = UITextLine_handle_input;

    UI_get_comp(&text_line)->update = UITextLine_update;

    UI_get_comp(&text_line)->on_gain_focus = UITextLine_gain_focus;
    UI_get_comp(&text_line)->on_lose_focus = UITextLine_lose_focus;

    return text_line;
}


void UIComponent_handle_input(UIComponent *comp, SDL_Event input) {
    if (comp == NULL) return;
    if (comp->handle_input != NULL) comp->handle_input(comp, input);

    for (int i = 0; i < array_length(comp->children); i++) {
        UIComponent_handle_input(comp->children[i], input);
    }
}


void UITextLine_handle_input(UIComponent *comp, SDL_Event input) {
    if (_current_focused_comp != comp) return;

    UITextLine *text_line = comp;

    if (input.type == SDL_TEXTINPUT) {
        if (input.text.text[0] != '\0') {
            UITextLine_type_char(text_line, input.text.text[0]);
        }
    }
    if (input.type == SDL_KEYDOWN) {
        if (input.key.keysym.sym == SDLK_RIGHT) {
            text_line->cursor_pos++;
        }
        if (input.key.keysym.sym == SDLK_LEFT) {
            text_line->cursor_pos--;
        }
        if (input.key.keysym.sym == SDLK_BACKSPACE && text_line->cursor_pos != 0) {
            UITextLine_remove_char(text_line);
        }
        if (input.key.keysym.sym == SDLK_v && (SDL_GetModState() & KMOD_CTRL)) {
            char *clipboard_data = _UI_get_clipboard();
            
            for (int i = 0; clipboard_data[i] != '\0'; i++) {
                printf("Should add: %c \n", clipboard_data[i]);
                UITextLine_type_char(text_line, clipboard_data[i]);
            }
            
            free(clipboard_data);
        }
    }
    text_line->cursor_pos = clamp(text_line->cursor_pos, 0, array_length(text_line->text));
    
    if (array_length(text_line->text) == 0) {
        UILabel_set_text(text_line, StringRef(""));
    } else {
        String str = String_ncopy_from_literal(text_line->text, array_length(text_line->text));

        UILabel_set_text(text_line, str);
    }

    

    UI_update(text_line);
}

void UIButton_on_click(UIButton *button, bool pressed) {

    UIButton_update(button);

    if (button->activate_on_release == pressed) return;

    if (button->custom_on_click != NULL) {
        button->custom_on_click(button, pressed);
    }
}

void UIComponent_gain_focus(UIComponent *comp) {
    
    if (_current_focused_comp == comp) return;


    if (_current_focused_comp != NULL && _current_focused_comp->on_lose_focus != NULL) {
        _current_focused_comp->on_lose_focus(_current_focused_comp);
    }
    
    _current_focused_comp = comp;


    if (comp->on_gain_focus != NULL) {
        comp->on_gain_focus(comp);
    }
}


void reset_focus() {
    if (_current_focused_comp != NULL && _current_focused_comp->on_lose_focus != NULL) {
        _current_focused_comp->on_lose_focus(_current_focused_comp);
    }

    _current_focused_comp = NULL;
}


void UITextLine_gain_focus(UIComponent *comp) {
    
    UITextLine *text_line = comp;

    int count = array_length(text_line->text);

    for (int i = 0; i < count; i++) {
        UITextLine_remove_char(text_line);
    }
    
    SDL_StartTextInput();
}

void UITextLine_lose_focus(UIComponent *comp) {
    SDL_StopTextInput();
    UI_update(comp);
}


void UITextLine_update(UIComponent *comp) {
    UILabel_update(comp);

    if (comp != _current_focused_comp) return;

    UILabel *label = comp;
    UITextLine *text_line = comp;

    v2 offset = V2(0, 0);
    double text_width = 0;

    for (int i = 0; i < array_length(text_line->text); i++) {
        int adv;
        TTF_GlyphMetrics32(label->font, text_line->text[i], NULL, NULL, NULL, NULL, &adv);
        text_width += adv;
        if (i < text_line->cursor_pos) offset.x += adv;
    }

    SDL_Surface *surface = GPU_CopySurfaceFromImage(comp->texture);

    switch (label->alignment_x) {
        case ALIGNMENT_CENTER:
            offset.x += comp->size.x / 2 - text_width / 2;
            break;
        case ALIGNMENT_RIGHT:
            offset.x += comp->size.x - text_width;
            break;
    }

    SDL_Rect rect = {
        offset.x,
        offset.y,
        5,
        comp->size.y
    };

    SDL_FillRect(surface, &rect, 0xFFFFFFFF);
    
    GPU_UpdateImage(comp->texture, NULL, surface, NULL);

    SDL_FreeSurface(surface);

}


// Thanks GPT

// Remember to free this! 
char *_UI_get_clipboard() {
    if (!OpenClipboard(NULL)) {
        return NULL;
    }

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == NULL) {
        CloseClipboard();
        return NULL;
    }


    char* clipboardText = (char*)GlobalLock(hData);
    if (clipboardText == NULL) {
        CloseClipboard();
        return NULL;
    }

    char* result = _strdup(clipboardText);

    GlobalUnlock(hData);
    CloseClipboard();

    return result;
}

// Thanks GPT
int _UI_set_clipboard(const char* text) {
    if (!OpenClipboard(NULL)) {
        return 0;
    }

    if (!EmptyClipboard()) {
        CloseClipboard();
        return 0;
    }

    size_t length = strlen(text) + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, length);
    if (hMem == NULL) {
        CloseClipboard();
        return 0;
    }

    void* memPtr = GlobalLock(hMem);
    if (memPtr == NULL) {
        GlobalFree(hMem);
        CloseClipboard();
        return 0;
    }
    memcpy(memPtr, text, length);
    GlobalUnlock(hMem);

    if (SetClipboardData(CF_TEXT, hMem) == NULL) {
        GlobalFree(hMem);
        CloseClipboard();
        return 0;
    }

    CloseClipboard();

    return 1;
}

void UITextLine_type_char(UIComponent *comp, char c) {

    UITextLine *text_line = comp;

    if (array_length(text_line->text) + 1 > text_line->char_limit) return;

    if (text_line->numbers_only && (c < '0' || c > '9')) return;

    array_insert(text_line->text, c, text_line->cursor_pos);
    text_line->cursor_pos++;
}


UIRect UIRect_new() {
    UIRect rect = {0};
    rect.comp = UIComponent_new();
    rect.color = GPU_MakeColor(255, 255, 255, 255);
    rect.comp.update = UIRect_update;

    return rect;
}

void UIRect_update(UIComponent *comp) {
    UIRect *rect = comp;
    void *thing = comp;

    SDL_Surface *surf = create_empty_surface(comp->size.x, comp->size.y);

    SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, rect->color.r, rect->color.g, rect->color.b));

    GPU_Image *texture = GPU_CopyImageFromSurface(surf);

    SDL_FreeSurface(surf);

    comp->texture = texture;

}

void UITextLine_remove_char(UITextLine *text_line) {
    array_remove(text_line->text, text_line->cursor_pos - 1);
    text_line->cursor_pos--;
}

// Really slow, obviously
void UI_render_text(GPU_Target *target, String text, v2 pos, v2 size, TTF_Font *f) {

    TTF_Font *font = f == NULL ? default_font : f;

    SDL_Surface *surface = TTF_RenderText_Blended(font, text.data, GPU_MakeColor(255, 255, 255, 255));

    if (surface == NULL) {
        printf("%s \n", TTF_GetError());
        exit(-1);
    }

    GPU_Image *image = GPU_CopyImageFromSurface(surface);

    int middle_x = pos.x + size.x / 2;
    int middle_y = pos.y + size.y / 2;

    GPU_Rect rect = {
        .x = middle_x - surface->w / 2, 
        .y = middle_y - surface->h / 2, 
        .w = surface->w, 
        .h = surface->h
    };

    GPU_BlitRect(image, NULL, target, &rect);

    SDL_FreeSurface(surface);
    GPU_FreeImage(image);

}


// #END

#pragma GCC diagnostic pop