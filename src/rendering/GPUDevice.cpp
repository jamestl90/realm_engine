#include "../../include/rendering/GPUDevice.hpp"
#include "../../include/rendering/PipelineManager.hpp"
#include <cassert>

namespace rendering {

GPUDevice::GPUDevice() {

    SDL_SetHint(SDL_HINT_GPU_DRIVER, "vulkan"); 
    gpu_device_ = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, 
        true, 
        "Main GPU Device"
    );
    
    if (!gpu_device_) {
        SDL_Log("Failed to create GPU device: %s", SDL_GetError());
        return;
    }

    // Create pipeline manager
    pipeline_manager_ = std::make_unique<PipelineManager>(this);
    
    // Initialize pipelines with default swapchain format
    if (!pipeline_manager_->initialise(SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM)) {
        SDL_Log("Failed to initialize pipeline manager");
        return;
    }
}

GPUDevice::~GPUDevice() {
    if (pipeline_manager_) {
        pipeline_manager_.reset();
    }
    if (gpu_device_) {
        SDL_DestroyGPUDevice(gpu_device_);
        gpu_device_ = nullptr;
    }
}

GPUDevice::GPUDevice(GPUDevice&& other) noexcept
    : gpu_device_(other.gpu_device_) {
    other.gpu_device_ = nullptr;
}

GPUDevice& GPUDevice::operator=(GPUDevice&& other) noexcept {
    if (this != &other) {
        if (gpu_device_) {
            SDL_DestroyGPUDevice(gpu_device_);
        }

        gpu_device_ = other.gpu_device_;
        other.gpu_device_ = nullptr;
    }
    return *this;
}

const char* GPUDevice::get_device_name() const noexcept {
    if (!gpu_device_) return "Invalid Device";
    
    return SDL_GetGPUDeviceDriver(gpu_device_);
}

bool GPUDevice::supports_feature(uint32_t flags) const noexcept {
    if (!gpu_device_) return false;
    
    uint32_t device_flags = SDL_GetGPUDeviceProperties(gpu_device_);
    return (device_flags & flags) != 0;
}

SDL_GPUTexture* GPUDevice::create_texture(Uint32 format, int access, int w, int h) const {
    if (!gpu_device_) return nullptr;
    
    SDL_GPUTextureCreateInfo createInfo = {};
    createInfo.type = SDL_GPU_TEXTURETYPE_2D;
    createInfo.format = static_cast<SDL_GPUTextureFormat>(format);
    createInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    createInfo.width = w;
    createInfo.height = h;
    createInfo.layer_count_or_depth = 1;
    createInfo.num_levels = 1;
    createInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    
    return SDL_CreateGPUTexture(gpu_device_, &createInfo);
}

} // namespace rendering
