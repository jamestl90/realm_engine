#pragma once

#include "PipelineTypes.hpp"
#include "ShaderReflection.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <array>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace rendering {

// Forward declarations
class GPUDevice;

// Shader bytecode container
struct ShaderBytecode {
    std::vector<std::uint8_t> data;
    SDL_GPUShaderStage stage;
    std::uint32_t num_samplers{0};
    std::uint32_t num_storage_textures{0};
    std::uint32_t num_storage_buffers{0};
    std::uint32_t num_uniform_buffers{0};
};

// Pipeline handle for external use
using PipelineHandle = std::uint32_t;
constexpr PipelineHandle INVALID_PIPELINE_HANDLE = 0;

// Hash for pipeline configurations (for cache lookup)
struct PipelineConfigHash {
    std::size_t operator()(const PipelineConfig& config) const noexcept;
};

// Equality comparison for pipeline configurations
struct PipelineConfigEqual {
    bool operator()(const PipelineConfig& a, const PipelineConfig& b) const noexcept;
};

// Pipeline manager - handles creation, caching, and lifecycle of GPU pipelines
class PipelineManager {
public:
    explicit PipelineManager(GPUDevice* device);
    ~PipelineManager();

    // Non-copyable, movable
    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;
    PipelineManager(PipelineManager&&) noexcept;
    PipelineManager& operator=(PipelineManager&&) noexcept;

    // Initialise core pipelines (call after device is ready)
    [[nodiscard]] bool initialise(SDL_GPUTextureFormat swapchain_format);

    // Get a core pipeline by type
    [[nodiscard]] SDL_GPUGraphicsPipeline* get_core_pipeline(PipelineType type) const noexcept;

    // Get or create a custom pipeline with specific configuration
    [[nodiscard]] PipelineHandle get_or_create_pipeline(
        PipelineType base_type,
        const PipelineConfig& config
    );

    // Get pipeline by handle
    [[nodiscard]] SDL_GPUGraphicsPipeline* get_pipeline(PipelineHandle handle) const noexcept;

    // Load shader from file
    [[nodiscard]] std::optional<ShaderBytecode> load_shader(
        const std::string& path,
        SDL_GPUShaderStage stage
    ) const;

    // Release all pipelines
    void release_all();

    // Get the default configuration for a pipeline type
    [[nodiscard]] static PipelineConfig get_default_config(PipelineType type) noexcept;

private:
    // Internal pipeline entry
    struct PipelineEntry {
        SDL_GPUGraphicsPipeline* pipeline{nullptr};
        PipelineType base_type{PipelineType::Sprite};
        PipelineConfig config;
    };

    // Create shader module from bytecode
    [[nodiscard]] SDL_GPUShader* create_shader(const ShaderBytecode& bytecode) const;

    // Create a graphics pipeline
    [[nodiscard]] SDL_GPUGraphicsPipeline* create_pipeline(
        PipelineType type,
        const PipelineConfig& config,
        SDL_GPUTextureFormat target_format,
        const ShaderReflectionData& vert_reflection,
        const ShaderReflectionData& frag_reflection
    );

    // Build vertex input state for pipeline type
    [[nodiscard]] SDL_GPUVertexInputState build_vertex_input_state(PipelineType type) const noexcept;

    // Initialise individual core pipelines
    [[nodiscard]] bool initialise_sprite_pipeline(SDL_GPUTextureFormat format);

    GPUDevice* device_{nullptr};
    SDL_GPUTextureFormat swapchain_format_{SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM};

    // Core pipelines (indexed by PipelineType)
    std::array<SDL_GPUGraphicsPipeline*, static_cast<std::size_t>(PipelineType::Count)> core_pipelines_{};

    // Cached shaders for core pipeline types
    std::array<SDL_GPUShader*, static_cast<std::size_t>(PipelineType::Count)> vertex_shaders_{};
    std::array<SDL_GPUShader*, static_cast<std::size_t>(PipelineType::Count)> fragment_shaders_{};

    // Custom pipeline cache
    std::unordered_map<PipelineConfig, PipelineHandle, PipelineConfigHash, PipelineConfigEqual> pipeline_cache_;
    std::vector<PipelineEntry> pipelines_;
    PipelineHandle next_handle_{1};

    std::string path_to_assets_ = "\\assets\\";
};

} // namespace rendering
