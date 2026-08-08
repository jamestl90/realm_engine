#include "RogueFarmGame.hpp"
#include "../../include/core/Engine.hpp"
#include "../../include/core/Config.hpp"
#include "../../include/core/Utils.hpp"
#include "../../include/rendering/Sprite.hpp"
#include "../../include/rendering/UIRenderer.hpp"
#include "../../include/rendering/FontManager.hpp"
#include "../../include/ui/Layout.hpp"
#include "../../include/ui/ComboBox.hpp"
#include <SDL3/SDL.h>

#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
#include "../../include/procgen/GreaterRealm.hpp"
#endif

namespace game {

#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
namespace {

SDL_Color terrain_colour(const procgen::GreaterRealmCell& cell) noexcept {
    const float shade = 0.82f + cell.elevation * 0.18f;
    const auto scale = [shade](std::uint8_t value) -> std::uint8_t {
        return static_cast<std::uint8_t>(static_cast<float>(value) * shade);
    };

    switch (cell.terrain_form) {
        case procgen::TerrainForm::Ocean:
            return SDL_Color{24, 76, 132, 255};
        case procgen::TerrainForm::Coast:
            return SDL_Color{210, 190, 126, 255};
        case procgen::TerrainForm::Plains:
            return SDL_Color{scale(78), scale(150), scale(82), 255};
        case procgen::TerrainForm::Hills:
            return SDL_Color{scale(112), scale(136), scale(74), 255};
        case procgen::TerrainForm::Highlands:
            return SDL_Color{scale(126), scale(112), scale(94), 255};
        case procgen::TerrainForm::Mountains:
            return SDL_Color{scale(192), scale(194), scale(188), 255};
    }

    return SDL_Color{255, 0, 255, 255};
}

SDL_Surface* create_procgen_debug_surface() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 8675309;
    settings.width = 256;
    settings.height = 192;
    settings.sea_level = 0.5f;

    const auto map = procgen::generate_greater_realm(settings);
    SDL_Surface* surface = SDL_CreateSurface(
        static_cast<int>(map.width),
        static_cast<int>(map.height),
        SDL_PIXELFORMAT_RGBA32
    );

    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create procgen debug surface: %s", SDL_GetError());
        return nullptr;
    }

    if (SDL_MUSTLOCK(surface) && !SDL_LockSurface(surface)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to lock procgen debug surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return nullptr;
    }

    for (std::uint32_t y = 0; y < map.height; ++y) {
        auto* row = reinterpret_cast<std::uint32_t*>(
            static_cast<std::uint8_t*>(surface->pixels) + static_cast<std::size_t>(y) * surface->pitch
        );

        for (std::uint32_t x = 0; x < map.width; ++x) {
            const auto& cell = map.cells[map.index(x, y)];
            const SDL_Color colour = terrain_colour(cell);
            row[x] = SDL_MapSurfaceRGBA(surface, colour.r, colour.g, colour.b, colour.a);
        }
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    SDL_Log("Generated procgen debug map: seed=%llu size=%ux%u", settings.seed, map.width, map.height);
    return surface;
}

} // namespace
#endif

void RogueFarmGame::on_startup(core::Engine& engine) {
    SDL_Log("RogueFarmGame starting up...");

    // Load font
    auto* ui_renderer = engine.ui_renderer();
    if (ui_renderer) {
        const std::string fontPath = config::get_executable_dir().string() + "\\assets\\fonts\\Kenney High.ttf";
        rendering::FontID fontId = ui_renderer->loadFont(fontPath.c_str(), 16.0f);
        if (fontId != rendering::INVALID_FONT_ID) {
            ui_renderer->setDefaultFont(fontId);
            // Wire up font manager for text measurement in UI
            engine.ui_manager().setFontManager(engine.font_manager(), fontId);
            SDL_Log("Loaded font: %s", fontPath.c_str());
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load font: %s", fontPath.c_str());
        }
    }

    SDL_Surface* surface = nullptr;

#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
    surface = create_procgen_debug_surface();
#else
    surface = SDL_CreateSurface(24, 24, SDL_PIXELFORMAT_RGBA32);
#endif

    if (!surface) {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        return;
    }

#if !defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
    SDL_Color color = {255, 0, 0, 255};
    Uint32 pixel = SDL_MapSurfaceRGBA(surface, color.r, color.g, color.b, color.a);
    if (!SDL_FillSurfaceRect(surface, nullptr, pixel)) {
        SDL_Log("Failed to fill surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return;
    }
#endif

    auto* texture_manager = engine.texture_manager();
    if (!texture_manager) {
        SDL_Log("Texture manager is null!");
        SDL_DestroySurface(surface);
        return;
    }

    m_test_texture = texture_manager->create_from_surface(surface);
    if (m_test_texture == rendering::INVALID_TEXTURE_ID) {
        SDL_Log("Failed to create texture from surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return;
    }

    SDL_DestroySurface(surface);

    m_test_entity = engine.world().create_entity();

    // Use logical coordinates (fixed resolution, letterboxed)
    const int logical_w = core::Engine::LOGICAL_W;
    const int logical_h = core::Engine::LOGICAL_H;

    rendering::Transform transform;
    transform.x = static_cast<float>(logical_w) / 2.0f;
    transform.y = static_cast<float>(logical_h) / 2.0f;
    transform.z = 0.0f;
    engine.world().add_component(m_test_entity, transform);

    rendering::Sprite sprite;
    sprite.texture_id = m_test_texture;
    sprite.layer = 0;
    sprite.r = 255;
    sprite.g = 255;
    sprite.b = 255;
    sprite.a = 255;
#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
    sprite.scale_x = 4.0f;
    sprite.scale_y = 4.0f;
#endif
    engine.world().add_component(m_test_entity, sprite);

    // Create UI elements using UIManager
    auto& ui_mgr = engine.ui_manager();

    // Create root as a vertical StackPanel for proper layout
    auto root = std::make_unique<ui::StackPanel>(ui::Orientation::Vertical);
    root->setPadding(ui::Thickness(20.0f));
    root->setSpacing(10.0f);

    // Set size constraints to make it 50% of logical screen width
    ui::SizeConstraints constraints;
    constraints.preferred_width = static_cast<float>(logical_w) * 0.25f;
    constraints.min_width = static_cast<float>(logical_w) * 0.25f;
    root->setSizeConstraints(constraints);

    // Set white background
    root->setBackgroundColour(ui::Colour::white());

    // Create Button
    auto button = std::make_unique<ui::Button>("Click Me!");
    button->setBackgroundColour(ui::Colour{80, 120, 200, 255});
    button->setHoverColour(ui::Colour{100, 140, 220, 255});
    button->setPressedColour(ui::Colour{60, 100, 180, 255});
    button->setTextColour(ui::Colour{255, 255, 255, 255});
    button->setBorderColour(ui::Colour{40, 80, 160, 255});
    button->setBorderThickness(2.0f);
    button->setFontSize(16.0f);
    button->setOnClick([&engine]() {
        SDL_Log("Button clicked.");
    });

    // Create TextBox
    auto textBox = std::make_unique<ui::TextBox>();
    textBox->setPlaceholder("Enter text here...");
    textBox->setBackgroundColour(ui::Colour{40, 40, 50, 255});
    textBox->setTextColour(ui::Colour{255, 255, 255, 255});
    textBox->setBorderColour(ui::Colour{80, 80, 100, 255});
    textBox->setFocusBorderColour(ui::Colour{100, 150, 255, 255});
    textBox->setBorderThickness(2.0f);
    textBox->setFontSize(14.0f);
    textBox->setOnTextChanged([](const std::string& text) {
        //SDL_Log("Text changed: %s", text.c_str());
    });

    // Create ComboBox
    auto comboBox = std::make_unique<ui::ComboBox>();
    comboBox->setPlaceholder("Select an resolution");
    comboBox->addItem("2560 x 1440");
    comboBox->addItem("1920 x 1080");
    comboBox->addItem("1280 x 720");
    comboBox->setBackgroundColour(ui::Colour{240, 240, 240, 255});
    comboBox->setTextColour(ui::Colour{0, 0, 0, 255});
    comboBox->setBorderColour(ui::Colour{180, 180, 180, 255});
    comboBox->setHoverColour(ui::Colour{230, 230, 230, 255});
    comboBox->setDropdownBackgroundColour(ui::Colour{250, 250, 250, 255});
    comboBox->setItemHoverColour(ui::Colour{220, 0, 220, 255});
    comboBox->setBorderThickness(1.0f);
    comboBox->setFontSize(14.0f);
    comboBox->setOnSelectionChanged([&engine](const std::string& selectedItem) {
        auto res = core::parseResolutionString(selectedItem);
        if (res.has_value()) {
            engine.resize_window(res->first, res->second);
        }
    });

    // Add UI elements to root
    root->addChild(std::move(button));
    root->addChild(std::move(textBox));
    root->addChild(std::move(comboBox));

    // Set root on UIManager (it will handle layout)
    ui_mgr.setRoot(std::move(root));

    SDL_Log("RogueFarmGame startup complete");
}

void RogueFarmGame::on_update(core::Engine& engine, double dt) {
    (void)engine;
    (void)dt;
    // UIManager handles UI updates now
}

void RogueFarmGame::on_render(core::Engine& engine, double alpha) {
    (void)alpha;

    // UI rendering happens after sprites, before present
    auto* renderer = engine.renderer();
    auto* ui_renderer = engine.ui_renderer();
    auto* ui_root = engine.ui_manager().root();

    if (!renderer || !ui_renderer || !ui_root) {
        return;
    }

    // End sprite render pass so UI can do its copy pass
    renderer->end_render_pass();

    // Render UI using logical dimensions
    ui_renderer->render(
        renderer->command_buffer(),
        renderer->swapchain_texture(),
        ui_root,
        static_cast<float>(core::Engine::LOGICAL_W),
        static_cast<float>(core::Engine::LOGICAL_H)
    );
}

void RogueFarmGame::on_shutdown(core::Engine& engine) {
    // UIManager owns the UI tree now, no need to clean it up here

    if (m_test_texture != rendering::INVALID_TEXTURE_ID) {
        engine.texture_manager()->destroy(m_test_texture);
        m_test_texture = rendering::INVALID_TEXTURE_ID;
    }

    if (m_test_entity.is_valid()) {
        engine.world().destroy_entity(m_test_entity);
    }

    SDL_Log("RogueFarmGame shutting down...");
}

void RogueFarmGame::on_resized(core::Engine& engine, int width, int height)
{

}

} // namespace game
