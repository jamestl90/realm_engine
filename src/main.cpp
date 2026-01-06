#include <iostream>

int main(int argv, char** argc)
{
    std::cout << "rogue farm game" << std::endl;
    return 0;
}#include "core/Engine.hpp"
#include <SDL3/SDL.h>

int main(int argc, char* argv[]) {
    core::Engine engine;
    
    if (!engine.initialize("SDL3 Game Engine", 1280, 720)) {
        SDL_Log("Failed to initialize engine");
        return 1;
    }
    
    SDL_Log("Engine initialized successfully");
    SDL_Log("Press ESC to quit");
    
    engine.run();
    
    return 0;
}
