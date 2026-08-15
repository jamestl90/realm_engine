#define SDL_DISABLE_OLD_NAMES

#include "core/Engine.hpp"
#include "game/test_app.hpp"
#include <memory>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    core::Engine engine;

    if (!engine.initialize("test_app", 2460, 1440)) {
        return 1;
    }

    engine.set_game(std::make_unique<game::TestApp>());

    engine.run();

    return 0;
}
