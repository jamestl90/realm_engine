#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <memory>
#include <string>

namespace rendering {

// Forward declarations
class PipelineManager;

class GPUDevice {
public:
    explicit GPUDevice();
    ~GPUDevice();

    // Non-copyable, movable
    GPUDevice(const GPUDevice&) = delete;
    GPUDevice& operator=(const GPUDevice&) = delete;
    GPUDevice(GPUDevice&&) noexcept;
    GPUDevice& operator=(GPUDevice&&) noexcept;

    // Device info and capabilities
    [[nodiscard]] SDL_GPUDevice* handle() const noexcept { return gpu_device_; }
    [[nodiscard]] bool is_valid() const noexcept { return gpu_device_ != nullptr; }
    [[nodiscard]] const char* get_device_name() const noexcept;
    [[nodiscard]] bool supports_feature(uint32_t flags) const noexcept;

    // Resource creation helpers
    [[nodiscard]] SDL_GPUTexture* create_texture(Uint32 format, int access, int w, int h) const;

    // Pipeline access
    [[nodiscard]] PipelineManager* pipeline_manager() noexcept { return pipeline_manager_.get(); }
    [[nodiscard]] const PipelineManager* pipeline_manager() const noexcept { return pipeline_manager_.get(); }

private:
    SDL_GPUDevice* gpu_device_{nullptr};
    SDL_Window* window_{nullptr};
    SDL_GPUShaderFormat format_flags_{SDL_GPU_SHADERFORMAT_SPIRV}; // Default to SPIR-V
    std::unique_ptr<PipelineManager> pipeline_manager_;
    SDL_GPUCommandBuffer* command_buffer_{nullptr};
    SDL_GPURenderPass* current_pass_{nullptr};
};

} // namespace rendering
