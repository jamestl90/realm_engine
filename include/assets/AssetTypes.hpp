#pragma once

#include "../rendering/Texture.hpp"
#include "../rendering/PipelineTypes.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <array>

namespace assets {

// Texture filter modes
enum class FilterMode : std::uint8_t {
    Nearest,    // Pixel-perfect, no filtering
    Linear      // Bilinear filtering
};

// Texture wrap modes
enum class WrapMode : std::uint8_t {
    Clamp,      // Clamp to edge
    Repeat      // Tile/repeat
};

// Texture asset with metadata
struct TextureAsset {
    rendering::TextureID texture_id{rendering::INVALID_TEXTURE_ID};
    std::uint16_t width{0};
    std::uint16_t height{0};
    FilterMode filter{FilterMode::Nearest};
    WrapMode wrap{WrapMode::Clamp};
    std::string source_path;

    // Atlas regions (name -> region)
    std::unordered_map<std::string, rendering::TextureRegion> regions;
};

// Audio asset with metadata
struct AudioAsset {
    std::uint8_t* buffer{nullptr};
    std::uint32_t length{0};
    std::uint32_t sample_rate{44100};
    std::uint8_t channels{2};
    float duration{0.0f};
    float default_volume{1.0f};
    bool streaming{false};  // For music tracks
    std::string source_path;
};

// Glyph metrics for font rendering
struct GlyphMetrics {
    float advance{0.0f};        // Horizontal advance
    float bearing_x{0.0f};      // Left bearing
    float bearing_y{0.0f};      // Top bearing
    std::uint16_t width{0};     // Glyph width in pixels
    std::uint16_t height{0};    // Glyph height in pixels
    rendering::TextureRegion region;  // UV coordinates in atlas
};

// Font asset with glyph data
struct FontAsset {
    rendering::TextureID atlas_texture{rendering::INVALID_TEXTURE_ID};
    std::uint16_t atlas_width{0};
    std::uint16_t atlas_height{0};
    float line_height{0.0f};
    float base_size{0.0f};      // Font size this was generated at
    bool is_sdf{false};         // Signed distance field font
    std::string source_path;

    // Glyph data (codepoint -> metrics)
    std::unordered_map<std::uint32_t, GlyphMetrics> glyphs;

    // Kerning pairs (packed pair -> adjustment)
    std::unordered_map<std::uint64_t, float> kerning;

    // Helper to get kerning between two codepoints
    [[nodiscard]] float get_kerning(std::uint32_t first, std::uint32_t second) const noexcept {
        const std::uint64_t key = (static_cast<std::uint64_t>(first) << 32) | second;
        auto it = kerning.find(key);
        return it != kerning.end() ? it->second : 0.0f;
    }
};

// Animation frame data
struct AnimationFrame {
    std::uint16_t region_index{0};  // Index into texture atlas
    float duration{0.1f};           // Duration in seconds
    // Optional per-frame events (index into event array)
    std::int16_t event_index{-1};
};

// Animation event triggered during playback
struct AnimationEvent {
    std::string name;
    float value{0.0f};
};

// Animation asset
struct AnimationAsset {
    std::vector<AnimationFrame> frames;
    std::vector<AnimationEvent> events;
    float total_duration{0.0f};
    bool looping{true};
    std::string source_path;
};

// Generic data asset (parsed JSON)
struct DataAsset {
    std::string source_path;
    std::vector<std::uint8_t> raw_data;  // Raw JSON bytes for re-parsing
    // Parsed data is accessed via typed accessors in AssetManager
};

// Asset metadata for manifest
struct AssetMetadata {
    std::string path;
    std::string type;
    std::uint64_t size{0};
    std::uint64_t last_modified{0};
    std::vector<std::string> dependencies;
    bool preload{false};
    bool pinned{false};
};

// ===== Tileset Types =====

// Tile collision shape types
enum class TileCollisionType : std::uint8_t {
    None,       // No collision
    Full,       // Full tile collision
    Half_Top,   // Top half only
    Half_Bottom,// Bottom half only
    Half_Left,  // Left half only
    Half_Right, // Right half only
    Slope_NE,   // Slope from bottom-left to top-right
    Slope_NW,   // Slope from bottom-right to top-left
    Slope_SE,   // Slope from top-left to bottom-right
    Slope_SW,   // Slope from top-right to bottom-left
    Custom      // Custom polygon (use collision_points)
};

// Individual tile properties within a tileset
struct TileProperties {
    TileCollisionType collision{TileCollisionType::None};
    std::uint32_t flags{0};         // Custom bitflags (walkable, water, etc.)
    std::uint8_t animation_length{0}; // Number of frames if animated (0 = static)
    std::uint8_t animation_speed{10}; // Frames per second for animated tiles
    float probability{1.0f};        // For random tile placement (terrain)
    
    // Custom collision polygon (normalised 0-1 within tile)
    // Only used when collision == TileCollisionType::Custom
    std::vector<std::pair<float, float>> collision_points;
    
    // Custom string properties from Tiled
    std::unordered_map<std::string, std::string> custom_properties;
};

// Terrain definition for auto-tiling
struct TerrainDefinition {
    std::string name;
    std::uint32_t color{0xFFFFFFFF};  // Editor colour (RGBA)
    
    // Tile indices for each terrain corner configuration
    // Index is bitmask: top-left(1) | top-right(2) | bottom-left(4) | bottom-right(8)
    std::array<std::vector<std::uint16_t>, 16> corner_tiles;
};

// Tileset asset - grid-based texture with tile metadata
struct TilesetAsset {
    rendering::TextureID texture_id{rendering::INVALID_TEXTURE_ID};
    std::string source_path;
    
    // Texture dimensions
    std::uint16_t image_width{0};
    std::uint16_t image_height{0};
    
    // Tile grid configuration
    std::uint16_t tile_width{16};
    std::uint16_t tile_height{16};
    std::uint16_t columns{0};       // Tiles per row
    std::uint16_t rows{0};          // Tiles per column
    std::uint16_t tile_count{0};    // Total number of tiles
    std::uint16_t margin{0};        // Margin around tileset image (pixels)
    std::uint16_t spacing{0};       // Spacing between tiles (pixels)
    
    // First global tile ID (for Tiled compatibility)
    std::uint32_t first_gid{1};
    
    // Per-tile properties (indexed by local tile ID)
    std::vector<TileProperties> tile_properties;
    
    // Terrain definitions for auto-tiling
    std::vector<TerrainDefinition> terrains;
    
    // Animated tile definitions: local_tile_id -> list of frame tile IDs
    std::unordered_map<std::uint16_t, std::vector<std::uint16_t>> animated_tiles;
    
    // Get texture region for a tile by local ID
    [[nodiscard]] rendering::TextureRegion get_tile_region(std::uint16_t local_id) const noexcept {
        if (local_id >= tile_count || columns == 0) {
            return rendering::TextureRegion{};
        }
        
        const std::uint16_t col = local_id % columns;
        const std::uint16_t row = local_id / columns;
        
        const float pixel_x = static_cast<float>(margin + col * (tile_width + spacing));
        const float pixel_y = static_cast<float>(margin + row * (tile_height + spacing));
        
        const float inv_width = 1.0f / static_cast<float>(image_width);
        const float inv_height = 1.0f / static_cast<float>(image_height);
        
        return rendering::TextureRegion{
            pixel_x * inv_width,
            pixel_y * inv_height,
            (pixel_x + tile_width) * inv_width,
            (pixel_y + tile_height) * inv_height,
            tile_width,
            tile_height
        };
    }
    
    // Get tile properties by local ID
    [[nodiscard]] const TileProperties* get_tile_properties(std::uint16_t local_id) const noexcept {
        if (local_id >= tile_properties.size()) {
            return nullptr;
        }
        return &tile_properties[local_id];
    }
    
    // Convert global tile ID to local tile ID
    [[nodiscard]] std::uint16_t global_to_local(std::uint32_t global_id) const noexcept {
        if (global_id < first_gid || global_id >= first_gid + tile_count) {
            return 0xFFFF; // Invalid
        }
        return static_cast<std::uint16_t>(global_id - first_gid);
    }
    
    // Convert local tile ID to global tile ID
    [[nodiscard]] std::uint32_t local_to_global(std::uint16_t local_id) const noexcept {
        return first_gid + local_id;
    }
    
    // Check if a global tile ID belongs to this tileset
    [[nodiscard]] bool contains_gid(std::uint32_t global_id) const noexcept {
        return global_id >= first_gid && global_id < first_gid + tile_count;
    }
};

// ===== Pipeline Types =====

// Pipeline asset - data-driven pipeline configuration
struct PipelineAsset {
    rendering::PipelineType base_type{rendering::PipelineType::Sprite};
    rendering::PipelineConfig config;
    std::uint32_t pipeline_handle{0}; // Handle from PipelineManager
    std::string vertex_shader_path;
    std::string fragment_shader_path;
    std::string source_path;
};

} // namespace assets
