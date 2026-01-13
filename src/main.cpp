#define SDL_DISABLE_OLD_NAMES

#include "core/Engine.hpp"
#include "game/RogueFarmGame.hpp"
#include <memory>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    core::Engine engine;

    if (!engine.initialize("Rogue Farm Game", 1920, 1080)) {
        return 1;
    }

    // Set up the game
    engine.set_game(std::make_unique<game::RogueFarmGame>());

    engine.run();

    return 0;
}
