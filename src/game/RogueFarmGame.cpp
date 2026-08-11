#include "RogueFarmGame.hpp"
#include "../../include/core/Engine.hpp"
#include "../../include/core/Config.hpp"
#include "../../include/core/Utils.hpp"
#include "../../include/rendering/Sprite.hpp"
#include "../../include/rendering/UIRenderer.hpp"
#include "../../include/rendering/FontManager.hpp"
#include "../../include/ui/Button.hpp"
#include "../../include/ui/Layout.hpp"
#include "../../include/ui/ComboBox.hpp"
#include "../../include/ui/Primitives.hpp"
#include <SDL3/SDL.h>

#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
#include "../../include/procgen/GreaterRealm.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#endif

namespace game {

#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
namespace {

struct TerrainCounts {
    std::size_t ocean{0};
    std::size_t coast{0};
    std::size_t plains{0};
    std::size_t hills{0};
    std::size_t highlands{0};
    std::size_t mountains{0};
};

TerrainCounts count_terrain_forms(const procgen::GreaterRealmMap& map) noexcept {
    TerrainCounts counts;

    for (const auto& cell : map.cells) {
        switch (cell.terrain_form) {
            case procgen::TerrainForm::Ocean:
                ++counts.ocean;
                break;
            case procgen::TerrainForm::Coast:
                ++counts.coast;
                break;
            case procgen::TerrainForm::Plains:
                ++counts.plains;
                break;
            case procgen::TerrainForm::Hills:
                ++counts.hills;
                break;
            case procgen::TerrainForm::Highlands:
                ++counts.highlands;
                break;
            case procgen::TerrainForm::Mountains:
                ++counts.mountains;
                break;
        }
    }

    return counts;
}

std::string format_float(float value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

std::string setting_text(const char* label, float value) {
    std::ostringstream stream;
    stream << label << ": " << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

std::string seed_text(procgen::Seed seed) {
    std::ostringstream stream;
    stream << "Seed: " << seed;
    return stream.str();
}

std::string coverage_text(const procgen::GreaterRealmMap& map, const TerrainCounts& counts) {
    const auto total = static_cast<float>(map.cells.size());
    const float land_percent = total > 0.0f ? (total - static_cast<float>(counts.ocean)) * 100.0f / total : 0.0f;
    const float ocean_percent = total > 0.0f ? static_cast<float>(counts.ocean) * 100.0f / total : 0.0f;

    std::ostringstream stream;
    stream << "Land " << format_float(land_percent) << "%  Ocean " << format_float(ocean_percent) << "%";
    return stream.str();
}

std::string terrain_text(const TerrainCounts& counts) {
    std::ostringstream stream;
    stream << "Coast " << counts.coast << "  Mountains " << counts.mountains;
    return stream.str();
}

std::unique_ptr<ui::TextBlock> make_text(const std::string& text, ui::TextBlock** out = nullptr) {
    auto element = std::make_unique<ui::TextBlock>(text);
    element->setFontSize(14.0f);
    element->setColour(ui::Colour{20, 24, 30, 255});
    if (out) {
        *out = element.get();
    }
    return element;
}

std::unique_ptr<ui::Button> make_debug_button(
    const std::string& text,
    ui::Button::ClickCallback callback,
    float min_width = 38.0f
) {
    auto button = std::make_unique<ui::Button>(text);
    button->setBackgroundColour(ui::Colour{54, 88, 128, 255});
    button->setHoverColour(ui::Colour{72, 108, 152, 255});
    button->setPressedColour(ui::Colour{38, 68, 102, 255});
    button->setTextColour(ui::Colour::white());
    button->setBorderColour(ui::Colour{28, 52, 78, 255});
    button->setBorderThickness(1.0f);
    button->setFontSize(14.0f);
    button->setPadding(ui::Thickness(8.0f, 4.0f));
    button->setOnClick(std::move(callback));

    ui::SizeConstraints constraints;
    constraints.min_width = min_width;
    constraints.min_height = 30.0f;
    button->setSizeConstraints(constraints);
    return button;
}

std::unique_ptr<ui::StackPanel> make_control_row(
    const std::string& decrease_text,
    const std::string& increase_text,
    std::unique_ptr<ui::TextBlock> label,
    ui::Button::ClickCallback decrease,
    ui::Button::ClickCallback increase
) {
    auto row = std::make_unique<ui::StackPanel>(ui::Orientation::Horizontal);
    row->setSpacing(4.0f);

    ui::SizeConstraints label_constraints;
    label_constraints.preferred_width = 210.0f;
    label_constraints.min_width = 210.0f;
    label->setSizeConstraints(label_constraints);

    row->addChild(std::move(label));
    row->addChild(make_debug_button(decrease_text, std::move(decrease)));
    row->addChild(make_debug_button(increase_text, std::move(increase)));
    return row;
}

SDL_Color terrain_colour(const procgen::GreaterRealmCell& cell, float sea_level) noexcept {
    const float land_range = sea_level < 0.99f ? 1.0f - sea_level : 0.01f;
    const float relative_land_height = std::clamp((cell.elevation - sea_level) / land_range, 0.0f, 1.0f);
    const float shade = 0.62f + relative_land_height * 0.38f;
    const auto scale = [shade](std::uint8_t value) -> std::uint8_t {
        return static_cast<std::uint8_t>(static_cast<float>(value) * shade);
    };

    switch (cell.terrain_form) {
        case procgen::TerrainForm::Ocean: {
            const float safe_sea_level = sea_level > 0.01f ? sea_level : 0.01f;
            const float relative_depth = 1.0f - std::clamp(cell.elevation / safe_sea_level, 0.0f, 1.0f);
            const float shallow = 1.0f - std::sqrt(relative_depth);
            const auto mix = [shallow](std::uint8_t deep, std::uint8_t coast) -> std::uint8_t {
                return static_cast<std::uint8_t>(
                    static_cast<float>(deep)
                    + (static_cast<float>(coast) - static_cast<float>(deep)) * shallow
                );
            };
            return SDL_Color{mix(3, 66), mix(18, 145), mix(52, 196), 255};
        }
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

SDL_Surface* create_procgen_debug_surface(const procgen::GreaterRealmMap& map, float sea_level) {
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
            const SDL_Color colour = terrain_colour(cell, sea_level);
            row[x] = SDL_MapSurfaceRGBA(surface, colour.r, colour.g, colour.b, colour.a);
        }
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    return surface;
}

} // namespace

bool RogueFarmGame::regenerate_procgen_debug_map(core::Engine& engine) {
    auto* texture_manager = engine.texture_manager();
    if (!texture_manager) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture manager is null!");
        return false;
    }

    const auto map = procgen::generate_greater_realm(m_procgen_settings);
    SDL_Surface* surface = create_procgen_debug_surface(map, m_procgen_settings.sea_level);
    if (!surface) {
        return false;
    }

    const rendering::TextureID new_texture = texture_manager->create_from_surface(surface);
    SDL_DestroySurface(surface);

    if (new_texture == rendering::INVALID_TEXTURE_ID) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create procgen debug texture: %s", SDL_GetError());
        return false;
    }

    if (m_test_texture != rendering::INVALID_TEXTURE_ID) {
        texture_manager->destroy(m_test_texture);
    }

    m_test_texture = new_texture;

    if (auto* sprite = engine.world().get_component<rendering::Sprite>(m_test_entity)) {
        sprite->texture_id = m_test_texture;
        sprite->scale_x = 4.0f;
        sprite->scale_y = 4.0f;
    }

    if (auto* transform = engine.world().get_component<rendering::Transform>(m_test_entity)) {
        transform->x = static_cast<float>(core::Engine::LOGICAL_W) * 0.65f;
        transform->y = static_cast<float>(core::Engine::LOGICAL_H) * 0.50f;
    }

    update_procgen_debug_text(map);
    SDL_Log(
        "Generated procgen debug map: seed=%llu size=%ux%u sea=%.2f land=%.2f island=%.2f coast=%.2f base=%.2f mountain=%.2f ridge=%.2f valley=%.2f noise=%.2f ocean=%.2f",
        m_procgen_settings.seed,
        m_procgen_settings.width,
        m_procgen_settings.height,
        m_procgen_settings.sea_level,
        m_procgen_settings.land_shape_weight,
        m_procgen_settings.island_bias,
        m_procgen_settings.coastline_noise_weight,
        m_procgen_settings.base_elevation_weight,
        m_procgen_settings.mountain_weight,
        m_procgen_settings.ridge_weight,
        m_procgen_settings.valley_weight,
        m_procgen_settings.terrain_noise_weight,
        m_procgen_settings.ocean_depth_weight
    );
    return true;
}

void RogueFarmGame::update_procgen_debug_text(const procgen::GreaterRealmMap& map) {
    if (m_procgen_seed_text) {
        m_procgen_seed_text->setText(seed_text(m_procgen_settings.seed));
    }
    if (m_procgen_sea_text) {
        m_procgen_sea_text->setText(setting_text("Sea", m_procgen_settings.sea_level));
    }
    if (m_procgen_land_shape_text) {
        m_procgen_land_shape_text->setText(setting_text("Land shape", m_procgen_settings.land_shape_weight));
    }
    if (m_procgen_island_bias_text) {
        m_procgen_island_bias_text->setText(setting_text("Island bias", m_procgen_settings.island_bias));
    }
    if (m_procgen_coastline_noise_text) {
        m_procgen_coastline_noise_text->setText(setting_text("Coast detail", m_procgen_settings.coastline_noise_weight));
    }
    if (m_procgen_base_elevation_text) {
        m_procgen_base_elevation_text->setText(setting_text("Base relief", m_procgen_settings.base_elevation_weight));
    }
    if (m_procgen_mountain_text) {
        m_procgen_mountain_text->setText(setting_text("Mountain", m_procgen_settings.mountain_weight));
    }
    if (m_procgen_ridge_text) {
        m_procgen_ridge_text->setText(setting_text("Ridge", m_procgen_settings.ridge_weight));
    }
    if (m_procgen_valley_text) {
        m_procgen_valley_text->setText(setting_text("Valley", m_procgen_settings.valley_weight));
    }
    if (m_procgen_noise_text) {
        m_procgen_noise_text->setText(setting_text("Terrain noise", m_procgen_settings.terrain_noise_weight));
    }
    if (m_procgen_ocean_depth_text) {
        m_procgen_ocean_depth_text->setText(setting_text("Ocean depth", m_procgen_settings.ocean_depth_weight));
    }
    if (m_procgen_coverage_text || m_procgen_terrain_text) {
        const auto counts = count_terrain_forms(map);
        if (m_procgen_coverage_text) {
            m_procgen_coverage_text->setText(coverage_text(map, counts));
        }
        if (m_procgen_terrain_text) {
            m_procgen_terrain_text->setText(terrain_text(counts));
        }
    }
}

void RogueFarmGame::create_procgen_debug_ui(core::Engine& engine, const procgen::GreaterRealmMap& map) {
    auto& ui_mgr = engine.ui_manager();

    auto root = std::make_unique<ui::StackPanel>(ui::Orientation::Vertical);
    root->setPadding(ui::Thickness(10.0f));
    root->setSpacing(5.0f);
    root->setBackgroundColour(ui::Colour{238, 242, 238, 240});

    ui::SizeConstraints root_constraints;
    root_constraints.preferred_width = 430.0f;
    root_constraints.min_width = 430.0f;
    root->setSizeConstraints(root_constraints);

    auto title = make_text("Greater Realm Debug");
    title->setFontSize(20.0f);
    title->setColour(ui::Colour{10, 18, 24, 255});
    root->addChild(std::move(title));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(seed_text(m_procgen_settings.seed), &m_procgen_seed_text),
        [this, &engine]() {
            if (m_procgen_settings.seed > 0) {
                --m_procgen_settings.seed;
            }
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            ++m_procgen_settings.seed;
            regenerate_procgen_debug_map(engine);
        }
    ));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(setting_text("Island bias", m_procgen_settings.island_bias), &m_procgen_island_bias_text),
        [this, &engine]() {
            m_procgen_settings.island_bias = std::clamp(m_procgen_settings.island_bias - 0.10f, 0.0f, 2.0f);
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            m_procgen_settings.island_bias = std::clamp(m_procgen_settings.island_bias + 0.10f, 0.0f, 2.0f);
            regenerate_procgen_debug_map(engine);
        }
    ));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(setting_text("Land shape", m_procgen_settings.land_shape_weight), &m_procgen_land_shape_text),
        [this, &engine]() {
            m_procgen_settings.land_shape_weight = std::clamp(m_procgen_settings.land_shape_weight - 0.05f, 0.20f, 2.0f);
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            m_procgen_settings.land_shape_weight = std::clamp(m_procgen_settings.land_shape_weight + 0.05f, 0.20f, 2.0f);
            regenerate_procgen_debug_map(engine);
        }
    ));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(setting_text("Coast detail", m_procgen_settings.coastline_noise_weight), &m_procgen_coastline_noise_text),
        [this, &engine]() {
            m_procgen_settings.coastline_noise_weight = std::clamp(m_procgen_settings.coastline_noise_weight - 0.01f, 0.0f, 0.40f);
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            m_procgen_settings.coastline_noise_weight = std::clamp(m_procgen_settings.coastline_noise_weight + 0.01f, 0.0f, 0.40f);
            regenerate_procgen_debug_map(engine);
        }
    ));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(setting_text("Base relief", m_procgen_settings.base_elevation_weight), &m_procgen_base_elevation_text),
        [this, &engine]() {
            m_procgen_settings.base_elevation_weight = std::clamp(m_procgen_settings.base_elevation_weight - 0.05f, 0.0f, 2.0f);
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            m_procgen_settings.base_elevation_weight = std::clamp(m_procgen_settings.base_elevation_weight + 0.05f, 0.0f, 2.0f);
            regenerate_procgen_debug_map(engine);
        }
    ));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(setting_text("Sea", m_procgen_settings.sea_level), &m_procgen_sea_text),
        [this, &engine]() {
            m_procgen_settings.sea_level = std::clamp(m_procgen_settings.sea_level - 0.02f, 0.10f, 0.90f);
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            m_procgen_settings.sea_level = std::clamp(m_procgen_settings.sea_level + 0.02f, 0.10f, 0.90f);
            regenerate_procgen_debug_map(engine);
        }
    ));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(setting_text("Mountain", m_procgen_settings.mountain_weight), &m_procgen_mountain_text),
        [this, &engine]() {
            m_procgen_settings.mountain_weight = std::clamp(m_procgen_settings.mountain_weight - 0.05f, 0.0f, 1.5f);
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            m_procgen_settings.mountain_weight = std::clamp(m_procgen_settings.mountain_weight + 0.05f, 0.0f, 1.5f);
            regenerate_procgen_debug_map(engine);
        }
    ));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(setting_text("Ridge", m_procgen_settings.ridge_weight), &m_procgen_ridge_text),
        [this, &engine]() {
            m_procgen_settings.ridge_weight = std::clamp(m_procgen_settings.ridge_weight - 0.05f, 0.0f, 1.5f);
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            m_procgen_settings.ridge_weight = std::clamp(m_procgen_settings.ridge_weight + 0.05f, 0.0f, 1.5f);
            regenerate_procgen_debug_map(engine);
        }
    ));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(setting_text("Valley", m_procgen_settings.valley_weight), &m_procgen_valley_text),
        [this, &engine]() {
            m_procgen_settings.valley_weight = std::clamp(m_procgen_settings.valley_weight - 0.05f, 0.0f, 1.5f);
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            m_procgen_settings.valley_weight = std::clamp(m_procgen_settings.valley_weight + 0.05f, 0.0f, 1.5f);
            regenerate_procgen_debug_map(engine);
        }
    ));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(setting_text("Terrain noise", m_procgen_settings.terrain_noise_weight), &m_procgen_noise_text),
        [this, &engine]() {
            m_procgen_settings.terrain_noise_weight = std::clamp(m_procgen_settings.terrain_noise_weight - 0.10f, 0.0f, 2.0f);
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            m_procgen_settings.terrain_noise_weight = std::clamp(m_procgen_settings.terrain_noise_weight + 0.10f, 0.0f, 2.0f);
            regenerate_procgen_debug_map(engine);
        }
    ));

    root->addChild(make_control_row(
        "-",
        "+",
        make_text(setting_text("Ocean depth", m_procgen_settings.ocean_depth_weight), &m_procgen_ocean_depth_text),
        [this, &engine]() {
            m_procgen_settings.ocean_depth_weight = std::clamp(m_procgen_settings.ocean_depth_weight - 0.25f, 0.0f, 3.0f);
            regenerate_procgen_debug_map(engine);
        },
        [this, &engine]() {
            m_procgen_settings.ocean_depth_weight = std::clamp(m_procgen_settings.ocean_depth_weight + 0.25f, 0.0f, 3.0f);
            regenerate_procgen_debug_map(engine);
        }
    ));

    auto button_row = std::make_unique<ui::StackPanel>(ui::Orientation::Horizontal);
    button_row->setSpacing(6.0f);
    button_row->addChild(make_debug_button("Regenerate", [this, &engine]() {
        regenerate_procgen_debug_map(engine);
    }));
    button_row->addChild(make_debug_button("Random Seed", [this, &engine]() {
        m_procgen_settings.seed += 101;
        regenerate_procgen_debug_map(engine);
    }));
    root->addChild(std::move(button_row));

    auto coverage = make_text("", &m_procgen_coverage_text);
    coverage->setColour(ui::Colour{38, 44, 48, 255});
    ui::SizeConstraints summary_constraints;
    summary_constraints.preferred_width = 410.0f;
    summary_constraints.min_width = 410.0f;
    coverage->setSizeConstraints(summary_constraints);
    root->addChild(std::move(coverage));

    auto terrain = make_text("", &m_procgen_terrain_text);
    terrain->setColour(ui::Colour{38, 44, 48, 255});
    terrain->setSizeConstraints(summary_constraints);
    root->addChild(std::move(terrain));

    ui_mgr.setRoot(std::move(root));
    update_procgen_debug_text(map);
}
#endif

void RogueFarmGame::on_startup(core::Engine& engine) {
    SDL_Log("RogueFarmGame starting up...");

    // Load font
    auto* ui_renderer = engine.ui_renderer();
    if (ui_renderer) {
        const std::string fontPath = (
            config::get_executable_dir() / "assets" / "Fonts" / "Inter-Regular.ttf"
        ).string();
#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
        constexpr float ui_font_size = 16.0f;
#else
        constexpr float ui_font_size = 16.0f;
#endif
        rendering::FontID fontId = ui_renderer->loadFont(fontPath.c_str(), ui_font_size);
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
    m_procgen_settings.seed = 8675309;
    m_procgen_settings.width = 256;
    m_procgen_settings.height = 192;
    m_procgen_settings.sea_level = 0.5f;
    const auto initial_map = procgen::generate_greater_realm(m_procgen_settings);
    surface = create_procgen_debug_surface(initial_map, m_procgen_settings.sea_level);
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
#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
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

#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
    create_procgen_debug_ui(engine, initial_map);
#else
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
#endif

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
