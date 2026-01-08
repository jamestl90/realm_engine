#pragma once

#include <string>
#include <cstdint>

namespace rendering {

struct ShaderReflectionData {
    std::uint32_t num_uniform_buffers{0};
    std::uint32_t num_storage_textures{0}; 
    std::uint32_t num_storage_buffers{0};
    std::uint32_t num_samplers{0};
    std::uint32_t num_sets{1};
    std::uint32_t ubo_block_size{0};
    std::uint32_t ubo_binding{0};
    std::uint32_t ubo_set{0};

    // Validate UBO size matches shader expectations
    template<typename T>
    [[nodiscard]] bool validate_ubo_size() const noexcept {
        return sizeof(T) == ubo_block_size;
    }
};

// Load shader reflection data from JSON file
[[nodiscard]] ShaderReflectionData load_shader_reflection(const std::string& reflection_path);

} // namespace rendering
