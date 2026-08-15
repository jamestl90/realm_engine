#include "test_app.hpp"
#include "../../include/core/Engine.hpp"
#include "../../include/core/Config.hpp"
#include "../../include/core/Utils.hpp"
#include "../../include/rendering/FontManager.hpp"
#include "../../include/rendering/Sprite.hpp"
#include "../../include/rendering/TerrainMesh.hpp"
#include "../../include/rendering/TerrainRenderer.hpp"
#include "../../include/rendering/UIRenderer.hpp"
#include "../../include/ui/Button.hpp"
#include "../../include/ui/ComboBox.hpp"
#include "../../include/ui/Layout.hpp"
#include "../../include/ui/Primitives.hpp"
#include <SDL3/SDL.h>
#include <cmath>
#if defined(REALM_ENABLE_PROCGEN_PROFILING)
#include <chrono>
#endif
#include <vector>

#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
#include "../../include/procgen/GreaterRealmDebug.hpp"
#endif

namespace game {

#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
bool TestApp::regenerate_procgen_debug_map(core::Engine& engine) {
    m_procgen_paint_dirty = false;
#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    using ProfileClock = std::chrono::steady_clock;
    const auto started_at = ProfileClock::now();
#endif

    m_procgen_map = procgen::generate_greater_realm(m_procgen_settings, m_procgen_constraints);
#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const auto generated_at = ProfileClock::now();
#endif
    const auto image = procgen::build_greater_realm_debug_image(
        m_procgen_map,
        procgen::NORMALIZED_WATERLINE,
        m_procgen_debug_options
    );
#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const auto image_built_at = ProfileClock::now();
#endif
    if (!image.has_expected_byte_count()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to build procgen debug image");
        return false;
    }

    if (!replace_procgen_debug_texture(engine, image)) {
        return false;
    }
    if (!refresh_procgen_terrain_mesh(engine, image)) {
        return false;
    }
#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const auto texture_uploaded_at = ProfileClock::now();
#endif

    m_procgen_debug_panel.update(m_procgen_map);
#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const auto finished_at = ProfileClock::now();
    const auto elapsed_ms = [](auto start, auto finish) {
        return std::chrono::duration<double, std::milli>(finish - start).count();
    };
    SDL_Log(
        "Procgen end-to-end stages: generation=%.2fms image=%.2fms texture=%.2fms scene/UI=%.2fms total=%.2fms",
        elapsed_ms(started_at, generated_at),
        elapsed_ms(generated_at, image_built_at),
        elapsed_ms(image_built_at, texture_uploaded_at),
        elapsed_ms(texture_uploaded_at, finished_at),
        elapsed_ms(started_at, finished_at)
    );
#endif
    SDL_Log(
        "Generated procgen debug map: seed=%llu size=%ux%u waterline=%.2f island=%.2f coast=%.2f base=%.2f mountain=%.2f peaks=%zu ridge=%.2f valley=%.2f noise=%.2f ocean=%.2f channel_area=%.2f channels=%zu",
        m_procgen_settings.seed,
        m_procgen_settings.width,
        m_procgen_settings.height,
        procgen::NORMALIZED_WATERLINE,
        m_procgen_settings.island_bias,
        m_procgen_settings.coastline_noise_weight,
        m_procgen_settings.base_elevation_weight,
        m_procgen_settings.mountain_weight,
        m_procgen_map.mountain_peaks.size(),
        m_procgen_settings.ridge_weight,
        m_procgen_settings.valley_weight,
        m_procgen_settings.terrain_noise_weight,
        m_procgen_settings.ocean_depth_weight,
        m_procgen_settings.river_min_drainage_area,
        m_procgen_map.rivers.size()
    );
    return true;
}

bool TestApp::refresh_procgen_debug_view(core::Engine& engine) {
    if (!m_procgen_map.has_expected_cell_count()) {
        return false;
    }

    const auto image = procgen::build_greater_realm_debug_image(
        m_procgen_map,
        procgen::NORMALIZED_WATERLINE,
        m_procgen_debug_options
    );
    return replace_procgen_debug_texture(engine, image)
        && refresh_procgen_terrain_mesh(engine, image);
}

bool TestApp::replace_procgen_debug_texture(
    core::Engine& engine,
    const procgen::DebugImage& image
) {
    auto* texture_manager = engine.texture_manager();
    if (!texture_manager) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture manager is null!");
        return false;
    }
    if (!image.has_expected_byte_count()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to build procgen debug image");
        return false;
    }

    const rendering::TextureID new_texture = texture_manager->create_from_rgba_pixels(
        image.width,
        image.height,
        image.rgba
    );
    if (new_texture == rendering::INVALID_TEXTURE_ID) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create procgen debug texture: %s", SDL_GetError());
        return false;
    }

    if (m_test_texture != rendering::INVALID_TEXTURE_ID) {
        texture_manager->destroy(m_test_texture);
    }
    m_test_texture = new_texture;

    if (auto* sprite = engine.world().get_component<rendering::Sprite>(m_test_entity)) {
        sprite->texture_id = m_procgen_presentation.mode == GreaterRealmPresentationMode::Flat
            ? m_test_texture
            : rendering::INVALID_TEXTURE_ID;
        sprite->scale_x = 4.0f;
        sprite->scale_y = 4.0f;
    }

    if (auto* transform = engine.world().get_component<rendering::Transform>(m_test_entity)) {
        transform->x = static_cast<float>(core::Engine::LOGICAL_W) * 0.65f;
        transform->y = static_cast<float>(core::Engine::LOGICAL_H) * 0.50f;
    }
    return true;
}

bool TestApp::refresh_procgen_terrain_mesh(
    core::Engine& engine,
    const procgen::DebugImage& image
) {
    auto* terrain_renderer = engine.terrain_renderer();
    if (!terrain_renderer || !m_procgen_map.has_expected_cell_count()) {
        return false;
    }

    std::vector<float> relative_elevations;
    relative_elevations.reserve(m_procgen_map.cells.size());
    for (const auto& cell : m_procgen_map.cells) {
        relative_elevations.push_back(cell.elevation - procgen::NORMALIZED_WATERLINE);
    }

    auto mesh = rendering::build_heightfield_mesh(
        m_procgen_map.width,
        m_procgen_map.height,
        m_procgen_map.cell_size,
        relative_elevations,
        image.rgba
    );
    if (!mesh.has_expected_shape()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to build greater-realm terrain mesh");
        return false;
    }

    terrain_renderer->set_mesh(std::move(mesh));
    apply_procgen_presentation(engine);
    return true;
}

void TestApp::apply_procgen_presentation(core::Engine& engine) noexcept {
    const bool show_tilted_3d = m_procgen_presentation.mode
        == GreaterRealmPresentationMode::Tilted3D;
    if (auto* terrain_renderer = engine.terrain_renderer()) {
        terrain_renderer->set_elevation_scale(m_procgen_presentation.elevation_scale);
        terrain_renderer->set_enabled(show_tilted_3d);
    }
    if (auto* sprite = engine.world().get_component<rendering::Sprite>(m_test_entity)) {
        sprite->texture_id = show_tilted_3d
            ? rendering::INVALID_TEXTURE_ID
            : m_test_texture;
    }
    if (show_tilted_3d) {
        m_procgen_paint_session.cancel();
    }
}

procgen::TerrainPreviewBounds TestApp::procgen_preview_bounds(
    core::Engine& engine
) const noexcept {
    if (m_procgen_presentation.mode == GreaterRealmPresentationMode::Tilted3D) {
        return {};
    }
    const auto* sprite = engine.world().get_component<rendering::Sprite>(m_test_entity);
    const auto* transform = engine.world().get_component<rendering::Transform>(m_test_entity);
    if (!sprite || !transform || !m_procgen_map.has_expected_cell_count()) {
        return {};
    }

    const float width = static_cast<float>(m_procgen_map.width) * std::abs(sprite->scale_x);
    const float height = static_cast<float>(m_procgen_map.height) * std::abs(sprite->scale_y);
    return {
        transform->x - width * 0.5f,
        transform->y - height * 0.5f,
        width,
        height
    };
}

void TestApp::apply_procgen_paint_sample(
    const procgen::TerrainConstraintPaintSample& sample
) noexcept {
    m_procgen_constraints.paint(
        sample.tool,
        sample.normalized_x,
        sample.normalized_y,
        sample.normalized_radius,
        sample.strength
    );
    m_procgen_paint_dirty = true;
}
#endif

void TestApp::on_startup(core::Engine& engine) {
    SDL_Log("TestApp starting up...");

    auto* ui_renderer = engine.ui_renderer();
    if (ui_renderer) {
        const std::string font_path = (
            config::get_executable_dir() / "assets" / "Fonts" / "Inter-Regular.ttf"
        ).string();
        constexpr float ui_font_size = 16.0f;
        const rendering::FontID font_id = ui_renderer->loadFont(font_path.c_str(), ui_font_size);
        if (font_id != rendering::INVALID_FONT_ID) {
            ui_renderer->setDefaultFont(font_id);
            engine.ui_manager().setFontManager(engine.font_manager(), font_id);
            SDL_Log("Loaded font: %s", font_path.c_str());
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load font: %s", font_path.c_str());
        }
    }

#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    m_procgen_settings.seed = 8675309;
    m_procgen_settings.width = 256;
    m_procgen_settings.height = 192;
    m_procgen_map = procgen::generate_greater_realm(m_procgen_settings, m_procgen_constraints);
    const auto initial_image = procgen::build_greater_realm_debug_image(
        m_procgen_map,
        procgen::NORMALIZED_WATERLINE,
        m_procgen_debug_options
    );
#else
    constexpr std::uint32_t test_texture_width = 24;
    constexpr std::uint32_t test_texture_height = 24;
    std::vector<std::uint8_t> test_pixels(
        static_cast<std::size_t>(test_texture_width) * test_texture_height * 4,
        255
    );
    for (std::size_t index = 0; index < test_pixels.size(); index += 4) {
        test_pixels[index + 1] = 0;
        test_pixels[index + 2] = 0;
    }
#endif

    auto* texture_manager = engine.texture_manager();
    if (!texture_manager) {
        SDL_Log("Texture manager is null!");
        return;
    }

#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    m_test_texture = texture_manager->create_from_rgba_pixels(
        initial_image.width,
        initial_image.height,
        initial_image.rgba
    );
#else
    m_test_texture = texture_manager->create_from_rgba_pixels(
        test_texture_width,
        test_texture_height,
        test_pixels
    );
#endif
    if (m_test_texture == rendering::INVALID_TEXTURE_ID) {
        SDL_Log("Failed to create game texture: %s", SDL_GetError());
        return;
    }

    m_test_entity = engine.world().create_entity();
    const int logical_w = core::Engine::LOGICAL_W;
    const int logical_h = core::Engine::LOGICAL_H;

    rendering::Transform transform;
#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    transform.x = static_cast<float>(logical_w) * 0.65f;
    transform.y = static_cast<float>(logical_h) * 0.50f;
#else
    transform.x = static_cast<float>(logical_w) / 2.0f;
    transform.y = static_cast<float>(logical_h) / 2.0f;
#endif
    transform.z = 0.0f;
    engine.world().add_component(m_test_entity, transform);

    rendering::Sprite sprite;
    sprite.texture_id = m_test_texture;
    sprite.layer = 0;
#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    sprite.scale_x = 4.0f;
    sprite.scale_y = 4.0f;
#endif
    engine.world().add_component(m_test_entity, sprite);

#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    if (!refresh_procgen_terrain_mesh(engine, initial_image)) {
        return;
    }
#endif

    auto& ui_manager = engine.ui_manager();
#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    ui_manager.setRoot(m_procgen_debug_panel.build(
        m_procgen_settings,
        m_procgen_debug_options,
        m_procgen_presentation,
        m_procgen_brush_settings,
        m_procgen_map,
        [this, &engine]() { regenerate_procgen_debug_map(engine); },
        [this](procgen::TerrainConstraintTool tool) {
            m_procgen_paint_session.select_tool(tool);
        },
        [this](procgen::TerrainConstraintBrushSettings settings) {
            m_procgen_brush_settings = procgen::clamp_terrain_constraint_brush_settings(settings);
            m_procgen_paint_session.set_brush_settings(m_procgen_brush_settings);
        },
        [this, &engine]() {
            m_procgen_paint_session.cancel();
            m_procgen_paint_dirty = false;
            m_procgen_constraints.clear();
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() { refresh_procgen_debug_view(engine); },
        [this, &engine]() { apply_procgen_presentation(engine); }
    ));
#else
    auto root = std::make_unique<ui::StackPanel>(ui::Orientation::Vertical);
    root->setPadding(ui::Thickness(20.0f));
    root->setSpacing(10.0f);

    ui::SizeConstraints constraints;
    constraints.preferred_width = static_cast<float>(logical_w) * 0.25f;
    constraints.min_width = static_cast<float>(logical_w) * 0.25f;
    root->setSizeConstraints(constraints);
    root->setBackgroundColour(ui::Colour::white());

    auto button = std::make_unique<ui::Button>("Click Me!");
    button->setBackgroundColour(ui::Colour{80, 120, 200, 255});
    button->setHoverColour(ui::Colour{100, 140, 220, 255});
    button->setPressedColour(ui::Colour{60, 100, 180, 255});
    button->setTextColour(ui::Colour{255, 255, 255, 255});
    button->setBorderColour(ui::Colour{40, 80, 160, 255});
    button->setBorderThickness(2.0f);
    button->setFontSize(16.0f);
    button->setOnClick([]() { SDL_Log("Button clicked."); });

    auto text_box = std::make_unique<ui::TextBox>();
    text_box->setPlaceholder("Enter text here...");
    text_box->setBackgroundColour(ui::Colour{40, 40, 50, 255});
    text_box->setTextColour(ui::Colour{255, 255, 255, 255});
    text_box->setBorderColour(ui::Colour{80, 80, 100, 255});
    text_box->setFocusBorderColour(ui::Colour{100, 150, 255, 255});
    text_box->setBorderThickness(2.0f);
    text_box->setFontSize(14.0f);

    auto combo_box = std::make_unique<ui::ComboBox>();
    combo_box->setPlaceholder("Select a resolution");
    combo_box->addItem("2560 x 1440");
    combo_box->addItem("1920 x 1080");
    combo_box->addItem("1280 x 720");
    combo_box->setBackgroundColour(ui::Colour{240, 240, 240, 255});
    combo_box->setTextColour(ui::Colour{0, 0, 0, 255});
    combo_box->setBorderColour(ui::Colour{180, 180, 180, 255});
    combo_box->setHoverColour(ui::Colour{230, 230, 230, 255});
    combo_box->setDropdownBackgroundColour(ui::Colour{250, 250, 250, 255});
    combo_box->setItemHoverColour(ui::Colour{220, 0, 220, 255});
    combo_box->setBorderThickness(1.0f);
    combo_box->setFontSize(14.0f);
    combo_box->setOnSelectionChanged([&engine](const std::string& selected_item) {
        const auto resolution = core::parseResolutionString(selected_item);
        if (resolution.has_value()) {
            engine.resize_window(resolution->first, resolution->second);
        }
    });

    root->addChild(std::move(button));
    root->addChild(std::move(text_box));
    root->addChild(std::move(combo_box));
    ui_manager.setRoot(std::move(root));
#endif

    SDL_Log("TestApp startup complete");
}

void TestApp::on_update(core::Engine& engine, double dt) {
    (void)dt;
#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    if (m_procgen_paint_dirty) {
        regenerate_procgen_debug_map(engine);
    }
#else
    (void)engine;
#endif
}

void TestApp::on_event(core::Engine& engine, const SDL_Event& event, bool ui_consumed) {
#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST
        || event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
        m_procgen_paint_session.cancel();
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        m_procgen_paint_session.pointer_up();
        return;
    }

    float screen_x = 0.0f;
    float screen_y = 0.0f;
    bool is_pointer_down = false;
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        screen_x = event.button.x;
        screen_y = event.button.y;
        is_pointer_down = true;
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        screen_x = event.motion.x;
        screen_y = event.motion.y;
    } else {
        return;
    }

    float logical_x = 0.0f;
    float logical_y = 0.0f;
    if (!engine.screen_to_logical(screen_x, screen_y, logical_x, logical_y)) {
        return;
    }

    const auto bounds = procgen_preview_bounds(engine);
    const auto sample = is_pointer_down
        ? m_procgen_paint_session.pointer_down(bounds, logical_x, logical_y, ui_consumed)
        : m_procgen_paint_session.pointer_move(bounds, logical_x, logical_y, ui_consumed);
    if (sample) {
        apply_procgen_paint_sample(*sample);
    }
#else
    (void)engine;
    (void)event;
    (void)ui_consumed;
#endif
}

void TestApp::on_shutdown(core::Engine& engine) {
#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    if (auto* terrain_renderer = engine.terrain_renderer()) {
        terrain_renderer->set_enabled(false);
        terrain_renderer->clear_mesh();
    }
#endif
    if (m_test_texture != rendering::INVALID_TEXTURE_ID) {
        engine.texture_manager()->destroy(m_test_texture);
        m_test_texture = rendering::INVALID_TEXTURE_ID;
    }

    if (m_test_entity.is_valid()) {
        engine.world().destroy_entity(m_test_entity);
    }

    SDL_Log("TestApp shutting down...");
}

void TestApp::on_resized(core::Engine& engine, int width, int height) {
    (void)engine;
    (void)width;
    (void)height;
}

} // namespace game
