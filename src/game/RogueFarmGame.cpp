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

namespace game {

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

    SDL_Surface* surface = SDL_CreateSurface(24, 24, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        return;
    }

    SDL_Color color = {255, 0, 0, 255};
    Uint32 pixel = SDL_MapSurfaceRGBA(surface, color.r, color.g, color.b, color.a);
    if (!SDL_FillSurfaceRect(surface, nullptr, pixel)) {
        SDL_Log("Failed to fill surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return;
    }

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
