#include "core/Engine.hpp"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    core::Engine engine;

    if (!engine.initialize("Rogue Farm Game", 1280, 720)) {
        return 1;
    }

    engine.run();

    return 0;
}
