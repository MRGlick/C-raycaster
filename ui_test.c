
#define SDL_MAIN_HANDLED

#include "ui.c"

void render();
void tick();

GPU_Target *screen;
TTF_Font *font;
int points = 0;
UILabel *score_label;

void update_scoreboard() {
    String a = String("Score: ");
    String b = String_from_int(points);
    String final = String_concat(a, b);

    printf("final: %s \n", final.data);

    UILabel_set_text(score_label, final);
    score_label->component.update(score_label);

    String_delete(&a);
    String_delete(&b);
}

void test_click_event(UIComponent *component, bool pressed) {
    component->pos = to_vec((double)rand() / RAND_MAX * 400);
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
    component->update(component);
}



int main(int argc, char* argv[])
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create an SDL_gpu window
    screen = GPU_Init(800, 600, GPU_DEFAULT_INIT_FLAGS);
    if (screen == NULL) {
        printf("GPU_Init Error: %s\n", GPU_PopErrorCode().details);
        SDL_Quit();
        return 1;
    }

    UI_init();
    
    font = TTF_OpenFont("FFFFORWA.TTF", 30);

    UIButton *button = UI_alloc(UIButton);
    button->label.component.size = to_vec(100);
    button->on_click = test_click_event;
    button->label.alignment_x = ALIGNMENT_CENTER;
    button->label.alignment_y = ALIGNMENT_CENTER;
    UILabel_set_text(button, String("10000"));
    button->label.component.update(button);
    UI_add_component(button);


    score_label = UI_alloc(UILabel);
    score_label->component.pos = (v2){200, 0};
    score_label->component.size = (v2){400, 100};
    score_label->alignment_x = ALIGNMENT_CENTER;
    score_label->alignment_y = ALIGNMENT_CENTER;
    score_label->component.default_style.bg_color = (SDL_Color){0, 0, 0, 0};
    UILabel_set_text(score_label, String("Score: "));
    score_label->component.update(score_label);
    UI_add_component(score_label);

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

    arraylist *ui_comps = UI_get_components();

    for (int i = 0; i < ui_comps->length; i++) {
        UIComponent *component = arraylist_get_val(ui_comps, i);
        component->render(screen, component);
    }

    // Update the screen
    GPU_Flip(screen);
}

void tick() {

}