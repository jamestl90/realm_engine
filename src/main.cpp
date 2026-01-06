#include <SDL3/SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    
    SDL_Log("SDL3 initialized successfully");
    SDL_Log("Rogue Farm Game - Engine Starting");
    
    SDL_Window* window = SDL_CreateWindow(
        "Rogue Farm Game",
        1280,
        720,
        SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Log("Window created successfully");
    SDL_Log("Press ESC or close window to quit");
    
    bool running = true;
    SDL_Event event;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                        running = false;
                    }
                    break;
            }
        }
        
        // Frame delay to avoid hammering CPU
        SDL_Delay(16);
    }
    
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    SDL_Log("Engine shutdown complete");
    return 0;
}
