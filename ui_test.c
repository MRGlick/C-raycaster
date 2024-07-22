
#define SDL_MAIN_HANDLED

#include "ui.c"

void render();
void tick();

GPU_Target *screen;
TTF_Font *font;

void test_click_event(UIComponent *component, bool pressed) {
    component->pos = to_vec((double)rand() / RAND_MAX * 400);
    UILabel *label = component;
    int num = rand() % 5;
    if (num == 0) {
        label->text = "1";
    } else if (num == 1) {
        label->text = "2";
    }
    else if (num == 2) {
        label->text = "3";
    }
    else if (num == 3) {
        label->text = "4";
    }
    else if (num == 4) {
        label->text = "5";
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
    button->label.component.update(button);
    UI_add_component(button);

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

    GPU_RectangleFilled2(screen, GPU_MakeRect(0, 0, 100, 100), (SDL_Color){60, 120, 255, 255});

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