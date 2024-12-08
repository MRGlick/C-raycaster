
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#define SDL_MAIN_HANDLED

#include "ui.c"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

void render();
void tick();

GPU_Target *screen;
TTF_Font *font;
int points = 0;
UILabel *score_label;
UIComponent *menu;

#ifndef GET_NUM_DIGITS
#define GET_NUM_DIGITS
int get_num_digits(int num) {
    int res = 0;
    while (num > 0) {
        num /= 10;
        res++;
    }
    return res;
}
#endif

void update_scoreboard() {
    String a = String("Score: ");
    String b = String_from_int(points);
    String final = String_concat(a, b);

    UILabel_set_text(score_label, final);
    UI_update(score_label);

    String_delete(&a);
    String_delete(&b);
}

void test_click_event(UIComponent *component, bool pressed) {
    component->pos = to_vec((double)rand() / RAND_MAX * (WINDOW_WIDTH / 2));
    UILabel *label = component;
    int num = rand() % 5;

    points += atoi(label->text.data);
    update_scoreboard();

    if (num == 0) {
        label->text = String("1");
    } else if (num == 1) {
        label->text = String("2");
    }
    else if (num == 2) {
        label->text = String("3");
    }
    else if (num == 3) {
        label->text = String("4");
    }
    else if (num == 4) {
        label->text = String("5");
    }
    UI_update(label);
}

void paused_click_event(UIComponent *comp, bool pressed) {
    UI_set_visible(comp->parent, false);
}

void make_menu() {
    menu = UI_alloc(UIComponent);
    menu->size = (v2){400, 300};
    menu->pos = v2_sub((v2){WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2}, v2_div(menu->size, to_vec(2)));

    menu->default_style.bg_color = (SDL_Color){0, 0, 0, 125};

    UILabel *paused_label = UI_alloc(UILabel);
    UILabel_set_text(paused_label, String("Paused!"));
    paused_label->font_size = 30;
    paused_label->component.size = (v2){400, 100};
    paused_label->component.pos.y = 40;
    paused_label->alignment_x = ALIGNMENT_CENTER;
    paused_label->component.default_style = (UIStyle){.bg_color = (SDL_Color){0, 0, 0, 0}, .fg_color = (SDL_Color){255, 255, 255, 255}};

    UI_add_child(menu, paused_label);

    UIButton *button = UI_alloc(UIButton);
    UILabel_set_text(button, String("Continue"));
    UILabel_set_alignment(button, ALIGNMENT_CENTER, ALIGNMENT_CENTER);

    UI_set(UILabel, button, font_size, 24);

    UI_set_size(button, (v2){200, 100});
    UI_set_pos(button, (v2){100, 160});

    button->custom_on_click = paused_click_event;

    UI_add_child(menu, button);


    UITextLine *text_line = UI_alloc(UITextLine);
    UI_set_size(text_line, V2(500, 50));
    UI_center_around_pos(text_line, V2(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2));
    UI_add_child(root, text_line);


    UI_add_child(root, menu);
    UI_update(menu);

}

int main(int argc, char* argv[])
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create an SDL_gpu window
    screen = GPU_Init(WINDOW_WIDTH, WINDOW_HEIGHT, GPU_DEFAULT_INIT_FLAGS);
    if (screen == NULL) {
        printf("GPU_Init Error: %s\n", GPU_PopErrorCode().details);
        SDL_Quit();
        return 1;
    }

    UI_init(SDL_GetWindowFromID(screen->context->windowID), (v2){WINDOW_WIDTH, WINDOW_HEIGHT});
    
    font = TTF_OpenFont("FFFFORWA.TTF", 30);

    UIButton *button = UI_alloc(UIButton);
    button->label.component.size = to_vec(100);
    button->custom_on_click = test_click_event;
    button->label.alignment_x = ALIGNMENT_CENTER;
    button->label.alignment_y = ALIGNMENT_CENTER;
    UILabel_set_text(button, String("10000"));
    UI_update(button);
    UI_add_child(UI_get_root(), button);


    score_label = UI_alloc(UILabel);
    score_label->component.pos = (v2){200, 0};
    score_label->component.size = (v2){400, 100};
    score_label->alignment_x = ALIGNMENT_CENTER;
    score_label->alignment_y = ALIGNMENT_CENTER;
    score_label->component.default_style.bg_color = (SDL_Color){0, 0, 0, 0};
    UILabel_set_text(score_label, String("Score: "));
    UI_update(score_label);
    UI_add_child(UI_get_root(), score_label);

    make_menu();

    // Main loop flag
    bool quit = false;

    // Event handler
    SDL_Event e;

    // Main loop
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            UI_handle_event(e);


            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                UI_set_visible(menu, true);
            } 
        }

        tick();
        render();

        
    }

    // Clean up
    GPU_Quit();
    SDL_Quit();

    return 0;
}

void render() {
    // Clear screen
    GPU_Clear(screen);

    GPU_RectangleFilled2(screen, GPU_MakeRect(0, 0, 1000, 1000), (SDL_Color){60, 120, 255, 255});

    UI_render(screen, UI_get_root());

    // Update the screen
    GPU_Flip(screen);
}

void tick() {

}




#pragma GCC diagnostic pop