#include "../../include/rendering/ShaderReflection.hpp"
#include <fstream>
#include <rapidjson/document.h>
#include <SDL3/SDL.h>

namespace rendering {

ShaderReflectionData load_shader_reflection(const std::string& reflection_path) {
    ShaderReflectionData data{};
    
    std::ifstream file(reflection_path);
    if (!file.is_open()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "Could not open reflection file: %s", reflection_path.c_str());
        return data;
    }

    std::string json_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    if (doc.Parse(json_str.c_str()).HasParseError()) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to parse reflection JSON: %s", reflection_path.c_str());
        return data;
    }

    // Parse UBO information if present
    if (doc.HasMember("ubos") && doc["ubos"].IsArray()) {
        data.num_uniform_buffers = doc["ubos"].Size();
        if (data.num_uniform_buffers > 0) {
            const auto& ubo = doc["ubos"][0];
            if (ubo.HasMember("block_size")) {
                data.ubo_block_size = ubo["block_size"].GetUint();
            }
            if (ubo.HasMember("binding")) {
                data.ubo_binding = ubo["binding"].GetUint();
            }
            if (ubo.HasMember("set")) {
                data.ubo_set = ubo["set"].GetUint();
            }
        }
    }

    // For SDL3 GPU API:
    // - num_samplers = number of texture+sampler pairs
    // - SDL3 binds textures and samplers together at the same slot index
    // 
    // The separate_images array tells us how many sampled textures the shader uses.
    // Each sampled texture needs a corresponding sampler, so this count represents
    // the number of texture-sampler pairs to bind.
    if (doc.HasMember("separate_images") && doc["separate_images"].IsArray()) {
        data.num_samplers = doc["separate_images"].Size();
    } else if (doc.HasMember("separate_samplers") && doc["separate_samplers"].IsArray()) {
        // Fallback to separate_samplers count if no images declared
        data.num_samplers = doc["separate_samplers"].Size();
    }

    // Check for combined image samplers (textures array in some reflection formats)
    if (doc.HasMember("textures") && doc["textures"].IsArray()) {
        std::uint32_t combined_count = doc["textures"].Size();
        if (combined_count > 0) {
            data.num_samplers = combined_count;
        }
    }

    // Parse storage textures (read-write images, not sampled textures)
    if (doc.HasMember("storage_textures") && doc["storage_textures"].IsArray()) {
        data.num_storage_textures = doc["storage_textures"].Size();
    }

    // Parse storage buffers 
    if (doc.HasMember("storage_buffers") && doc["storage_buffers"].IsArray()) {
        data.num_storage_buffers = doc["storage_buffers"].Size();
    }

    // Also check for ssbos (alternative naming)
    if (doc.HasMember("ssbos") && doc["ssbos"].IsArray()) {
        data.num_storage_buffers = doc["ssbos"].Size();
    }

    return data;
}

} // namespace rendering
